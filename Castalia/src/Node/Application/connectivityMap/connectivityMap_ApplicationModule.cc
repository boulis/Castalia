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


#include "connectivityMap_ApplicationModule.h"

#define EV   ev.disabled() ? (ostream&)ev : ev

#define DRIFTED_TIME(time) ((time) * cpuClockDrift)

#define MEMORY_PROFILE_STORE(numBytes) resMgrModule->RamStore(numBytes)

#define MEMORY_PROFILE_FREE(numBytes) resMgrModule->RamFree(numBytes)

#define STARTUP_DELAY 0.05

#define APP_SAMPLE_INTERVAL 0.5

#define CIRCUITS_PREPARE_DELAY 0.001

#define BROADCAST_ADDR -1

#define TXINTERVAL_THRESHOLD 10


/************ Connectivity Map Definitions  ******************************/

#define INTER_PACKET_SPACING 0.02

/************ End of Connectivity Map Definitions  ******************************/



Define_Module(connectivityMap_ApplicationModule);




void connectivityMap_ApplicationModule::initialize() 
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
	
	char buff[30];
	sprintf(buff, "Application Vector of Node %d", self);
	appVector.setName(buff);
	
	// Send the STARTUP message to MAC & to Sensor_Manager modules so that the node start to operate. (after a random delay, because we don't want the nodes to be synchronized)
	double random_startup_delay = genk_dblrand(0) * STARTUP_DELAY;
	sendDelayed(new App_GenericDataPacket("Application --> Sensor Dev Mgr [STARTUP]", APP_NODE_STARTUP), simTime() + DRIFTED_TIME(random_startup_delay), "toSensorDeviceManager");
	sendDelayed(new App_GenericDataPacket("Application --> Mac [STARTUP]", APP_NODE_STARTUP), simTime() + DRIFTED_TIME(random_startup_delay), "toCommunicationModule");
	scheduleAt(simTime() + DRIFTED_TIME(random_startup_delay)+CIRCUITS_PREPARE_DELAY, new App_GenericDataPacket("Application --> Application (self)", APP_NODE_STARTUP));
	
	
	
	/**
	  ADD HERE INITIALIZATION CODE HERE FOR YOUR APPLICATION
	 **/
	neighborTable.clear();
	packetsSent = 0;
	last_sensed_value = -10;
	
	totalSimTime = ev.config()->getAsTime("General", "sim-time-limit");
	totalSNnodes = parentModule()->parentModule()->par("numNodes");
	
	txInterval = totalSimTime / totalSNnodes;
	
	txInterval = (txInterval > TXINTERVAL_THRESHOLD)?TXINTERVAL_THRESHOLD:txInterval;
	
	currentNodeTx = -1;
}


void connectivityMap_ApplicationModule::handleMessage(cMessage *msg)
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
			
			currentNodeTx = 0;
			
			/******* Start the timer of ConnectivityMap Application  *****************/
			scheduleAt(simTime()+ DRIFTED_TIME(INTER_PACKET_SPACING), new App_GenericDataPacket("timer to transmit a packet", APP_TIMER_1));
			
			break;
		}
		
		
		case APP_SELF_REQUEST_SAMPLE:
		{
			requestSampleFromSensorManager();

			break;
		}
		
		
		case APP_DATA_PACKET:
		{
			connectivityMap_DataPacket *rcvPacket;
			rcvPacket = check_and_cast<connectivityMap_DataPacket *>(msg);
			
			int senderID = rcvPacket->getHeader().srcID;
			int destinationID = rcvPacket->getHeader().destID;
			double theData = rcvPacket->getData();
			int sequenceNumber = rcvPacket->getHeader().seqNumber;

			/**
		   	    ADD HERE YOUR CODE FOR HANDLING A RECEIVED DATA PACKET FROM THE NETWORK
			    
			    SENDER ID:       senderID
			    DESTINATION ID:  destinationID
			    DATA:            theData
			    SEQUENCE NUMBER: sequenceNumber
			**/
			
			updateNeighborTable(senderID);

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
			
			simtime_t now = simTime();
			
			int nodeTxNow = (int)floor(now / txInterval);
			
			if(self == nodeTxNow)
			{
				send2MacDataPacket(BROADCAST_ADDR, last_sensed_value, 1);
				packetsSent++;
			}
			
			scheduleAt(now + DRIFTED_TIME(INTER_PACKET_SPACING), new App_GenericDataPacket("timer to transmit a packet", APP_TIMER_1));
			
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

			/**
		   	    ADD HERE YOUR CODE FOR HANDLING A NEW SAMPLE FORM THE SENSOR
			    
			    SENSED VALUE: sensIndex
			    SENSOR TYPE:  sensType
			    SENSOR INDEX: sensValue
			**/
			last_sensed_value = sensValue;
			
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




void connectivityMap_ApplicationModule::finish()
{
	int i=0, tblSize = (int)neighborTable.size();
	char buff[100];
	
	EV << "\n\n** Node [" << self << "] received from:\n";
	
	for(i=0; i < tblSize; i++)
	{
		EV << "[" << self << "<--" << neighborTable[i].id << "]\t -->\t " << neighborTable[i].timesRx << "\t out of \t" << packetsSent << "\n";
		//recordScalar("Received from node", neighborTable[i].id);
		//sprintf(buff, "Number of packets received from Node[%d]", neighborTable[i].id);
		//recordScalar(buff, neighborTable[i].timesRx);
	}
}



void connectivityMap_ApplicationModule::send2MacDataPacket(int destID, double data, int pckSeqNumber)
{
	if(!disabled)
	{
		connectivityMap_DataPacket *packet2Mac;
		packet2Mac = new connectivityMap_DataPacket("Application Packet Application->Mac", APP_DATA_PACKET);
		packet2Mac->setData(data);
		packet2Mac->getHeader().srcID = self;
		packet2Mac->getHeader().destID = destID;
		packet2Mac->getHeader().seqNumber = pckSeqNumber;
		packet2Mac->setByteLength(sizeof(double) + packetHeaderOverhead);
		//TODO: here the user can control the size of the packet and break it into smaller fragments
		send(packet2Mac, "toCommunicationModule");
	}
}


void connectivityMap_ApplicationModule::requestSampleFromSensorManager()
{
	// send the request message to the Sensor Device Manager
	SensorDevMgr_GenericMessage *reqMsg;
	reqMsg = new SensorDevMgr_GenericMessage("app 2 sensor dev manager (Sample request)", APP_2_SDM_SAMPLE_REQUEST);
	reqMsg->setSensorIndex(0); //we need the index of the vector in the sensorTypes vector to distinguish the self messages for each sensor 
	send(reqMsg, "toSensorDeviceManager");
	
	// schedule a message (sent to ourself) to request a new sample
	//scheduleAt(simTime()+DRIFTED_TIME(APP_SAMPLE_INTERVAL), new App_GenericDataPacket("Application self message (request sample)", APP_SELF_REQUEST_SAMPLE));
	
}


void connectivityMap_ApplicationModule::updateNeighborTable(int nodeID)
{
	int i=0, pos = -1;
	int tblSize = (int)neighborTable.size();
	
	for(i=0; i < tblSize; i++)
		if(neighborTable[i].id == nodeID)
			pos = i;
	
	if(pos == -1)
	{
		neighborRecord newRec;
		newRec.id = nodeID;
		newRec.timesRx = 1;
		
		neighborTable.push_back(newRec);
	}
	else
		neighborTable[pos].timesRx++;
}
