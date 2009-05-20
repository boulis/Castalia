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



#include "valuePropagation_ApplicationModule.h"

#define EV   ev.disabled()?(ostream&)ev:ev

#define CASTALIA_DEBUG (!printDebugInfo)?(ostream&)DebugInfoWriter::getStream():DebugInfoWriter::getStream()

#define DRIFTED_TIME(time) ((time) * cpuClockDrift)

#define MEMORY_PROFILE_STORE(numBytes) resMgrModule->RamStore(numBytes)

#define MEMORY_PROFILE_FREE(numBytes) resMgrModule->RamFree(numBytes)

#define STARTUP_DELAY 0.05

#define APP_SAMPLE_INTERVAL 0.5

#define CIRCUITS_PREPARE_DELAY 0.001

#define ENERGY_TRACK_CYCLE 1.0

#define TEMP_THRESHOLD 15.0



/************ Implementation Functions of TunableMAC set functions ******************************/
// If you are not going to use the TunableMAC module, then you can comment the following line and build Castalia
#include "valuePropagation_radioControlImpl.cc.inc"
#include "valuePropagation_tunableMacControlImpl.cc.inc"
/************ Connectivity Map Definitions  ************************************************/



Define_Module(valuePropagation_ApplicationModule);



void valuePropagation_ApplicationModule::initialize() 
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
		opp_error("\n[App]:\n Error in geting a valid reference to  nodeResourceMgr for direct method calls.");
	cpuClockDrift = resMgrModule->getCPUClockDrift();
	
	disabled = 1;
	
	const char * tmpID = par("applicationID");
	applicationID.assign(tmpID);
	
	printDebugInfo = par("printDebugInfo");
	
	priority = par("priority");
	
	maxAppPacketSize = par("maxAppPacketSize");
	
	packetHeaderOverhead = par("packetHeaderOverhead");
	
	constantDataPayload = par("constantDataPayload");
	
	totalPackets = 0;
	
	currMaxReceivedValue = -1.0;
	currMaxSensedValue = -1.0;
	
	char buff[30];
	sprintf(buff, "Application Vector of Node %d", self);
	appVector.setName(buff);
	
	// Send the STARTUP message to MAC & to Sensor_Manager modules so that the node start to operate. (after a random delay, because we don't want the nodes to be synchronized)
	
	double random_startup_delay = genk_dblrand(0) * STARTUP_DELAY + CIRCUITS_PREPARE_DELAY;
	sendDelayed(new App_ControlMessage("Application --> Sensor Dev Mgr [STARTUP]", APP_NODE_STARTUP), simTime() + DRIFTED_TIME(random_startup_delay), "toSensorDeviceManager");
	sendDelayed(new App_ControlMessage("Application --> Network [STARTUP]", APP_NODE_STARTUP), simTime() + DRIFTED_TIME(random_startup_delay), "toCommunicationModule");
	scheduleAt(simTime() + DRIFTED_TIME(random_startup_delay), new App_ControlMessage("Application --> Application (self)", APP_NODE_STARTUP));
	
	
	
	/****
	 ****  ADD INITIALIZATION CODE HERE FOR YOUR APPLICATION  ****
	 ****/
	sentOnce = 0;
	theValue = 0;
	
	sensedValues.clear();
}


void valuePropagation_ApplicationModule::handleMessage(cMessage *msg)
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
			
			scheduleAt(simTime()+STARTUP_DELAY/* to avoid missed receptions due to not yet started-up nodes */, new App_ControlMessage("Application self message (request sample)", APP_SELF_REQUEST_SAMPLE));
			
			/******* Start here the timers that you plan to use  *****************/
			scheduleAt(simTime(), new App_ControlMessage("timer 1", APP_TIMER_1));

			break;
		}
		
		
		case APP_SELF_REQUEST_SAMPLE:
		{
			requestSampleFromSensorManager();
		
			// schedule a message (sent to ourself) to request a new sample
			// scheduleAt(simTime()+DRIFTED_TIME(APP_SAMPLE_INTERVAL), new App_ControlMessage("Application self message (request sample)", APP_SELF_REQUEST_SAMPLE));
			
			break;
		}
		
		
		case APP_DATA_PACKET:
		{
			valuePropagation_DataPacket *rcvPacket;
			rcvPacket = check_and_cast<valuePropagation_DataPacket *>(msg);
			
			string msgSender(rcvPacket->getHeader().source.c_str());
			string msgDestination(rcvPacket->getHeader().destination.c_str());
			double theData = rcvPacket->getData();
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
			
			/***************************************/
			onReceiveMessage(atoi(msgSender.c_str()), theData);
			/***************************************/
			break;
		}
		

		/*--------------------------------------------------------------------------------------------------------------
		 * Received whenever application timer 1 expires
		 *--------------------------------------------------------------------------------------------------------------*/
		case APP_TIMER_1:
		{
			
			// Just keep track of the remaining energy every one second
            CASTALIA_DEBUG << "\n[Energy_"<< self <<"] t= " << simTime() << ": " << resMgrModule->getSpentEnergy();
			
			scheduleAt(simTime()+ENERGY_TRACK_CYCLE, new App_ControlMessage("timer 1", APP_TIMER_1));

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
			/***************************************/
			onSensorReading(sensIndex, sensType, sensValue);
			/***************************************/
			break;
		}
		
		
		/*--------------------------------------------------------------------------------------------------------------
		 * 
		 *--------------------------------------------------------------------------------------------------------------*/
		case RESOURCE_MGR_OUT_OF_ENERGY:
		{
			disabled = 1;
    			CASTALIA_DEBUG << "\n[App_"<< self <<"] t= " << simTime() << ": WILL TRY TO FINISH\n";
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
			 CASTALIA_DEBUG << "\n[App_"<< self <<"] t= " << simTime() << ": WARNING: NETWORK_2_APP_FULL_BUFFER received because the Network buffer is full.\n";
			 break;
		}
		
		
		default:
		{
			CASTALIA_DEBUG << "\n[App_"<< self <<"] t= " << simTime() << ": WARNING: received packet of unknown type";
			break;
		}
	}
	
	delete msg;
	msg = NULL;		// safeguard
}


void valuePropagation_ApplicationModule::finish()
{	
	// output the value that the node has when the simulation stopped
	EV <<  "Node [" << self << "] Value: " << theValue << "\n";
	
	// output the spent energy of the node
	EV <<  "Node [" << self << "] spent energy: " << resMgrModule->getSpentEnergy() << "\n";
	
	sensedValues.clear();
	
	//close the output stream that CASTALIA_DEBUG is writing to
	DebugInfoWriter::closeStream();
}



void valuePropagation_ApplicationModule::send2NetworkDataPacket(const char *destID, double data, int pckSeqNumber)
{
	if(!disabled)
	{
		valuePropagation_DataPacket *packet2Net;
		packet2Net = new valuePropagation_DataPacket("Application Packet Application->Mac", APP_DATA_PACKET);
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
			packet2Net->setByteLength(sizeof(double) + packetHeaderOverhead);
		
		//TODO: here the user can control the size of the packet and break it into smaller fragments
		
		send(packet2Net, "toCommunicationModule");
	}
}


void valuePropagation_ApplicationModule::requestSampleFromSensorManager()
{
	// send the request message to the Sensor Device Manager
	SensorDevMgr_GenericMessage *reqMsg;
	reqMsg = new SensorDevMgr_GenericMessage("app 2 sensor dev manager (Sample request)", APP_2_SDM_SAMPLE_REQUEST);
	reqMsg->setSensorIndex(0); //we need the index of the vector in the sensorTypes vector to distinguish the self messages for each sensor 
	send(reqMsg, "toSensorDeviceManager");
}


void valuePropagation_ApplicationModule::onReceiveMessage(int senderID, double receivedData)
{
	/****
	 ****  ADD CODE HERE FOR PROCESSING THE RECEIVED MESSAGE  ****
	 ****/
	 totalPackets++;
	 
	if(receivedData > currMaxReceivedValue)
	 	currMaxReceivedValue = receivedData;
	
	
	if ((receivedData > TEMP_THRESHOLD) && !sentOnce)
	{
		theValue = receivedData;
		
		send2NetworkDataPacket(BROADCAST, receivedData, 1);

		sentOnce = 1;

		CASTALIA_DEBUG << "\n[App_"<< self <<"] t= " << simTime() << ": Got the value: " << theValue;
	}
}


void valuePropagation_ApplicationModule::onSensorReading(int sensorIndex, string sensorType, double sensedValue)
{
	/****
	 ****  ADD CODE HERE FOR PROCESSING THE NEW SENSOR READING  ****
	 ****/
	 
	if(sensedValue > currMaxSensedValue)
		currMaxSensedValue = sensedValue;
	
	if( (!sentOnce) && (sensedValue > TEMP_THRESHOLD) )
	{
		theValue = sensedValue;
		
		send2NetworkDataPacket(BROADCAST, sensedValue, 1);

		sentOnce = 1;
	}
	
	sensedValues.push_back(sensedValue);
	
}
