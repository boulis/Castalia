/************************************************************************************
 *  Copyright: National ICT Australia,  2006						*
 *  Developed at the Networks and Pervasive Computing program				*
 *  Author(s): Athanassios Boulis, Dimosthenis Pediaditakis				*
 *  This file is distributed under the terms in the attached LICENSE file.		*
 *  If you do not find this file, copies can be found by writing to:			*
 *											*
 *      NICTA, Locked Bag 9013, Alexandria, NSW 1435, Australia				*
 *      Attention:  License Inquiry.							*
 *											*
 ************************************************************************************/


#include "multipathAggregation_ApplicationModule.h"

#define EV   ev.disabled() ? (ostream&)ev : ev

#define DRIFTED_TIME(time) ((time) * cpuClockDrift)

#define MEMORY_PROFILE_STORE(numBytes) resMgrModule->RamStore(numBytes)

#define MEMORY_PROFILE_FREE(numBytes) resMgrModule->RamFree(numBytes)

#define STARTUP_DELAY 0.05

#define APP_SAMPLE_INTERVAL 0.5

#define CIRCUITS_PREPARE_DELAY 0.001

#define BROADCAST_ADDR -1

#define HUGE_VALUE 100000000


Define_Module(multipathAggregation_ApplicationModule);



void multipathAggregation_ApplicationModule::initialize() 
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
	sinkID = par("sinkID");
	
	epoch = par("epoch");
	
	/*tableUpdateRate = par("tableUpdateRate"); // number of updates per second
	if(tableUpdateRate <= 0)
		opp_error("\nError! Parameter \"tableUpdateRate\" has zero or negative value.\n");
	appTimer1_delay = (double) 1/tableUpdateRate;*/
	
	if(self == sinkID)
		isSink = 1;
	else
		isSink = 0;
	
	treeLevel = HUGE_VALUE;
		
	/*neighborTable.clear();*/
	
	lastSensedValue = -1;
	
	inInitializationPhase = 0;
	
	isInitialized = 0;
	
	initPhase_time_delay = par("initPhaseDuration");
}


void multipathAggregation_ApplicationModule::handleMessage(cMessage *msg)
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
			
			/***
				PLACE HERE THE CODE FOR STARTING-UP/SCHEDULING THE APPLICATION TIMERS
			 ***/
			
			inInitializationPhase = 1;
			//schedule when to exit from the Initialization Phase
			scheduleAt(simTime()+DRIFTED_TIME(initPhase_time_delay), new App_GenericDataPacket("timer 1", APP_TIMER_1));
			
			if(isSink)
				bootStrapInitPhase();
				
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
			multipathAggregation_DataPacket *rcvPacket;
			rcvPacket = check_and_cast<multipathAggregation_DataPacket *>(msg);
			
			int senderID = rcvPacket->getHeader().srcID;
			int destinationID = rcvPacket->getHeader().destID;
			int msgLevel = rcvPacket->getLevel();
			int msgType = rcvPacket->getMessageType();
			int msgSequenceNumber = rcvPacket->getHeader().seqNumber;

			/**
		   	    ADD HERE YOUR CODE FOR HANDLING A RECEIVED DATA PACKET FROM THE NETWORK
			    
			    SENDER ID:       senderID
			    DESTINATION ID:  destinationID
			    DATA:            theData
			    SEQUENCE NUMBER: sequenceNumber
			**/
			
			if(msgType == APP_MULTI_TREE_INIT_PACKET)
			{
				int initiator = rcvPacket->getInitInfo().initiatorID;
				int parent = rcvPacket->getInitInfo().parentID;
				
				if(inInitializationPhase)
				{
					//inInitializationPhase = 0; // set to 0 because now we have got attached to the tree and got a level
					if((msgLevel+1) < treeLevel)
					{
						treeLevel = msgLevel+1;
						epoch = epoch / pow(2, treeLevel);
						send2MacInitPacket(BROADCAST_ADDR, initiator, treeLevel, parent, 1);
					}
					
					if(!isInitialized)
						isInitialized = 1;

				}
			}
			else
			if(msgType == APP_MULTI_TREE_DATA_PACKET)
			{
				double theData = rcvPacket->getData();
				
				if(!inInitializationPhase)
				{
					
				}
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
			
			inInitializationPhase = 0;
			
			scheduleAt(simTime() + DRIFTED_TIME(epoch), new App_GenericDataPacket("timer 1", APP_TIMER_2));
			
			
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
			 
			scheduleAt(simTime()+ DRIFTED_TIME(epoch), new App_GenericDataPacket("timer 2", APP_TIMER_2));
			 
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
			
			//updateNeighborTable(); 
			
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
			
			lastSensedValue = sensValue;
			
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




void multipathAggregation_ApplicationModule::finish()
{
	recordScalar("The tree level).", treeLevel);
}


/***
    Based on the custom message definition *.msg file of your application the second parameter of the following function might need
    to have a different type. The type should be the same as in your *.msg file data field.
***/
void multipathAggregation_ApplicationModule::send2MacDataPacket(int destID, double data, int level, int pckSeqNumber)
{
	if(!disabled)
	{
		multipathAggregation_DataPacket *packet2Mac;
		packet2Mac = new multipathAggregation_DataPacket("Application Packet Application->Mac", APP_DATA_PACKET);
		
		packet2Mac->setMessageType(APP_MULTI_TREE_DATA_PACKET);
		
		packet2Mac->setData(data);
		packet2Mac->setLevel(level);
		
		packet2Mac->getHeader().srcID = self;
		packet2Mac->getHeader().destID = destID;
		packet2Mac->getHeader().seqNumber = pckSeqNumber;
		packet2Mac->setByteLength(sizeof(double) + 2*sizeof(int) + packetHeaderOverhead);
		//TODO: here the user can control the size of the packet and break it into smaller fragments
		send(packet2Mac, "toCommunicationModule");
	}
}


void multipathAggregation_ApplicationModule::requestSampleFromSensorManager()
{
	// send the request message to the Sensor Device Manager
	SensorDevMgr_GenericMessage *reqMsg;
	reqMsg = new SensorDevMgr_GenericMessage("app 2 sensor dev manager (Sample request)", APP_2_SDM_SAMPLE_REQUEST);
	reqMsg->setSensorIndex(0); //we need the index of the vector in the sensorTypes vector to distinguish the self messages for each sensor 
	send(reqMsg, "toSensorDeviceManager");
	
	// schedule a message (sent to ourself) to request a new sample
	scheduleAt(simTime()+DRIFTED_TIME(APP_SAMPLE_INTERVAL), new App_GenericDataPacket("Application self message (request sample)", APP_SELF_REQUEST_SAMPLE));
	
}


void multipathAggregation_ApplicationModule::send2MacInitPacket(int destID, int initiatorID, int level, int parentID, int pckSeqNumber)
{
	if(!disabled)
	{
		multipathAggregation_DataPacket *packet2Mac;
		packet2Mac = new multipathAggregation_DataPacket("Application Packet Application->Mac", APP_DATA_PACKET);
		
		packet2Mac->setMessageType(APP_MULTI_TREE_INIT_PACKET);
		
		packet2Mac->setLevel(level);
		packet2Mac->getInitInfo().initiatorID = initiatorID;
		packet2Mac->getInitInfo().parentID = parentID;
		
		packet2Mac->getHeader().srcID = self;
		packet2Mac->getHeader().destID = destID;
		packet2Mac->getHeader().seqNumber = pckSeqNumber;
		
		packet2Mac->setByteLength(4*sizeof(int) + packetHeaderOverhead);
		//TODO: here the user can control the size of the packet and break it into smaller fragments
		send(packet2Mac, "toCommunicationModule");
	}
}


void multipathAggregation_ApplicationModule::bootStrapInitPhase()
{
	treeLevel = 0;
	
	send2MacInitPacket(BROADCAST_ADDR, self, treeLevel, -1, 1);
	
	isInitialized = 1;
}


void multipathAggregation_ApplicationModule::updateNeighborTable()
{
	
}
