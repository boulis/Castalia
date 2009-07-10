//***************************************************************************************
//*  Copyright: National ICT Australia,  2007, 2008, 2009				*
//*  Developed at the Networks and Pervasive Computing program				*
//*  Author(s): Athanassios Boulis, Dimosthenis Pediaditakis				*
//*  This file is distributed under the terms in the attached LICENSE file.		*
//*  If you do not find this file, copies can be found by writing to:			*
//*											*
//*      NICTA, Locked Bag 9013, Alexandria, NSW 1435, Australia			*
//*      Attention:  License Inquiry.							*
//*											*
//***************************************************************************************


#include "throughputTest_ApplicationModule.h"

#define EV   ev.disabled()?(ostream&)ev:ev

#define CASTALIA_DEBUG (!printDebugInfo)?(ostream&)DebugInfoWriter::getStream():DebugInfoWriter::getStream()

#define DRIFTED_TIME(time) ((time) * cpuClockDrift)

#define MEMORY_PROFILE_STORE(numBytes) resMgrModule->RamStore(numBytes)

#define MEMORY_PROFILE_FREE(numBytes) resMgrModule->RamFree(numBytes)

#define STARTUP_DELAY 0.05

#define APP_SAMPLE_INTERVAL 0.5

#define CIRCUITS_PREPARE_DELAY 0.001


/************ Implementation Functions of TunableMAC set functions ******************************/
//  If you are not going to use the TunableMAC module, then you can comment the following line and build Castalia
#include "throughputTest_radioControlImpl.cc.inc"
#include "throughputTest_tunableMacControlImpl.cc.inc"
/************ Connectivity Map Definitions  ************************************************/


Define_Module(throughputTest_ApplicationModule);



void throughputTest_ApplicationModule::initialize()
{
	self = parentModule()->index();

	self_xCoo = parentModule()->par("xCoor");

	self_yCoo = parentModule()->par("yCoor");

	//get a valid reference to the object of the Resources Manager module so that we can make direct calls to its public methods
	//instead of using extra messages & message types for tighlty couplped operations.
	cModule *parent = parentModule();
	if(parent->findSubmodule("nodeResourceMgr") != -1)
	{
		resMgrModule = check_and_cast<ResourceGenericManager*>(parent->submodule("nodeResourceMgr"));
	}
	else
		opp_error("\n[Application]:\n Error in geting a valid reference to  nodeResourceMgr for direct method calls.");
	cpuClockDrift = resMgrModule->getCPUClockDrift();

	disabled = 1;

	const char * tmpID = par("applicationID");
	applicationID.assign(tmpID);

	printDebugInfo = par("printDebugInfo");

	priority = par("priority");

	maxAppPacketSize = par("maxAppPacketSize");

	packetHeaderOverhead = par("packetHeaderOverhead");

	constantDataPayload = par("constantDataPayload");

	packet_rate = hasPar("packet_rate") ? par("packet_rate") : 0;
	
	nextRecipient = hasPar("nextRecipient") ? par("nextRecipient") : 0;
	
	sprintf(selfAddr, "%i", self);
	sprintf(dstAddr, "%i", nextRecipient);

	char buff[30];
	sprintf(buff, "Application Vector of Node %d", self);
	appVector.setName(buff);

	// Send the STARTUP message to MAC & to Sensor_Manager modules so that the node start to operate. (after a random delay, because we don't want the nodes to be synchronized)
	double random_startup_delay = genk_dblrand(0) * STARTUP_DELAY + CIRCUITS_PREPARE_DELAY;
	sendDelayed(new App_ControlMessage("Application --> Sensor Dev Mgr [STARTUP]", APP_NODE_STARTUP), simTime() + DRIFTED_TIME(random_startup_delay), "toSensorDeviceManager");
	sendDelayed(new App_ControlMessage("Application --> Network [STARTUP]", APP_NODE_STARTUP), simTime() + DRIFTED_TIME(random_startup_delay), "toCommunicationModule");
	scheduleAt(simTime() + DRIFTED_TIME(random_startup_delay), new App_ControlMessage("Application --> Application (self)", APP_NODE_STARTUP));



	/**
	  ADD HERE INITIALIZATION CODE HERE FOR YOUR APPLICATION
	 **/
	packet_spacing = packet_rate > 0 ? 1/float(packet_rate) : -1;
	packet_info_table.clear();
	total_packets_received = 0;
	packets_lost_at_mac = 0;
	packets_lost_at_network = 0;
	dataSN = 0;
}


void throughputTest_ApplicationModule::handleMessage(cMessage *msg)
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

			//scheduleAt(simTime(), new App_ControlMessage("Application self message (request sample)", APP_SELF_REQUEST_SAMPLE));

			/***
				PLACE HERE THE CODE FOR STARTING-UP/SCHEDULING THE APPLICATION TIMERS

			 ******* EXAMPLE  *****************
			  scheduleAt(simTime(), new App_ControlMessage("timer 1", APP_TIMER_1));
			 **********************************************************************/
			if (packet_spacing > 0 && self != nextRecipient) {
			    scheduleAt(simTime() + DRIFTED_TIME(packet_spacing), new App_ControlMessage("timer 1", APP_TIMER_1));
			}

			break;
		}


		case APP_SELF_REQUEST_SAMPLE:
		{
			requestSampleFromSensorManager();

			// schedule a message (sent to ourself) to request a new sample
			scheduleAt(simTime()+DRIFTED_TIME(APP_SAMPLE_INTERVAL), new App_ControlMessage("Application self message (request sample)", APP_SELF_REQUEST_SAMPLE));

			break;
		}


		case APP_DATA_PACKET:
		{
			throughputTest_DataPacket *rcvPacket;
			rcvPacket = check_and_cast<throughputTest_DataPacket *>(msg);

			string msgSender(rcvPacket->getHeader().source.c_str());
			string msgDestination(rcvPacket->getHeader().destination.c_str());

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
			
			if (nextRecipient == self) {
			    update_packets_received(atoi(msgSender.c_str()),sequenceNumber);
			} else {
			    throughputTest_DataPacket *dupPacket = check_and_cast<throughputTest_DataPacket *>(rcvPacket->dup());
			    dupPacket->getHeader().destination = dstAddr;
			    send(dupPacket,"toCommunicationModule");
			}

			CASTALIA_DEBUG << "[APP_" << self << "] Packet received from node "<<msgSender<<" at time "<<simTime() << "\n";

			break;
		}


		/*--------------------------------------------------------------------------------------------------------------
		 * Received whenever application timer 1 expires
		 *--------------------------------------------------------------------------------------------------------------*/
		case APP_TIMER_1:
		{
		    send2NetworkDataPacket(dstAddr,10000,dataSN);
		    dataSN++;
    		    scheduleAt(simTime() + DRIFTED_TIME(packet_spacing), new App_ControlMessage("timer1", APP_TIMER_1));
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

			break;
		}


		/*--------------------------------------------------------------------------------------------------------------
		 *
		 *--------------------------------------------------------------------------------------------------------------*/
		case RESOURCE_MGR_OUT_OF_ENERGY:
		{
			disabled = 1;
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
			packets_lost_at_network++;
			CASTALIA_DEBUG << "\n[Application_"<< self <<"] t= " << simTime() << ": WARNING: NETWORK_2_APP_FULL_BUFFER received because the Network buffer is full.\n";
			break;
		}
		
		
		case MAC_2_APP_FULL_BUFFER:
		{
			packets_lost_at_mac++;
			CASTALIA_DEBUG << "\n[Application_"<< self <<"] t= " << simTime() << ": WARNING: MAC_2_APP_FULL_BUFFER received because the MAC buffer is full.\n";
			break;
		}


		case NETWORK_2_APP_TREE_LEVEL_UPDATED:
	    {
	    	// this message notifies the application of a cartan routing state (level)
	    	// for certain routing protocols.
	    	Network_ControlMessage *levelMsg = check_and_cast<Network_ControlMessage *>(msg);

	    	//int routingLevel = levelMsg->getLevel();

	    	break;
	    }


	    case NETWORK_2_APP_CONNECTED_2_TREE:
	    {
	    	Network_ControlMessage *connectedMsg = check_and_cast<Network_ControlMessage *>(msg);

	    	int routingLevel = connectedMsg->getLevel();
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




void throughputTest_ApplicationModule::finish()
{
	// print out the number of packets received from each node by node 0, and also the total number
	if(self == nextRecipient)
	{
		EV << "\n\n** Node [" << self << "] received from:\n";
		for(map<int,packet_info>::iterator i = packet_info_table.begin();
		    i != packet_info_table.end(); i++) {
			EV << "[" << self << "<--" << i->first << "]\t -->\t " << i->second.packets_received.size() << "\n";
			total_packets_received += i->second.packets_received.size();
		}
		EV << "total number of packets received is: " << total_packets_received << "\n";
	}

	// output the spent energy of the node
	EV << "Node [" << self << "] spent energy: " << resMgrModule->getSpentEnergy() << "\n";
	if (packets_lost_at_network || packets_lost_at_mac) {
	    EV << "Node [" << self << "] lost packets due to buffer overflow: " << packets_lost_at_mac << "(MAC), " << packets_lost_at_network << "(Network)\n";
	}

	//close the output stream that CASTALIA_DEBUG is writing to
	DebugInfoWriter::closeStream();
}


/***
    Based on the custom message definition *.msg file of your application the second parameter of the following function might need
    to have a different type. The type should be the same as in your *.msg file data field.
***/
void throughputTest_ApplicationModule::send2NetworkDataPacket(const char *destID, int data, int pckSeqNumber)
{
	if(!disabled)
	{
		throughputTest_DataPacket *packet2Net;
		packet2Net = new throughputTest_DataPacket("Application Packet Application->Mac", APP_DATA_PACKET);
		packet2Net->setData(data);

		packet2Net->getHeader().applicationID = applicationID.c_str();

		packet2Net->getHeader().source = selfAddr;

		packet2Net->getHeader().destination = destID;

		packet2Net->getHeader().seqNumber = pckSeqNumber;

		if(constantDataPayload > 0)
			packet2Net->setByteLength(constantDataPayload + packetHeaderOverhead);
		else
			packet2Net->setByteLength(sizeof(data) + packetHeaderOverhead);

		send(packet2Net, "toCommunicationModule");
	}
}


void throughputTest_ApplicationModule::requestSampleFromSensorManager()
{
	// send the request message to the Sensor Device Manager
	SensorDevMgr_GenericMessage *reqMsg;
	reqMsg = new SensorDevMgr_GenericMessage("app 2 sensor dev manager (Sample request)", APP_2_SDM_SAMPLE_REQUEST);
	reqMsg->setSensorIndex(0); //we need the index of the vector in the sensorTypes vector to distinguish the self messages for each sensor
	send(reqMsg, "toSensorDeviceManager");

}


// this method updates the number of packets received by node 0 from other nodes
void throughputTest_ApplicationModule::update_packets_received(int srcID, int SN)
{
    packet_info_table[srcID].packets_received[SN]++;
}




