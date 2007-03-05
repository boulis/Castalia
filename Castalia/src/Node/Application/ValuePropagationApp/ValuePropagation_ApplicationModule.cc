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


#include "ValuePropagation_ApplicationModule.h"

#define EV   ev.disabled() ? (ostream&)ev : ev

#define DRIFTED_TIME(time) ((time) * cpuClockDrift)

#define MEMORY_PROFILE_STORE(numBytes) resMgrModule->RamStore(numBytes)

#define MEMORY_PROFILE_FREE(numBytes) resMgrModule->RamFree(numBytes)

#define STARTUP_DELAY 0.05

#define APP_SAMPLE_INTERVAL 0.5

#define CIRCUITS_PREPARE_DELAY 0.001

#define TEMP_THRESHOLD 40

#define BROADCAST_ADDR -1

Define_Module(ValuePropagation_ApplicationModule);



void ValuePropagation_ApplicationModule::initialize() 
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
	
	sensedValues.clear();
}


void ValuePropagation_ApplicationModule::handleMessage(cMessage *msg)
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
			
			/******* Start here the timers that you plan to use  *****************
			  e.g.: scheduleAt(simTime(), new App_GenericDataPacket("timer 1", APP_TIMER_1));
			**********************************************************************/

			break;
		}
		
		
		case APP_SELF_REQUEST_SAMPLE:
		{
			// send the request message to the Sensor Device Manager
			SensorDevMgr_GenericMessage *reqMsg;
			reqMsg = new SensorDevMgr_GenericMessage("app 2 sensor dev manager (Sample request)", APP_2_SDM_SAMPLE_REQUEST);
			reqMsg->setSensorIndex(0); //we need to specify which sensor we are "asking" for a sample
			send(reqMsg, "toSensorDeviceManager");
			
			// schedule a message (sent to ourself) to request a new sample
			//scheduleAt(simTime()+DRIFTED_TIME(APP_SAMPLE_INTERVAL), new App_GenericDataPacket("Application self message (request sample)", APP_SELF_REQUEST_SAMPLE));
			
			break;
		}
		
		
		case APP_DATA_PACKET:
		{
			ValuePropagation_DataPacket *rcvPacket;
			rcvPacket = check_and_cast<ValuePropagation_DataPacket *>(msg);
			/***************************************/
			onReceiveMessage(rcvPacket->getHeader().srcID, rcvPacket->getData());
			/***************************************/
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
		 * There are 5 more timers messages defined in the App_GenericDataPacket.msg file, (APP_TIMER_4 to APP_TIMER_8). If you need more
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


void ValuePropagation_ApplicationModule::finish()
{
	//recordScalar("Node's total packets RX", totalPackets);
	//recordScalar("Node's max sensed value", currMaxSensedValue);
	//recordScalar("Node's max received value", currMaxReceivedValue);
	recordScalar("Node's Value", theValue);
	
	/*
	if(self == 6)
	{
		cout << "\n*** Node " << self << " (sensed values with periodic sample request)\n";
		for(int i=0; i<sensedValues.size(); i++)
		{
			cout << sensedValues[i] << " \n";
		}
	}
	*/
	sensedValues.clear();
}



void ValuePropagation_ApplicationModule::send2MacDataPacket(int destID, double data, int pckSeqNumber)
{
	if(!disabled)
	{
		ValuePropagation_DataPacket *packet2Mac;
		packet2Mac = new ValuePropagation_DataPacket("Application Packet Application->Mac", APP_DATA_PACKET);
		packet2Mac->setData(data);
		packet2Mac->getHeader().srcID = self;
		packet2Mac->getHeader().destID = destID;
		packet2Mac->getHeader().seqNumber = pckSeqNumber;
		packet2Mac->setByteLength(sizeof(double) + packetHeaderOverhead);
		//TODO: here the user can control the size of the packet and break it into smaller fragments
		send(packet2Mac, "toCommunicationModule");
	}
}


void ValuePropagation_ApplicationModule::requestSampleFromSensorManager()
{
	// send the request message to the Sensor Device Manager
	SensorDevMgr_GenericMessage *reqMsg;
	reqMsg = new SensorDevMgr_GenericMessage("app 2 sensor dev manager (Sample request)", APP_2_SDM_SAMPLE_REQUEST);
	reqMsg->setSensorIndex(0); //we need the index of the vector in the sensorTypes vector to distinguish the self messages for each sensor 
	send(reqMsg, "toSensorDeviceManager");
	
	// schedule a message (sent to ourself) to request a new sample
	scheduleAt(simTime()+DRIFTED_TIME(APP_SAMPLE_INTERVAL), new App_GenericDataPacket("Application self message (request sample)", APP_SELF_REQUEST_SAMPLE));
	
}


void ValuePropagation_ApplicationModule::onReceiveMessage(int senderID, double receivedData)
{
	//appVector.record(receivedData);
	/****
	 ****  ADD CODE HERE FOR PROCESSING THE RECEIVED MESSAGE  ****
	 ****/
	 totalPackets++;
	 
	 if(receivedData > currMaxReceivedValue)
	 	currMaxReceivedValue = receivedData;
	
	
	if ((receivedData > TEMP_THRESHOLD) && !sentOnce)
	{
		theValue = receivedData;
		send2MacDataPacket(BROADCAST_ADDR, theValue, 1);
		sentOnce = 1;
	}
}


void ValuePropagation_ApplicationModule::onSensorReading(int sensorIndex, string sensorType, double sensedValue)
{
	/****
	 ****  ADD CODE HERE FOR PROCESSING THE NEW SENSOR READING  ****
	 ****/
	 
	if(sensedValue > currMaxSensedValue)
		currMaxSensedValue = sensedValue;
	
	if( (!sentOnce) && (sensedValue > TEMP_THRESHOLD) )
	{
		theValue = sensedValue;	// update only if zero
		
		send2MacDataPacket(BROADCAST_ADDR, theValue, 1);
		sentOnce = 1;
	}
	
	sensedValues.push_back(sensedValue);
	
}
