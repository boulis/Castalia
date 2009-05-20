//***************************************************************************************
//*  Copyright: National ICT Australia,  2007, 2008, 2009				*
//*  Developed at the Networks and Pervasive Computing program				*
//*  Author(s): Yuri Tselishchev							*
//*  This file is distributed under the terms in the attached LICENSE file.		*
//*  If you do not find this file, copies can be found by writing to:			*
//*											*
//*      NICTA, Locked Bag 9013, Alexandria, NSW 1435, Australia			*
//*      Attention:  License Inquiry.							*
//*											*
//***************************************************************************************


#include "BridgeTest_ApplicationModule.h"

#define EV   ev.disabled()?(ostream&)ev:ev

#define CASTALIA_DEBUG (!printDebugInfo)?(ostream&)DebugInfoWriter::getStream():DebugInfoWriter::getStream()

#define DRIFTED_TIME(time) ((time) * cpuClockDrift)

#define MEMORY_PROFILE_STORE(numBytes) resMgrModule->RamStore(numBytes)

#define MEMORY_PROFILE_FREE(numBytes) resMgrModule->RamFree(numBytes)

#define STARTUP_DELAY 0.05

#define CIRCUITS_PREPARE_DELAY 0.001
#define REPROGRAM_INTERVAL 86400
#define REPROGRAM_PAYLOAD 5120
#define REPROGRAM_PACKET_DELAY 0.5
#define NO_ROUTING_LEVEL -1

#define REPORT_PACKET "Report packet"
#define VERSION_PACKET "Version packet"

/************ Implementation Functions of TunableMAC set functions ******************************/
//  If you are not going to use the TunableMAC module, then you can comment the following line and build Castalia
#include "BridgeTest_radioControlImpl.cc.inc"
#include "BridgeTest_tunableMacControlImpl.cc.inc"
/************ Connectivity Map Definitions  ************************************************/


Define_Module(BridgeTest_ApplicationModule);



void BridgeTest_ApplicationModule::initialize() 
{
	self = parentModule()->index();
	self_xCoo = parentModule()->par("xCoor");
	self_yCoo = parentModule()->par("yCoor");
	
	//get a valid reference to the object of the Resources Manager module so that we can make direct calls to its public methods
	//instead of using extra messages & message types for tighlty couplped operations.
	cModule *parent = parentModule();
	if(parent->findSubmodule("nodeResourceMgr") != -1) {	
	    resMgrModule = check_and_cast<ResourceGenericManager*>(parent->submodule("nodeResourceMgr"));
	} else {
	    opp_error("\n[Application]:\n Error in geting a valid reference to  nodeResourceMgr for direct method calls.");
	}
	cpuClockDrift = resMgrModule->getCPUClockDrift();
	
	disabled = 1;
	
	const char * tmpID = par("applicationID");
	applicationID.assign(tmpID);
	
	printDebugInfo = par("printDebugInfo");
	priority = par("priority");
	maxAppPacketSize = par("maxAppPacketSize");
	packetHeaderOverhead = par("packetHeaderOverhead");
	isSink = par("isSink");
	reportTreshold = par("reportTreshold");
	reportInterval = par("reportInterval");
	reportInterval = reportInterval/1000;
	broadcastReports = par("broadcastReports");
	
	char buff[30];
	sprintf(buff, "Application Vector of Node %d", self);
	appVector.setName(buff);
	
	sprintf(selfAddr, "%i", self);
	
	// Send the STARTUP message to MAC & to Sensor_Manager modules so that the node start to operate. (after a random delay, because we don't want the nodes to be synchronized)
	double random_startup_delay = genk_dblrand(0) * STARTUP_DELAY + CIRCUITS_PREPARE_DELAY;
	sendDelayed(new App_ControlMessage("Application --> Sensor Dev Mgr [STARTUP]", APP_NODE_STARTUP), simTime() + DRIFTED_TIME(random_startup_delay), "toSensorDeviceManager");
	sendDelayed(new App_ControlMessage("Application --> Network [STARTUP]", APP_NODE_STARTUP), simTime() + DRIFTED_TIME(random_startup_delay), "toCommunicationModule");
	scheduleAt(simTime() + DRIFTED_TIME(random_startup_delay), new App_ControlMessage("Application --> Application (self)", APP_NODE_STARTUP));
	
	/**
	  ADD HERE INITIALIZATION CODE HERE FOR YOUR APPLICATION
	 **/
	 currentVersionPacket = 0;
	 currentVersion = 0;
	 currSentSampleSN = 0;
	 outOfEnergy = 0;
	 div_t tmp_div = div(REPROGRAM_PAYLOAD,maxAppPacketSize-packetHeaderOverhead);
	 totalVersionPackets = tmp_div.rem > 0 ? tmp_div.quot + 1 : tmp_div.quot;
	 
	 routingLevel = (isSink)?0:NO_ROUTING_LEVEL;
	 
	 version_info_table.clear();
	 report_info_table.clear();
}


void BridgeTest_ApplicationModule::handleMessage(cMessage *msg)
{
	int msgKind = msg->kind();
	
	
	if((disabled) && (msgKind != APP_NODE_STARTUP))
	{
		delete msg;
		msg = NULL;		// safeguard	
		return;
	}
	
	
	switch (msgKind) 
	{
		/*--------------------------------------------------------------------------------------------------------------
		 * Sent by ourself in order to start running the Application.
		 *--------------------------------------------------------------------------------------------------------------*/
		case APP_NODE_STARTUP:
		{
			disabled = 0;
			
			if (isSink) {
			    scheduleAt(simTime(), new App_ControlMessage("Application self message (reprogram nodes)", APP_TIMER_1));
			} else {
			    scheduleAt(simTime(), new App_ControlMessage("Application self message (request sample)", APP_SELF_REQUEST_SAMPLE));
			}
			
			/***
				PLACE HERE THE CODE FOR STARTING-UP/SCHEDULING THE APPLICATION TIMERS
			 
			 ******* EXAMPLE  *****************
			  scheduleAt(simTime(), new App_ControlMessage("timer 1", APP_TIMER_1));
			 **********************************************************************/

			break;
		}
		
		
		case APP_SELF_REQUEST_SAMPLE:
		{
			requestSampleFromSensorManager();
		
			// schedule a message (sent to ourself) to request a new sample
			scheduleAt(simTime()+DRIFTED_TIME(reportInterval), new App_ControlMessage("Application self message (request sample)", APP_SELF_REQUEST_SAMPLE));
			
			break;
		}
		
		
		case APP_DATA_PACKET:
		{
			BridgeTest_DataPacket *rcvPacket;
			rcvPacket = check_and_cast<BridgeTest_DataPacket *>(msg);
			
			string msgSender(rcvPacket->getHeader().source.c_str());
			string msgDestination(rcvPacket->getHeader().destination.c_str());
			string packetType(rcvPacket->getHeader().applicationID.c_str());
			
			int theData = rcvPacket->getData();
			int sequenceNumber = rcvPacket->getHeader().seqNumber;
			double rssi = rcvPacket->getRssi();
			string pathFromSource(rcvPacket->getCurrentPathFromSource());

			/**
		   	    ADD HERE YOUR CODE FOR HANDLING A RECEIVED DATA PACKET FROM THE NETWORK
			    
			    SENDER:       msgSender
			    DESTINATION:  msgDestination
			    DATA:         theData
			    SEQUENCE NUMBER: sequenceNumber
			    RSSI(dBm):    rssi
			**/
		
			if (packetType.compare(REPORT_PACKET) == 0) {
			    if (isSink || broadcastReports) {
				if (updateReportTable(atoi(msgSender.c_str()),sequenceNumber) && broadcastReports) {
				    BridgeTest_DataPacket *dupPacket;
				    dupPacket = check_and_cast<BridgeTest_DataPacket *>(rcvPacket->dup());
				    send(dupPacket, "toCommunicationModule");
				}
			    }
			} else if (packetType.compare(VERSION_PACKET) == 0) {
			    if (!isSink && updateVersionTable(theData,sequenceNumber)) {
				send2NetworkDataPacket(BROADCAST,VERSION_PACKET,theData,sequenceNumber,maxAppPacketSize-packetHeaderOverhead);
			    }
			} else {
			    CASTALIA_DEBUG << "\n[Application_"<< self <<"] t= " << simTime() << ": unknown packet type " << packetType;
			}

			break;
		}

		
		/*--------------------------------------------------------------------------------------------------------------
		 * Received whenever application timer 1 expires
		 *--------------------------------------------------------------------------------------------------------------*/
		case APP_TIMER_1:
		{
			/***
			 ***   ADD YOUR CODE HERE (optional)
			 ***/
			
			//Example : how to broadcast a value
			/*
			 *  int data2send = 10000;
			 *  send2NetworkDataPacket(BROADCAST_ADDR, data2send, 1);
			 */
			currentVersion++;
			currentVersionPacket = 0;
			scheduleAt(simTime() + DRIFTED_TIME(REPROGRAM_INTERVAL), new App_ControlMessage("Application self message (reprogram nodes)", APP_TIMER_1));
			scheduleAt(simTime(), new App_ControlMessage("Application self message (send reprogram packet)", APP_TIMER_2));
			break;
		}
				
		/*--------------------------------------------------------------------------------------------------------------
		 * Received whenever application timer 2 expires
		 *--------------------------------------------------------------------------------------------------------------*/
		case APP_TIMER_2:
		{
			/***
			 ***   ADD YOUR CODE HERE (optional)
			 ***/
			send2NetworkDataPacket(BROADCAST,VERSION_PACKET,currentVersion,currentVersionPacket,maxAppPacketSize-packetHeaderOverhead);
			currentVersionPacket++;
			if (currentVersionPacket < totalVersionPackets) {
			    scheduleAt(simTime() + DRIFTED_TIME(REPROGRAM_PACKET_DELAY), new App_ControlMessage("Application self message (send reprogram packet)", APP_TIMER_2));
			}
			break;
		}

		/*--------------------------------------------------------------------------------------------------------------
		 * Received whenever application timer 3 expires
		 *--------------------------------------------------------------------------------------------------------------*/
		case APP_TIMER_3:
		{
			/***
			 ***   ADD YOUR CODE HERE (optional)
			 ***/
			break;
		}

		/*--------------------------------------------------------------------------------------------------------------
		 * There are 5 more timers messages defined in the App_ControlMessage.msg file, (APP_TIMER_4 to APP_TIMER_8). If you need more
		 * than 3 timers (up to 8) simply put more case statements here.
		 *--------------------------------------------------------------------------------------------------------------*/
		
		
		
		case SDM_2_APP_SENSOR_READING:
		{
			SensorDevMgr_GenericMessage *rcvReading;
			rcvReading = check_and_cast<SensorDevMgr_GenericMessage*>(msg);
			
			int sensIndex =  rcvReading->getSensorIndex();
			string sensType(rcvReading->getSensorType());
			double sensValue = rcvReading->getSensedValue();

			/**
		   	    ADD HERE YOUR CODE FOR HANDLING A NEW SAMPLE FORM THE SENSOR
			    
			    SENSED VALUE: sensValue
			    SENSOR TYPE:  sensType
			    SENSOR INDEX: sensIndex
			**/
			
			if (sensValue < reportTreshold) break;
			if (isSink) {
			    CASTALIA_DEBUG << "\n[Application_"<< self <<"] t= " << simTime() << ": Sink recieved SENSOR_READING (while it shouldnt) " << sensValue << " (int)"<<(int)sensValue;
			}
			
			if (broadcastReports) {
			    send2NetworkDataPacket(BROADCAST, REPORT_PACKET, (int)sensValue, currSentSampleSN, packetHeaderOverhead+6);
			    currSentSampleSN++;
			} else if (routingLevel != NO_ROUTING_LEVEL) {
			    send2NetworkDataPacket(SINK, REPORT_PACKET, (int)sensValue, currSentSampleSN, packetHeaderOverhead+6);
			    currSentSampleSN++;
			} else {
			    CASTALIA_DEBUG << "\n[Application_"<< self <<"] t= " << simTime() << ": unable to report: " << sensValue << " (int)"<<(int)sensValue;
			}
			break;
		}
		
		
		/*--------------------------------------------------------------------------------------------------------------
		 * 
		 *--------------------------------------------------------------------------------------------------------------*/
		case RESOURCE_MGR_OUT_OF_ENERGY:
		{
			disabled = 1;
			outOfEnergy = simTime();
			CASTALIA_DEBUG << "\n[Application_"<< self <<"] t= " << simTime() << ": out of energy, will try to finish";
		    
//			parentModule()->callFinish();			    

			break;
		}
		
		
		/*--------------------------------------------------------------------------------------------------------------
		 * Message received by the ResourceManager module. It commands the module to stop its operation.
		 *--------------------------------------------------------------------------------------------------------------*/
		case RESOURCE_MGR_DESTROY_NODE:
		{
			disabled = 1;
			break;
		}
		
		
		case NETWORK_2_APP_FULL_BUFFER:
		{
			/*
			 * TODO:
			 * add your code here to manage the situation of a full Network/Routing buffer.
			 * Apparently we 'll have to stop sending messages and enter into listen or sleep mode (depending on the Application protocol that we implement).
			 */
			 CASTALIA_DEBUG << "\n[Application_"<< self <<"] t= " << simTime() << ": WARNING: NETWORK_2_APP_FULL_BUFFER received because the Network buffer is full.\n";
			 break;
		}
		
		
		case NETWORK_2_APP_TREE_LEVEL_UPDATED:
	    {
	    	// this message notifies the application of a cartan routing state (level)
	    	// for certain routing protocols.
	    	Network_ControlMessage *levelMsg = check_and_cast<Network_ControlMessage *>(msg);
	    	
	    	routingLevel = levelMsg->getLevel();
	    	
	    	break;
	    }
	    
	    
	    case NETWORK_2_APP_CONNECTED_2_TREE:
	    {
	    	Network_ControlMessage *connectedMsg = check_and_cast<Network_ControlMessage *>(msg);
	    	
	    	routingLevel = connectedMsg->getLevel();
	    	int sinkID = connectedMsg->getSinkID();
	    	string parents;
	    	parents.assign(connectedMsg->getParents());
	    	
	    	
	    	break;
	    }
	    
	    
	    case NETWORK_2_APP_NOT_CONNECTED:
	    {
	    	break;
	    }
		
		
		default:
		{
			CASTALIA_DEBUG << "\\n[Application_"<< self <<"] t= " << simTime() << ": WARNING: received packet of unknown type";
			break;
		}
	}
	
	delete msg;
	msg = NULL;		// safeguard
}




void BridgeTest_ApplicationModule::finish()
{
	// output the spent energy of the node
	EV <<  "Node [" << self << "] spent energy: " << resMgrModule->getSpentEnergy() << "\n";
	if (isSink) {
	    EV << "Sink is at version: " << currentVersion << " packet: " << currentVersionPacket << " total: " << totalVersionPackets << "\n";
	    EV << "Sink received from:\n";
	    for (int i=0; i<(int)report_info_table.size(); i++) {
		EV << report_info_table[i].source << " " << report_info_table[i].parts.size() << "\n";
	    }
	} else {
	    if (outOfEnergy > 0) EV << "Node "<<self<<" ran out of energy at "<<outOfEnergy<<"\n";
	    EV << "Node "<<self<<" sent "<<currSentSampleSN<<" packets\n";
	    EV << "Node "<<self<<" received version information:\n";
	    for (int i=0; i<(int)version_info_table.size(); i++) {
		EV << version_info_table[i].version << " " << version_info_table[i].parts.size() << "\n";
	    }
	}
	//close the output stream that CASTALIA_DEBUG is writing to
	DebugInfoWriter::closeStream();
}


/***
    Based on the custom message definition *.msg file of your application the second parameter of the following function might need
    to have a different type. The type should be the same as in your *.msg file data field.
***/
void BridgeTest_ApplicationModule::send2NetworkDataPacket(const char *destID, const char *pcktID, int data, int pckSeqNumber, int size)
{
	if(!disabled) {
		BridgeTest_DataPacket *packet2Net;
		packet2Net = new BridgeTest_DataPacket("Application Packet Application->Mac", APP_DATA_PACKET);
		packet2Net->setData(data);
		packet2Net->getHeader().applicationID = pcktID;
		packet2Net->getHeader().source = selfAddr;
		packet2Net->getHeader().destination = destID;
		packet2Net->getHeader().seqNumber = pckSeqNumber;
		packet2Net->setByteLength(size);
		send(packet2Net, "toCommunicationModule");
	}
}


void BridgeTest_ApplicationModule::requestSampleFromSensorManager()
{
	// send the request message to the Sensor Device Manager
	SensorDevMgr_GenericMessage *reqMsg;
	reqMsg = new SensorDevMgr_GenericMessage("app 2 sensor dev manager (Sample request)", APP_2_SDM_SAMPLE_REQUEST);
	reqMsg->setSensorIndex(0); //we need the index of the vector in the sensorTypes vector to distinguish the self messages for each sensor 
	send(reqMsg, "toSensorDeviceManager");
	
}

int BridgeTest_ApplicationModule::updateReportTable(int src, int seq) {
    int pos = -1;
    for (int i=0; i<(int)report_info_table.size(); i++) {
	if (report_info_table[i].source == src) pos = i;
    }
    
    if (pos == -1) {
	report_info newInfo;
	newInfo.source = src;
	newInfo.parts.clear();
	newInfo.parts.push_back(seq);
	report_info_table.push_back(newInfo);
    } else {
	for (int i=0; i<(int)report_info_table[pos].parts.size(); i++) {
	    if (report_info_table[pos].parts[i] == seq) return 0;
	}
	report_info_table[pos].parts.push_back(seq);
    }
    return 1;
}

int BridgeTest_ApplicationModule::updateVersionTable(int version, int seq) {
    int pos = -1;
    for (int i=0; i<(int)version_info_table.size(); i++) {
	if (version_info_table[i].version == version) pos = i;
    }
    
    if (pos == -1) {
	version_info newInfo;
	newInfo.version = version;
	newInfo.parts.clear();
	newInfo.parts.push_back(seq);
	version_info_table.push_back(newInfo);
    } else {
	for (int i=0; i<(int)version_info_table[pos].parts.size(); i++) {
	    if (version_info_table[pos].parts[i] == seq) return 0;
	}
	version_info_table[pos].parts.push_back(seq);
    }
    return 1;
}
