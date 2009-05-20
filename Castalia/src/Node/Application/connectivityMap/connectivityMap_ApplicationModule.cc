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




#include "connectivityMap_ApplicationModule.h"

#define EV   ev.disabled()?(ostream&)ev:ev

#define CASTALIA_DEBUG (!printDebugInfo)?(ostream&)DebugInfoWriter::getStream():DebugInfoWriter::getStream()

#define DRIFTED_TIME(time) ((time) * cpuClockDrift)

#define MEMORY_PROFILE_STORE(numBytes) resMgrModule->RamStore(numBytes)

#define MEMORY_PROFILE_FREE(numBytes) resMgrModule->RamFree(numBytes)

#define STARTUP_DELAY 0.05

#define APP_SAMPLE_INTERVAL 0.5

#define CIRCUITS_PREPARE_DELAY 0.001


/************ Connectivity Map Definitions  ******************************/

#define INTER_PACKET_SPACING 0.01
#define SENT_PACKETS_PER_NODE_PER_LEVEL 100
#define CHANGE_POWER_LEVELS 0         // 0 means only the default power level is used


/************ End of Connectivity Map Definitions  ******************************/



/************ Implementation Functions of TunableMAC set functions ******************************/
// If you are not going to use the TunableMAC module, then you can comment the following line and build Castalia
#include "connectivityMap_radioControlImpl.cc.inc"
#include "connectivityMap_tunableMacControlImpl.cc.inc"
/************ Connectivity Map Definitions  ************************************************/


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
		opp_error("\n[App]:\n Error in geting a valid reference to  nodeResourceMgr for direct method calls.");
	cpuClockDrift = resMgrModule->getCPUClockDrift();

	disabled = 1;

	const char * tmpID = par("applicationID");
	applicationID.assign(tmpID);

	printDebugInfo = par("printDebugInfo");

	printConnMap = par("printConnectivityMap");

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
	neighborTable.clear();
	packetsSent = 0;
	numTxPowerLevels = 1;

	totalSimTime = ev.config()->getAsTime("General", "sim-time-limit");
	totalSNnodes = parentModule()->parentModule()->par("numNodes");

	if (CHANGE_POWER_LEVELS) {
		// extract the numTxPowerLevels from the string module parameter
		const char *str = parentModule()->submodule("networkInterface")->submodule("Radio")->par("txPowerLevels");
		cStringTokenizer tokenizer(str);
		const char *token;
		numTxPowerLevels = 0;
		while ((token = tokenizer.nextToken())!=NULL){
      strcpy(txPowerLevelNames[numTxPowerLevels], token);
      numTxPowerLevels++;
    }
	}

	txInterval_perNode = SENT_PACKETS_PER_NODE_PER_LEVEL * INTER_PACKET_SPACING;
	txInterval_perPowerLevel = (txInterval_perNode * totalSNnodes);

	if (totalSimTime < txInterval_perPowerLevel * numTxPowerLevels){
		EV << "\n[Application_"<< self <<"] t= " << simTime() << "ERROR: Total sim time should be at least = " << txInterval_perPowerLevel * numTxPowerLevels;
		opp_error("\nError: simulation time not large enough for the conectivity map application\n");
	}

	currentPowerLevel = -1;

	serialNumber = 0;
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

			if (CHANGE_POWER_LEVELS) {
				currentPowerLevel = 0;
				setRadio_TXPowerLevel(currentPowerLevel);
			}

			/******* Start the timer(s) of ConnectivityMap Application  *****************/
			scheduleAt(simTime()+ DRIFTED_TIME(INTER_PACKET_SPACING), new App_ControlMessage("timer1", APP_TIMER_1));
			if (CHANGE_POWER_LEVELS) {
				scheduleAt(simTime()+ DRIFTED_TIME(txInterval_perPowerLevel), new App_ControlMessage("timer2", APP_TIMER_2));
			}
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

			string msgSender(rcvPacket->getHeader().source.c_str());
			string msgDestination(rcvPacket->getHeader().destination.c_str());

			int theData = rcvPacket->getData();
			double rssi = rcvPacket->getRssi();
			string pathFromSource(rcvPacket->getCurrentPathFromSource());

			/**
		   	    ADD HERE YOUR CODE FOR HANDLING A RECEIVED DATA PACKET FROM THE NETWORK

			    SENDER:       msgSender
			    DESTINATION:  msgDestination
			    DATA:         theData
			    SEQUENCE NUMBER: sequenceNumber
			**/
			// CASTALIA_DEBUG << "Node[" << self << "] received data from Node[" << atoi(msgSender.c_str()) << "]  --> data{" << theData << "} \n";
			updateNeighborTable(atoi(msgSender.c_str()), theData);

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

			int currentNodeTx = ((int)floor(now / txInterval_perNode))%totalSNnodes;

			if( (self == currentNodeTx) && (packetsSent < SENT_PACKETS_PER_NODE_PER_LEVEL) )
			{
				send2NetworkDataPacket(BROADCAST, serialNumber, 1);

				serialNumber++;
				packetsSent++;
			}

			scheduleAt(now + DRIFTED_TIME(INTER_PACKET_SPACING), new App_ControlMessage("timer1", APP_TIMER_1));

			break;
		}


		/*--------------------------------------------------------------------------------------------------------------
		 * Received whenever application timer 2 expires
		 *--------------------------------------------------------------------------------------------------------------*/
		case APP_TIMER_2:
		{

			// print out the ConnectivityMap of the sensor network (each node prints its own statistics:
			//   # of reveived packets from a specific node OUT OF total 3 of  the total packets that should have received under ideal reception conditions
			if(printConnMap)
			{
				EV << "\n\n** TX Power Level "<< txPowerLevelNames[currentPowerLevel]<<"dBm, Node [" << self << "] received from:\n";

				for(int i=0; i < (int)neighborTable.size(); i++)
				{
					EV << "[" << self << "<--" << neighborTable[i].id << "]\t -->\t " << neighborTable[i].receivedPackets << "\t out of \t" << SENT_PACKETS_PER_NODE_PER_LEVEL << "\n";
				}
			}

			if (currentPowerLevel < (numTxPowerLevels -1)) {
				neighborTable.clear();
				packetsSent = 0;
				serialNumber = 0;
				currentPowerLevel++;
				setRadio_TXPowerLevel(currentPowerLevel);
				scheduleAt(simTime()+ DRIFTED_TIME(txInterval_perPowerLevel), new App_ControlMessage("timer2", APP_TIMER_2));
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
			 CASTALIA_DEBUG << "\n[App_"<< self <<"] t= " << simTime() << ": WARNING: NETWORK_2_APP_FULL_BUFFER received because the Network buffer is full.\n";
			 break;
		}


		default:
		{
			CASTALIA_DEBUG << "\n[App_"<< self <<"] t= " << simTime() << ":WARNING: received packet of unknown type";
			break;
		}
	}

	delete msg;
	msg = NULL;		// safeguard
}




void connectivityMap_ApplicationModule::finish()
{
	// print out the ConnectivityMap of the sensor network (each node prints its own statistics:
	//   # of reveived packets from a specific node OUT OF total 3 of  the total packets that should have received under ideal reception conditions
	if(printConnMap)
	{

    if (CHANGE_POWER_LEVELS)
		  EV << "\n\n** TX Power Level "<< txPowerLevelNames[currentPowerLevel]<<"dBm, Node [" << self << "] received from:\n";
    else
		  EV << "\n\n** Node [" << self << "] received from:\n";

		for(int i=0; i < (int)neighborTable.size(); i++)
		{
			EV << "[" << self << "<--" << neighborTable[i].id << "]\t -->\t " << neighborTable[i].receivedPackets << "\t out of \t" << SENT_PACKETS_PER_NODE_PER_LEVEL << "\n";
		}
	}

	//close the output stream that CASTALIA_DEBUG is writing to
	DebugInfoWriter::closeStream();
}



void connectivityMap_ApplicationModule::send2NetworkDataPacket(const char *destID, int data, int pckSeqNumber)
{
	if(!disabled)
	{
		connectivityMap_DataPacket *packet2Net;
		packet2Net = new connectivityMap_DataPacket("Application Packet Application->Mac", APP_DATA_PACKET);
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
		
		//TODO: here the user can control the size of the packet and break it into smaller fragments
		
		send(packet2Net, "toCommunicationModule");
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
	//scheduleAt(simTime()+DRIFTED_TIME(APP_SAMPLE_INTERVAL), new App_ControlMessage("Application self message (request sample)", APP_SELF_REQUEST_SAMPLE));

}


void connectivityMap_ApplicationModule::updateNeighborTable(int nodeID, int theSN /* the Serial Number of the packet */)
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

		if( (theSN >= 0) && (theSN < SENT_PACKETS_PER_NODE_PER_LEVEL) )
			newRec.receivedPackets = 1;

		neighborTable.push_back(newRec);
	}
	else
	{
		neighborTable[pos].timesRx++;

		if( (theSN >= 0) && (theSN < SENT_PACKETS_PER_NODE_PER_LEVEL) )
			neighborTable[pos].receivedPackets++;
	}


}
