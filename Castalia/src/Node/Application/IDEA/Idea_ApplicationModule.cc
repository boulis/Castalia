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


#include "Idea_ApplicationModule.h"

#define EV   ev.disabled() ? (ostream&)ev : ev

#define DRIFTED_TIME(time) ((time) * cpuClockDrift)

#define MEMORY_PROFILE_STORE(numBytes) resMgrModule->RamStore(numBytes)

#define MEMORY_PROFILE_FREE(numBytes) resMgrModule->RamFree(numBytes)

#define STARTUP_DELAY 0.05

#define APP_SAMPLE_INTERVAL 0.5

#define CIRCUITS_PREPARE_DELAY 0.001

#define STALLNESS_MULTIPLYER 2

#define BROADCAST_ADDR -1

Define_Module(Idea_ApplicationModule);



void Idea_ApplicationModule::initialize() 
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
	
	cModule *devMgrModule;
	if(parent->findSubmodule("nodeSensorDevMgr") != -1)
	{	
		devMgrModule = check_and_cast<cModule*>(parent->submodule("nodeSensorDevMgr"));
	}
	else
		opp_error("\n[Application]:\n Error in geting a valid reference to  nodeResourceMgr.");
		
	cpuClockDrift = resMgrModule->getCPUClockDrift();
	
	disabled = 1;
	
	priority = par("priority");
	
	maxAppPacketSize = par("maxAppPacketSize");
	
	packetHeaderOverhead = par("packetHeaderOverhead");
	
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
	
	
	
	
	
	/**
	   ADD INITIALIZATION CODE HERE FOR YOUR APPLICATION
	 **/
	 
	//we dont just call resMgrModule->getSensorDeviceBias(0) because we are not sure if the resMgrModule has been initialized  
	cStringTokenizer biasSigmaTokenizer(devMgrModule->par("devicesBias"));
	const char *token = biasSigmaTokenizer.nextToken();
	sensorVariance = (double)atof(token);
	
	nodeGlobalEstimation.mean = 0;
	nodeGlobalEstimation.variance = 100000;
	threshold.mean = par("thresholdMean");
	threshold.variance = par("thresholdVariance");
	minimumNeighNum = par("minimumNeighNum");
	neighTable.clear();

	previousReadingWasClose = 1;
	globalEstimTimeStamp = 0;
	
	char nameA[50];
	sprintf(nameA, "Node_%d Sensed Values Vector", self);
	valuesVector.setName(nameA);
	char nameB[50];
	sprintf(nameB, "Node_%d Estimates Vector", self);
	estimatesVector.setName(nameB);

	RXVector.setName("Node 6 RX");

	totalBroadcasts = 0;
	totalPacketsReceived = 0;
}


void Idea_ApplicationModule::handleMessage(cMessage *msg)
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
			reqMsg->setSensorIndex(0); //we need the index of the vector in the sensorTypes vector to distinguish the self messages for each sensor 
			send(reqMsg, "toSensorDeviceManager");
			
			// schedule a message (sent to ourself) to request a new sample
			scheduleAt(simTime()+DRIFTED_TIME(APP_SAMPLE_INTERVAL), new App_GenericDataPacket("Application self message (request sample)", APP_SELF_REQUEST_SAMPLE));
			
			break;
		}
		
		
		case APP_DATA_PACKET:
		{
			Idea_DataPacket *rcvPacket;
			rcvPacket = check_and_cast<Idea_DataPacket *>(msg);
			onReceiveMessage(rcvPacket->getHeader().srcID, rcvPacket->getData());
			break;
		}
		

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


void Idea_ApplicationModule::finish()
{
	EV << "\n\nTOTAL NEIGH SIZE: " << neighTable.size() << "\n\n";
	recordScalar("Neighbours table size.", neighTable.size());
	recordScalar("Total broadcasts of DATA packets(TX).", totalBroadcasts);
	recordScalar("Total DATA packets received (RX) [non beacons].", totalPacketsReceived);
}



void Idea_ApplicationModule::send2MacDataPacket(int destID, estim data, int pckSeqNumber)
{
	if(!disabled)
	{
		Idea_DataPacket *packet2Mac;
		packet2Mac = new Idea_DataPacket("Application Packet Application->Mac", APP_DATA_PACKET);
		packet2Mac->setData(data);
		packet2Mac->getHeader().srcID = self;
		packet2Mac->getHeader().destID = destID;
		packet2Mac->getHeader().seqNumber = pckSeqNumber;
		packet2Mac->setByteLength(sizeof(estim) + packetHeaderOverhead);
		//TODO: here the user can control the size of the packet and break it into smaller fragments
		send(packet2Mac, "toCommunicationModule");
	}
}


void Idea_ApplicationModule::requestSampleFromSensorManager()
{
	// send the request message to the Sensor Device Manager
	SensorDevMgr_GenericMessage *reqMsg;
	reqMsg = new SensorDevMgr_GenericMessage("app 2 sensor dev manager (Sample request)", APP_2_SDM_SAMPLE_REQUEST);
	reqMsg->setSensorIndex(0); //we need the index of the vector in the sensorTypes vector to distinguish the self messages for each sensor 
	send(reqMsg, "toSensorDeviceManager");
	
	// schedule a message (sent to ourself) to request a new sample
	scheduleAt(simTime()+DRIFTED_TIME(APP_SAMPLE_INTERVAL), new App_GenericDataPacket("Application self message (request sample)", APP_SELF_REQUEST_SAMPLE));
	
}


void Idea_ApplicationModule::onReceiveMessage(int senderID, estim receivedGlobalEstim)
{
	/****
	 ****  ADD CODE HERE FOR PROCESSING THE RECEIVED MESSAGE  ****
	 ****/
	
	totalPacketsReceived++;

	RXVector.record(senderID);
	RXVector.record(receivedGlobalEstim.mean);
	RXVector.record(receivedGlobalEstim.variance);
	RXVector.record(nodeGlobalEstimation.mean);
	RXVector.record(nodeGlobalEstimation.variance);

	// the struct that will hold the new global estimation
	estim FusedEstimation;

	// update the node estimate's variance with respect to elapsed time of creation of the estimate --> staleness of data trick
	//simtime_t age = simTime() - globalEstimTimeStamp;
	//nodeGlobalEstimation.variance *= 1+(age*STALLNESS_MULTIPLYER);

	// fuse our current global Estimation with the new received Global Estimation
	fuseglobal(receivedGlobalEstim, nodeGlobalEstimation, FusedEstimation);
	nodeGlobalEstimation = FusedEstimation;
	//globalEstimTimeStamp = simTime();

	// check the table with the neighbours and DECIDE wether or not to send the new global estimation
	NeighborRecord incoming;
	incoming.id = senderID;				//typical preparing incoming estimation to put in table
	incoming.estimation = receivedGlobalEstim;
	incoming.tag=false;
	int doTx = 0;
	
	doTx = neighbors(1, threshold, incoming, FusedEstimation, neighTable, simTime());	

	if(((int)neighTable.size()) < minimumNeighNum)
	{	
		doTx = 1;
	}
	
	if(doTx)
	{	
		send2MacDataPacket(BROADCAST_ADDR, FusedEstimation, 1);
		totalBroadcasts++;
	}
	 
}



void Idea_ApplicationModule::onSensorReading(int sensorIndex, string sensorType, double sensedValue)
{
	/****
	 ****  ADD CODE HERE FOR PROCESSING THE NEW SENSOR READING  ****
	 ****/
	
	EV << "\n\nSensed Value: " << sensedValue << " \n\n";
	valuesVector.record(sensedValue);
	estimatesVector.record(nodeGlobalEstimation.mean);

	// crate a local estimation from the sensor reading/value
	estim localEstimation;
	localEstimation.mean = sensedValue;
	localEstimation.variance = sensorVariance;

	// update the node estimate's variance with respect to elapsed time of creation of the estimate --> staleness of data trick
	//simtime_t age = simTime() - globalEstimTimeStamp;
	//nodeGlobalEstimation.variance *= 1+(age*STALLNESS_MULTIPLYER);
	
	// the struct that will hold the new global estimation
	estim FusedEstimation;
	
	/* fuse the localEstimation(sensor_reading) with the nodeGlobalEstimation(pre existing global estimation of the node)
	double difference = sensedValue - nodeGlobalEstimation.mean;
	double dist = (difference < 0)?-difference:difference;
	int isClose = (dist > (sensorVariance+nodeGlobalEstimation.variance))?0:1; //sensorVariance+nodeGlobalEstimation.variance can be raplaced with a fixed fraction
	if( (difference < 0) && (!isClose) && (previousReadingWasClose) )
	{
		// isSharpFall = 1
		fuselocal(localEstimation, nodeGlobalEstimation, FusedEstimation, 1);
	}
	else
	{
		// isSharpFall = 0
		fuselocal(localEstimation, nodeGlobalEstimation, FusedEstimation);
	}*/
	fuselocal(localEstimation, nodeGlobalEstimation, FusedEstimation);
	nodeGlobalEstimation = FusedEstimation;
	//globalEstimTimeStamp = simTime();
	//previousReadingWasClose = isClose;
	

	// check the table with the neighbours and DECIDE wether or not to send the new global estimation
	int doTx = 0;
	if(((int)neighTable.size()) < minimumNeighNum)
		doTx = 1;
	else
	{
		NeighborRecord blank;
		doTx = neighbors(2, threshold, blank, FusedEstimation, neighTable, simTime());	
	}
	
	
	
	if(doTx)
	{
		send2MacDataPacket(BROADCAST_ADDR, FusedEstimation, 1);
		totalBroadcasts++;
	}
	
}
