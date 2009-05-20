//***************************************************************************************
//*  Copyright: Athens Information Technology (AIT),  2007, 2008, 2009			*
//*		http://www.ait.gr							*
//*             Developed at the Broadband Wireless and Sensor Networks group (B-WiSe) 	*
//*		http://www.ait.edu.gr/research/Wireless_and_Sensors/overview.asp	*
//*											*
//*  Author(s): Dimosthenis Pediaditakis						*
//*											*
//*  This file is distributed under the terms in the attached LICENSE file.		*
//*  If you do not find this file, copies can be found by writing to:			*
//*											*
//*      NICTA, Locked Bag 9013, Alexandria, NSW 1435, Australia			*
//*      Attention:  License Inquiry.							*
//**************************************************************************************/



#include "valueReporting_ApplicationModule.h"

#define EV   ev.disabled()?(ostream&)ev:ev

#define CASTALIA_DEBUG (!printDebugInfo)?(ostream&)DebugInfoWriter::getStream():DebugInfoWriter::getStream()

#define DRIFTED_TIME(time) ((time) * cpuClockDrift)

#define MEMORY_PROFILE_STORE(numBytes) resMgrModule->RamStore(numBytes)

#define MEMORY_PROFILE_FREE(numBytes) resMgrModule->RamFree(numBytes)

#define STARTUP_DELAY 0.05

#define CIRCUITS_PREPARE_DELAY 0.001


#define NO_ROUTING_LEVEL -1


/************ Implementation Functions of TunableMAC set functions ******************************/
//  If you are not going to use the TunableMAC module, then you can comment the following line and build Castalia
#include "valueReporting_radioControlImpl.cc.inc"
#include "valueReporting_tunableMacControlImpl.cc.inc"
/************ Connectivity Map Definitions  ************************************************/


Define_Module(valueReporting_ApplicationModule);



void valueReporting_ApplicationModule::initialize() 
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
	 
	isSink = par("isSink");
	
	maxSampleInterval = ((double)par("maxSampleInterval"))/1000.0;
	
	minSampleInterval = ((double)par("minSampleInterval"))/1000.0;
	
	app_timer_A = NULL;
	
	routingLevel = (isSink)?0:NO_ROUTING_LEVEL;
	
	lastSensedValue = 0.0;
	
	currSentSampleSN = 0;
	
	randomBackoffIntervalFraction = genk_dblrand(0);
	
	sentOnce = false;
}


void valueReporting_ApplicationModule::handleMessage(cMessage *msg)
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

			scheduleAt(simTime()+STARTUP_DELAY+DRIFTED_TIME(maxSampleInterval)*randomBackoffIntervalFraction, new App_ControlMessage("Application self message (request sample)", APP_SELF_REQUEST_SAMPLE));
			
			break;
		}
		
		
		case APP_SELF_REQUEST_SAMPLE:
		{
			requestSampleFromSensorManager();
		
			// schedule a message (sent to ourself) to request a new sample
			scheduleAt(simTime()+DRIFTED_TIME(maxSampleInterval), new App_ControlMessage("Application self message (request sample)", APP_SELF_REQUEST_SAMPLE));
			
			break;
		}
		
		
		case APP_DATA_PACKET:
		{
			valueReporting_DataPacket *rcvPacket;
			rcvPacket = check_and_cast<valueReporting_DataPacket *>(msg);
			
			string msgSender(rcvPacket->getHeader().source.c_str());
			string msgDestination(rcvPacket->getHeader().destination.c_str());
			
			valueReportData theData = rcvPacket->getData();
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
			
			if(isSink)	
		    		CASTALIA_DEBUG << "\n T=" << simTime() << ",  Sink(Node[" << self << "]) \treceived from:" << theData.nodeID << " \tvalue=" << theData.value;

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
			
			if ( (routingLevel != NO_ROUTING_LEVEL) &&  (!isSink) )
			{
				valueReportData tmpData;
				tmpData.nodeID = (unsigned short)self;
				tmpData.locX = self_xCoo;
				tmpData.locY = self_yCoo;
				tmpData.sampleSN = currSentSampleSN;
				tmpData.value = lastSensedValue;
				
				//if(!sentOnce)
				{	
					send2NetworkDataPacket(SINK, tmpData, 0);
					sentOnce = true;
				}
			}
				
			app_timer_A = NULL;
			
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
			
			
			lastSensedValue = sensValue;
			
			// schedule the TX of the value
    		if( (app_timer_A != NULL) && (app_timer_A->isScheduled()) )
    			cancelAndDelete(app_timer_A);
    		app_timer_A = new App_ControlMessage("timer 1", APP_TIMER_1);    		
    		scheduleAt(simTime(), app_timer_A);
			
			
			CASTALIA_DEBUG << "\n[App_"<< self <<"] t= " << simTime() << ": Sensed = " << sensValue;
			

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
			 CASTALIA_DEBUG << "\n[Application_"<< self <<"] t= " << simTime() << ": WARNING: NETWORK_2_APP_FULL_BUFFER received because the Network buffer is full.\n";
			 break;
		}
		
		
		case NETWORK_2_APP_TREE_LEVEL_UPDATED:
	    {
	    	// this message notifies the application of a cartan routing state (level)
	    	// for certain routing protocols.
	    	Network_ControlMessage *levelMsg = check_and_cast<Network_ControlMessage *>(msg);
	    	
	    	routingLevel = levelMsg->getLevel();
	    	
	    	CASTALIA_DEBUG << "\n[App_"<< self <<"] t= " << simTime() << ": Level " << routingLevel ;
	    	
	    	break;
	    }
	    
	    
	    case NETWORK_2_APP_CONNECTED_2_TREE:
	    {
	    	Network_ControlMessage *connectedMsg = check_and_cast<Network_ControlMessage *>(msg);
	    	
	    	routingLevel = connectedMsg->getLevel();
	    	int sinkID = connectedMsg->getSinkID();
	    	string parents;
	    	parents.assign(connectedMsg->getParents());
	    	
	    	CASTALIA_DEBUG << "\n[App_"<< self <<"] t= " << simTime() << ": Level " << routingLevel << ", Parent " << parents ;
	    	
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




void valueReporting_ApplicationModule::finish()
{
	// output the spent energy of the node
	EV <<  "Node [" << self << "] spent energy: " << resMgrModule->getSpentEnergy() << "\n";
	
	//close the output stream that CASTALIA_DEBUG is writing to
	DebugInfoWriter::closeStream();
}


/***
    Based on the custom message definition *.msg file of your application the second parameter of the following function might need
    to have a different type. The type should be the same as in your *.msg file data field.
***/
void valueReporting_ApplicationModule::send2NetworkDataPacket(const char *destID, valueReportData data, int pckSeqNumber)
{
	if(!disabled)
	{
		valueReporting_DataPacket *packet2Net;
		packet2Net = new valueReporting_DataPacket("Application Packet Application->Mac", APP_DATA_PACKET);
		
		packet2Net->setData(data);
		
		packet2Net->getHeader().applicationID = applicationID.c_str();
		
		char tmpAddr[256];
		sprintf(tmpAddr, "%i", self);
		packet2Net->getHeader().source = tmpAddr;
		
		packet2Net->getHeader().destination = destID;
		
		packet2Net->getHeader().seqNumber = pckSeqNumber;

		if(constantDataPayload != 0)
			packet2Net->setByteLength(constantDataPayload + packetHeaderOverhead);
		else
			packet2Net->setByteLength(sizeof(data) + packetHeaderOverhead);
		
		send(packet2Net, "toCommunicationModule");
	}
}


void valueReporting_ApplicationModule::requestSampleFromSensorManager()
{
	// send the request message to the Sensor Device Manager
	SensorDevMgr_GenericMessage *reqMsg;
	reqMsg = new SensorDevMgr_GenericMessage("app 2 sensor dev manager (Sample request)", APP_2_SDM_SAMPLE_REQUEST);
	reqMsg->setSensorIndex(0); //we need the index of the vector in the sensorTypes vector to distinguish the self messages for each sensor 
	send(reqMsg, "toSensorDeviceManager");
	
}

