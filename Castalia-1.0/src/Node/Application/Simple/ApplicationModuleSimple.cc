/************************************************************************************
 *  Copyright: National ICT Australia,  2006										*
 *  Developed at the Networks and Pervasive Computing program						*
 *  Author(s): Athanassios Boulis, Dimosthenis Pediaditakis							*
 *  This file is distributed under the terms in the attached LICENSE file.			*
 *  If you do not find this file, copies can be found by writing to:				*
 *																					*
 *      NICTA, Locked Bag 9013, Alexandria, NSW 1435, Australia						*
 *      Attention:  License Inquiry.												*
 *																					*
 ************************************************************************************/


#include "ApplicationModuleSimple.h"

#define EV   ev.disabled() ? (ostream&)ev : ev

#define DRIFTED_TIME(time) ((time) * cpuClockDrift)

#define MEMORY_PROFILE_STORE(numBytes) resMgrModule->RamStore(numBytes)

#define MEMORY_PROFILE_FREE(numBytes) resMgrModule->RamFree(numBytes)

#define STARTUP_DELAY 0.05

#define APP_SAMPLE_INTERVAL 0.5

#define CIRCUITS_PREPARE_DELAY 0.001

#define TEMP_THRESHOLD 50

Define_Module(ApplicationModuleSimple);



void ApplicationModuleSimple::initialize() 
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
	
	priority = par("priority");
	
	maxAppPacketSize = par("maxAppPacketSize");
	
	packetHeaderOverhead = par("packetHeaderOverhead");
	
	totalPackets = 0;
	
	currMaxReceivedValue = -1;
	currMaxSensedValue = -1;
	
	char buff[30];
	sprintf(buff, "Application Vector of Node %d", self);
	appVector.setName(buff);
	
	// Send the STARTUP message to MAC & to Sensor_Manager modules so that the node start to operate. (after a random delay, because we don't want the nodes to be synchronized)
	double random_startup_delay = genk_dblrand(0) * STARTUP_DELAY;
	sendDelayed(new App_GenericDataPacket("Application --> Sensor Dev Mgr [STARTUP]", APP_NODE_STARTUP), simTime() + DRIFTED_TIME(random_startup_delay), "toSensorDeviceManager");
	sendDelayed(new App_GenericDataPacket("Application --> Mac [STARTUP]", APP_NODE_STARTUP), simTime() + DRIFTED_TIME(random_startup_delay), "toCommunicationModule");
	scheduleAt(simTime() + DRIFTED_TIME(random_startup_delay)+CIRCUITS_PREPARE_DELAY, new App_GenericDataPacket("Application --> Application (self)", APP_NODE_STARTUP));
	
	
	
	/****
	 ****  ADD INITIALIZATION CODE HERE FOR YOUR APPLICATION  ****
	 ****/
	sentOnce = 0;
	theValue = 0;
}


void ApplicationModuleSimple::handleMessage(cMessage *msg)
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
			
			scheduleAt(simTime(), new App_GenericDataPacket("Application self message (request sample)", APP_SELF_REQUEST_SAMPLE));
			
			break;
		}
		
		
		case APP_SELF_REQUEST_SAMPLE:
		{
			// send the request message to the Sensor Device Manager
			SensorDevMgr_GenericMessage *reqMsg;
			reqMsg = new SensorDevMgr_GenericMessage("app 2 sensor dev manager (Sample request)", APP_2_SDM_SAMPLE_REQUEST);
			reqMsg->setSensorIndex(0); //we need the index of the vector in the sensorTypes vector to distinguish the self messages for each sensor 
			send(reqMsg, "toSensorDeviceManager");
			
			// schedule a message (sent to ourself) to request a new sample
			scheduleAt(simTime()+DRIFTED_TIME(APP_SAMPLE_INTERVAL), new App_GenericDataPacket("Application self message (request sample)", APP_SELF_REQUEST_SAMPLE));
			
			break;
		}
		
		
		case APP_DATA_PACKET:
		{
			App_GenericDataPacket *rcvPacket;
			rcvPacket = check_and_cast<App_GenericDataPacket*>(msg);
			onReceiveMessage(rcvPacket->getData());
			break;
		}
		
		
		case SDM_2_APP_SENSOR_READING:
		{
			SensorDevMgr_GenericMessage *rcvReading;
			rcvReading = check_and_cast<SensorDevMgr_GenericMessage*>(msg);
			
			int sensIndex =  rcvReading->getSensorIndex();
			string sensType(rcvReading->getSensorType());
			double sensValue = rcvReading->getSensedValue();
			
			onSensorReading(sensIndex, sensType, sensValue);
			
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
		
		
		default:
		{
			EV << "\\n[Application]:\n WARNING: received packet of unknown type";
			break;
		}
	}
	
	delete msg;
	msg = NULL;		// safeguard
}


void ApplicationModuleSimple::finish()
{
	recordScalar("Node's total packets RX", totalPackets);
	/*recordScalar("Node's max sensed value", currMaxSensedValue);
	recordScalar("Node's max received value", currMaxReceivedValue);*/
	
	//recordScalar("Node's Value", theValue);
}



void ApplicationModuleSimple::send2MacDataPacket(int destID, double data)
{
	if(!disabled)
	{
		App_GenericDataPacket *packet2Mac;
		packet2Mac = new App_GenericDataPacket("Application Packet Application->Mac", APP_DATA_PACKET);
		packet2Mac->setData(data);
		packet2Mac->setSrcID(self);
		packet2Mac->setDestID(destID);
		packet2Mac->setFrameLength(sizeof(double) + packetHeaderOverhead);
		
		send(packet2Mac, "toCommunicationModule");
	}
}


void ApplicationModuleSimple::onReceiveMessage(double receivedData)
{
	//appVector.record(receivedData);
	/****
	 ****  ADD CODE HERE FOR PROCESSING THE RECEIVED MESSAGE  ****
	 ****/
	 totalPackets++;
	 
	 /*if(receivedData > currMaxReceivedValue)
	 	currMaxReceivedValue = receivedData;*/
	
	/*
	if ((receivedData > TEMP_THRESHOLD) && !sentOnce)
	{
		theValue = receivedData;
		int dest = -1; //broadcast
		send2MacDataPacket(dest, theValue);
		sentOnce = 1;
	}*/
	//send2MacDataPacket(-1, receivedData);
}


void ApplicationModuleSimple::onSensorReading(int sensorIndex, string sensorType, double sensedValue)
{
	/****
	 ****  ADD CODE HERE FOR PROCESSING THE NEW SENSOR READING  ****
	 ****/
	//doesn't matter that the destID == self
	int dest = -1; //broadcast
	send2MacDataPacket(dest, sensedValue);
	 
	if(sensedValue > currMaxSensedValue)
		currMaxSensedValue = sensedValue;
	
	/*if( (!sentOnce) && (sensedValue > TEMP_THRESHOLD) )
	{
		theValue = sensedValue;	// update only if zero
		
		int dest = -1; //broadcast
		send2MacDataPacket(dest, theValue);
		sentOnce = 1;
	}*/
	
}
