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



#include "simpleAggregation_ApplicationModule.h"

#define EV   ev.disabled()?(ostream&)ev:ev

#define CASTALIA_DEBUG (!printDebugInfo)?(ostream&)DebugInfoWriter::getStream():DebugInfoWriter::getStream()

#define DRIFTED_TIME(time) ((time) * cpuClockDrift)

#define MEMORY_PROFILE_STORE(numBytes) resMgrModule->RamStore(numBytes)

#define MEMORY_PROFILE_FREE(numBytes) resMgrModule->RamFree(numBytes)

#define STARTUP_DELAY 0.01

#define APP_SAMPLE_INTERVAL 1.0

#define CIRCUITS_PREPARE_DELAY 0.001

#define EPSILON 0.000001


#define NO_ROUTING_LEVEL -1

/************ Implementation Functions of TunableMAC set functions ******************************/
//  If you are not going to use the TunableMAC module, then you can comment the following line and build Castalia
#include "simpleAggregation_radioControlImpl.cc.inc"
#include "simpleAggregation_tunableMacControlImpl.cc.inc"
/************ Connectivity Map Definitions  ************************************************/


Define_Module(simpleAggregation_ApplicationModule);



void simpleAggregation_ApplicationModule::initialize() 
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
	 
	 if(isSink)
	 	routingLevel = 0;
	 else
	 	routingLevel = NO_ROUTING_LEVEL;
	 
	 aggregatedValue = 0.0;
	 
	 waitingTimeForLowerLevelData = 0.0;
	 
	 app_timer_A = NULL;
	 
	 lastSensedValue = 0.0;
	 
	 totalPackets = 0;
}


void simpleAggregation_ApplicationModule::handleMessage(cMessage *msg)
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
			
			scheduleAt(simTime()+STARTUP_DELAY, new App_ControlMessage("Application self message (request sample)", APP_SELF_REQUEST_SAMPLE));
			
			if(isSink)
				waitingTimeForLowerLevelData = APP_SAMPLE_INTERVAL-EPSILON;		    	
			
			break;
		}
			
		
		case APP_SELF_REQUEST_SAMPLE:
		{
			requestSampleFromSensorManager();
		
			// schedule a message (sent to ourself) to request a new sample
			scheduleAt(simTime()+DRIFTED_TIME(APP_SAMPLE_INTERVAL), new App_ControlMessage("Application self message (request sample)", APP_SELF_REQUEST_SAMPLE));
			
			// maybe we should requestSampleFromSensorManager() just before the APP_TIMER_1
			// and we send our aggregated value to the sink
			// because now, we send an aggregated value, that the last sample from our sensor has not
			// contributed at all...
			// or just put some extra code in the case SDM_2_APP_SENSOR_READING: 
			//  if(lastSensedValue > aggregatedValue)
		    //	  aggregatedValue = lastSensedValue;
		    // the problem is that whenever APP_TIMER_1 occurs, we never take into account the most recent 
		    // sensor reading. We do that right after we send the aggregated value.
		    // This is why we notice big delays (~2 secs) in the time difference of sampling a certain big value
		    // till the appearence of this at the aggregated value of the sink.
			
      		// at the same time schedule the timer that specifies how long we wait before we transmit
			if (routingLevel != NO_ROUTING_LEVEL)
   			{
   				// re-schedule the APP_TIMER_1
	    		if( (app_timer_A != NULL) && (app_timer_A->isScheduled()) )
	    			cancelAndDelete(app_timer_A);
	    		
	    		app_timer_A = new App_ControlMessage("timer 1", APP_TIMER_1);
	    		
	    		scheduleAt(simTime()+DRIFTED_TIME(waitingTimeForLowerLevelData), app_timer_A);
   			}
    			
			break;
		}
		
		
		case APP_DATA_PACKET:
		{
			simpleAggregation_DataPacket *rcvPacket;
			rcvPacket = check_and_cast<simpleAggregation_DataPacket *>(msg);
			
			string msgSender(rcvPacket->getHeader().source.c_str());
			string msgDestination(rcvPacket->getHeader().destination.c_str());
			
			double theData = rcvPacket->getData();
			int sequenceNumber = rcvPacket->getHeader().seqNumber;
			double rssi = rcvPacket->getRssi();
			string pathFromSource(rcvPacket->getCurrentPathFromSource());
			
			// do the aggregation bit. For this example aggregation function is a simple max.
		    if (theData > aggregatedValue)
		    	aggregatedValue = theData;
		    
		    //printf("\n-- Node[%i] --\n", self);
		    
		    if(isSink)	
		    	CASTALIA_DEBUG << "\n T=" << simTime() << ",  Node[" << self << "] \treceived from:" << msgSender << " \tvalue=" << theData;
		    
			break;
		}

		
		/*--------------------------------------------------------------------------------------------------------------
		 * Received whenever application timer 1 expires
		 *--------------------------------------------------------------------------------------------------------------*/
		case APP_TIMER_1:
		{
			/* 
			 * The time waiting for data from nodes in the upper level has passed
			 * it is time to transmit the aggregated value so far to the level below
			 */
			
		 	// has to be a node other than the sink
		   	if(!isSink)
		   	{	
		   		//if(lastSensedValue > aggregatedValue)
		    	//	aggregatedValue = lastSensedValue;
		    		
		   		send2NetworkDataPacket(PARENT_LEVEL, aggregatedValue, 1);
		   		
		   		totalPackets++;
		   	}
		   	else
			   	// first report the result from the previous cycle (only for the sink)
			    CASTALIA_DEBUG << "\n[App_"<< self <<"] t= " << simTime() << ": Aggregated Value = " << aggregatedValue;
			
			
			//if(self == 8)
			//	cout << "\n T=" << simTime() << ",  Node[" << self << "] \tsent value:" << aggregatedValue;
			
			
			/* a new cycle of aggregation has started */
		    // update the aggregatedValue for the new cycle.
	      	aggregatedValue = lastSensedValue;
			
			app_timer_A = NULL;
			
			break; 
		}
		
		
		
		case SDM_2_APP_SENSOR_READING:
		{
			SensorDevMgr_GenericMessage *rcvReading;
			rcvReading = check_and_cast<SensorDevMgr_GenericMessage*>(msg);
			
			int sensIndex =  rcvReading->getSensorIndex();
			string sensType(rcvReading->getSensorType());
			double sensValue = rcvReading->getSensedValue();
			
			//EV << "Node[" << self << "] (x=" << self_xCoo << ",y=" << self_yCoo << ") \tValue=" << sensValue << "\n";
			if(self != 8)
				sensValue = 0.0;
				
			lastSensedValue = sensValue;
			
			
			if(self == 8)
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
			 CASTALIA_DEBUG << "\n[App_"<< self <<"] t= " << simTime() << ": WARNING: NETWORK_2_APP_FULL_BUFFER received because the Network buffer is full.\n";
			 break;
		}
		
		
		case NETWORK_2_APP_TREE_LEVEL_UPDATED:
	    {
	    	// this message notifies the application of routing state (level)
	    	// for certain routing protocols.
	    	Network_ControlMessage *levelMsg = check_and_cast<Network_ControlMessage *>(msg);
	    	routingLevel = levelMsg->getLevel();
	    
	    	waitingTimeForLowerLevelData = APP_SAMPLE_INTERVAL/pow((double)2,routingLevel);
	    	
	    	CASTALIA_DEBUG << "\n[App_"<< self <<"] t= " << simTime() << ": Level " << routingLevel ;
	    	
	    	// re-schedule the APP_TIMER_1
    		if( (app_timer_A != NULL) && (app_timer_A->isScheduled()) )
    			cancelAndDelete(app_timer_A);
    		
    		app_timer_A = new App_ControlMessage("timer 1", APP_TIMER_1);
    		
    		scheduleAt(simTime()+DRIFTED_TIME(waitingTimeForLowerLevelData), app_timer_A);
	    	
	    	break;
	    }

	    
	    case NETWORK_2_APP_CONNECTED_2_TREE:
	    {
	    	Network_ControlMessage *connectedMsg = check_and_cast<Network_ControlMessage *>(msg);
	    	
	    	int treeLevel = connectedMsg->getLevel();
	    	int sinkID = connectedMsg->getSinkID();
	    	string parents;
	    	parents.assign(connectedMsg->getParents());
	    	
	    	CASTALIA_DEBUG << "\n[App_"<< self <<"] t= " << simTime() << ": Level " << treeLevel ;
	    	//CASTALIA_DEBUG << "\n[App_"<< self <<"] t= " << simTime() << ": Parent " << parents ;
	    	
	    	routingLevel = treeLevel;
	    		
	    	waitingTimeForLowerLevelData = APP_SAMPLE_INTERVAL/pow((double)2,routingLevel);
	    	
	    	// re-schedule the APP_TIMER_1
    		if( (app_timer_A != NULL) && (app_timer_A->isScheduled()) )
    			cancelAndDelete(app_timer_A);
    		
    		app_timer_A = new App_ControlMessage("timer 1", APP_TIMER_1);
    		
    		scheduleAt(simTime()+DRIFTED_TIME(waitingTimeForLowerLevelData), app_timer_A);
	    	
	    	break;
	    }
	    
	    
	    case NETWORK_2_APP_NOT_CONNECTED:
	    {
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




void simpleAggregation_ApplicationModule::finish()
{
	//fprintf(stderr, "\nNode[%i]  Total sent messages:%i", self, totalPackets);
	
	// output the spent energy of the node
	CASTALIA_DEBUG << "\n[Energy_"<< self <<"] t= " << simTime() << ": " << resMgrModule->getSpentEnergy();
	
	//EV <<  "\n[Energy_"<< self <<"] t= " << simTime() << ": " << resMgrModule->getSpentEnergy();
	
	//close the output stream that CASTALIA_DEBUG is writing to
	DebugInfoWriter::closeStream();
			 
	
}


/***
    Based on the custom message definition *.msg file of your application the second parameter of the following function might need
    to have a different type. The type should be the same as in your *.msg file data field.
***/
void simpleAggregation_ApplicationModule::send2NetworkDataPacket(const char *destID, double data, int pckSeqNumber)
{
	if(!disabled)
	{
		simpleAggregation_DataPacket *packet2Net;
		packet2Net = new simpleAggregation_DataPacket("Application Packet Application->Mac", APP_DATA_PACKET);
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
		
		send(packet2Net, "toCommunicationModule");
	}
}


void simpleAggregation_ApplicationModule::requestSampleFromSensorManager()
{
	// send the request message to the Sensor Device Manager
	SensorDevMgr_GenericMessage *reqMsg;
	reqMsg = new SensorDevMgr_GenericMessage("app 2 sensor dev manager (Sample request)", APP_2_SDM_SAMPLE_REQUEST);
	reqMsg->setSensorIndex(0); //we need the index of the vector in the sensorTypes vector to distinguish the self messages for each sensor 
	send(reqMsg, "toSensorDeviceManager");
	
}

