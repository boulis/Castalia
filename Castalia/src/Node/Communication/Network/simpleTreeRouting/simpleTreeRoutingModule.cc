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


#include "simpleTreeRoutingModule.h"

#define NET_BUFFER_ARRAY_SIZE netBufferSize+1

#define BUFFER_IS_EMPTY  (headTxBuffer==tailTxBuffer)

#define BUFFER_IS_FULL  (getTXBufferSize() >= netBufferSize)

#define DRIFTED_TIME(time) ((time) * cpuClockDrift)

#define EV   ev.disabled() ? (ostream&)ev : ev

#define CASTALIA_DEBUG (!printDebugInfo)?(ostream&)DebugInfoWriter::getStream():DebugInfoWriter::getStream()


#define NO_PARENT 65535 // max value for unsigned short
#define NO_LEVEL  -110
#define NO_SINK   -120
#define MOTES_BOOT_TIMEOUT 0.05

#define BIG_INT_NUM 9999999
#define BIG_DBL_NUM 9999999.0


Define_Module(simpleTreeRoutingModule);



void simpleTreeRoutingModule::initialize()
{
	readIniFileParameters();

	//--------------------------------------------------------------------------------
	//------- Follows code for the initialization of the class member variables ------

	self = parentModule()->parentModule()->index();

	//get a valid reference to the object of the Radio module so that we can make direct calls to its public methods
	//instead of using extra messages & message types for tighlty couplped operations.
	radioModule = check_and_cast<RadioModule*>(gate("toMacModule")->toGate()->ownerModule()->gate("toRadioModule")->toGate()->ownerModule());
	radioDataRate = (double) radioModule->par("dataRate");

	macFrameOverhead = gate("toMacModule")->toGate()->ownerModule()->par("macFrameOverhead");

	//get a valid reference to the object of the Resources Manager module so that we can make direct calls to its public methods
	//instead of using extra messages & message types for tighlty couplped operations.
	cModule *parentParent = parentModule()->parentModule();
	if(parentParent->findSubmodule("nodeResourceMgr") != -1)
	{
		resMgrModule = check_and_cast<ResourceGenericManager*>(parentParent->submodule("nodeResourceMgr"));
	}
	else
		opp_error("\n[Network]:\n Error in geting a valid reference to  nodeResourceMgr for direct method calls.");
	cpuClockDrift = resMgrModule->getCPUClockDrift();
	
	schedTXBuffer = new Network_GenericFrame*[NET_BUFFER_ARRAY_SIZE];
	headTxBuffer = 0;
	tailTxBuffer = 0;

	maxSchedTXBufferSizeRecorded = 0;
	
	epsilon = 0.000001;
	
	disabled = 1;
	
	char buff[256];
	sprintf(buff, "%i", self);
	strSelfID.assign(buff);
	
	
	
	/*******  PeriodicBcastTreeRouting-related initializations *************/ 
	// make sure that in your omnetpp.ini you have used an Application module that has the boolean parameter "isSink"
	isSink =  gate("toCommunicationModule")->toGate()->ownerModule()->gate("toApplicationModule")->toGate()->ownerModule()->par("isSink");

	currentLevel = (isSink)?0:NO_LEVEL;
	currentSinkID = (isSink)?self:NO_SINK;
	
	isConnected = (isSink)?true:false;
	
	isScheduledNetSetupTimeout = false;
	
	currParents.clear();
	
	neighborsTable.clear();
	
	parentIDs.clear();
}


void simpleTreeRoutingModule::handleMessage(cMessage *msg)
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
		 * Sent by the Application submodule in order to start/switch-on the Network submodule.
		 *--------------------------------------------------------------------------------------------------------------*/
		case APP_NODE_STARTUP:
		{			
			disabled = 0;
			
			send(new App_ControlMessage("Network --> Mac startup message", APP_NODE_STARTUP), "toMacModule");
			
			//EV << "Node[" << self << "] started! (T=" << simTime() << ")";
			
			// wait for MOTES_BOOT_TIMEOUT and then send the bootstrap message for the formation of the network topology
			if(isSink)	
				scheduleAt(simTime()+MOTES_BOOT_TIMEOUT, new simpleTreeRouting_DataFrame("Sink simulation-specific self-message for Net Topology Setup", NETWORK_SELF_BOOTSTRAP_NET_SETUP));
			
			break;
		}
		
		
		
		case NETWORK_SELF_BOOTSTRAP_NET_SETUP:
		{			
			if(isSink)
			{
				sendTopoSetupMesage(self, 0);
				
				//scheduleAt(simTime()+topoSetupUdateTimeout, new simpleTreeRouting_DataFrame("Sink simulation-specific self-message for Net Topology Setup", NETWORK_SELF_BOOTSTRAP_NET_SETUP));
			}
			
			break;
		}
		
		
		
		case NETWORK_SELF_TOPOLOGY_SETUP_TIMEOUT:
		{		
			isScheduledNetSetupTimeout = false;
			
			if(neighborsTable.size() == 0)
			{	
				scheduleAt(simTime()+netSetupTimeout, new simpleTreeRouting_DataFrame("Simulation-specific self-message for Net topology setup timeout", NETWORK_SELF_TOPOLOGY_SETUP_TIMEOUT));				
				
				isScheduledNetSetupTimeout = true;
			}
			else
			{
				vector <neighborRec> selectedParents;
				int bestLevel = getBestQualityNeighbors(maxNumberOfParents, selectedParents, rssiBased_NeighborQuality);
				
				if( (selectedParents.size() > 0) &&
					( (currentLevel == NO_LEVEL) ) ) // || (currentLevel > bestLevel+1) ) )
				{
					currentLevel = bestLevel+1;
					
					currParents.swap(selectedParents);
					
					currentSinkID = ((neighborRec)currParents[0]).sinkID;
					
					parentIDs.clear();
					for(int i=0; i < currParents.size(); i++)
					{
						parentIDs.push_back(((neighborRec)currParents[i]).id);
					}
					
					
					// Send NETWORK_2_APP_CONNECTED_2_TREE message to application
					simpleTreeRouting_ControlMessage *connectedMsg = new simpleTreeRouting_ControlMessage("Node routing connected, Network->Application", NETWORK_2_APP_CONNECTED_2_TREE);
					connectedMsg->setLevel(currentLevel);
					connectedMsg->setSinkID(currentSinkID);
					string strParentIDs;
					for(int i=0; i < parentIDs.size(); i++)
					{
						char buff[512];
						sprintf(buff, "%i%s", (int)parentIDs[i], ROUTE_DEST_DELIMITER);
						strParentIDs += buff;
					}
					connectedMsg->setParents(strParentIDs.c_str());
					send(connectedMsg, "toCommunicationModule");
						
					
					if(!isConnected)
						isConnected = true;
					
					sendTopoSetupMesage(currentSinkID, currentLevel);
					
					//neighborsTable.clear();
					
					CASTALIA_DEBUG << "Node[" << self << "]-->CONNECTED(T=" << simTime() << "):" << " \tSinkID = " << currentSinkID << " \tLevel = " << currentLevel;
				}
			}

			break;
		}



		/*--------------------------------------------------------------------------------------------------------------
		 * Sent by the Application submodule. We need to push it into the buffer and schedule its forwarding to the MAC buffer for TX.
		 *--------------------------------------------------------------------------------------------------------------*/
		case APP_DATA_PACKET:
		{
			if(!BUFFER_IS_FULL)
			{
				App_GenericDataPacket *rcvAppDataPacket = check_and_cast<App_GenericDataPacket*>(msg);
				string appPktDest(rcvAppDataPacket->getHeader().destination.c_str());
				
				if( (isConnected) || ((appPktDest.compare(PARENT_LEVEL) != 0) && (appPktDest.compare(SINK) !=  0)) )
				{
					
					char buff[50];
					sprintf(buff, "Network Data frame (%f)", simTime());
					simpleTreeRouting_DataFrame *newDataFrame = new simpleTreeRouting_DataFrame(buff, NETWORK_FRAME);
					
					//create the NetworkFrame from the Application Data Packet (encapsulation)
					if(encapsulateAppPacket(rcvAppDataPacket, newDataFrame))
					{										
						simpleTreeRoute_forwardPacket(newDataFrame);
					}
					else
					{
						cancelAndDelete(newDataFrame);
						newDataFrame = NULL;
						CASTALIA_DEBUG << "\n[Network_" << self <<"] t= " << simTime() << ": WARNING: Application sent to Network an oversized packet...packet dropped!!\n";
					}
				}
				else
				{
					simpleTreeRouting_ControlMessage *notConnMsg = new simpleTreeRouting_ControlMessage("Node is not connected, Network->Application", NETWORK_2_APP_NOT_CONNECTED);
					send(notConnMsg, "toCommunicationModule");
				}
				
				rcvAppDataPacket = NULL;
			}
			else
			{				
				simpleTreeRouting_DataFrame *fullBuffMsg = new simpleTreeRouting_DataFrame("Network buffer is full Network->Application", NETWORK_2_APP_FULL_BUFFER);

				send(fullBuffMsg, "toCommunicationModule");
				
				CASTALIA_DEBUG << "\n[Network_" << self <<"] t= " << simTime() << ": WARNING: SchedTxBuffer FULL!!! Application data packet is discarded";
			}
			

			break;
			
		}//END_OF_CASE(APP_DATA_PACKET)



		/*--------------------------------------------------------------------------------------------------------------
		 * Sent by the Network submodule itself to send the next Frame to Radio
		 *--------------------------------------------------------------------------------------------------------------*/
		case NETWORK_SELF_CHECK_TX_BUFFER:
		{	
			
			if(!(BUFFER_IS_EMPTY))
			{
				// SEND THE DATA FRAME TO MAC BUFFER
				Network_GenericFrame *theFrame = popTxBuffer();

				send(theFrame, "toMacModule");
				
				/*if(!(BUFFER_IS_EMPTY))
				{
					double dataTXtime = ((double)(dataFrame->byteLength()+netFrameOverhead) * 8.0 / (1000.0 * radioDataRate));
					
					scheduleAt(simTime() + dataTXtime + epsilon, new Network_ControlMessage("check schedTXBuffer buffer", NETWORK_SELF_CHECK_TX_BUFFER));
				}*/
				
				theFrame = NULL;
			}

			break;
		}



		/*--------------------------------------------------------------------------------------------------------------
		 * Data Frame Received from the Radio submodule (the data frame can be a Data packet or a beacon packet)
		 *--------------------------------------------------------------------------------------------------------------*/
		case NETWORK_FRAME:
		{			
			Network_GenericFrame *rcvFrame;
			rcvFrame = check_and_cast<Network_GenericFrame*>(msg);
			
			//TODO: here we can add code to control the destination of the message and drop packets accordingly
			//rcvFrame->getHeader().applicationID
			
			int incomingDataPacketType = (int)rcvFrame->getHeader().frameType;
			
			switch(incomingDataPacketType)
			{
				case NET_PROTOCOL_DATA_FRAME:
				{
					filterIncomingNetworkDataFrames(rcvFrame);
					
					break;
				}
				
				case SIMPLE_TREE_ROUTING_TOPOLOGY_SETUP_FRAME:
				{
					if(!isSink)
					{
						//EV << "\n---- SIMPLE_TREE_ROUTING_TOPOLOGY_SETUP_FRAME ----\n";
						
						if( ( ! isScheduledNetSetupTimeout ) && (!isConnected) )
						{
							scheduleAt(simTime()+netSetupTimeout, new simpleTreeRouting_ControlMessage("Simulation-specific self-message for Net topology setup timeout", NETWORK_SELF_TOPOLOGY_SETUP_TIMEOUT));
							
							isScheduledNetSetupTimeout = true;
							
							neighborsTable.clear();
						}
						
						simpleTreeRouting_TopoSetup_Frame *setupMsg = check_and_cast<simpleTreeRouting_TopoSetup_Frame *>(rcvFrame);
						
						neighborRec tmpNeighREC;
						tmpNeighREC.id = atoi(setupMsg->getHeader().lastHop.c_str());
						tmpNeighREC.sinkID = (int) setupMsg->getSinkID();
						tmpNeighREC.level = (int) setupMsg->getSenderLevel();
						tmpNeighREC.parentID = (int) setupMsg->getSenderParentID();
						tmpNeighREC.rssi = setupMsg->getRssi();
						
						storeNeighbor(tmpNeighREC, rssiBased_NeighborQuality);
						
						setupMsg = NULL;
					}
					
					break;
				}
				
				default:
				{
					EV << "\n---- UNKOWN NETWORK_FRAME type ----\n";
					break;
				}
			}
			
			rcvFrame = NULL;

			break;
		}


		case MAC_2_NETWORK_FULL_BUFFER:
		{
			/*
			 * TODO:
			 * add your code here to manage the situation of a full MAC buffer.
			 * Apparently we 'll have to stop sending messages and enter into listen or sleep mode (depending on the Network protocol that we implement).
			 */
			 CASTALIA_DEBUG << "\n[Network_"<< self <<"] t= " << simTime() << ": WARNING: MAC_2_NET_FULL_BUFFER received because the MAC buffer is full.\n";
			 break;
		}



		/*--------------------------------------------------------------------------------------------------------------
		 * Message sent by the Resource Manager when battery is out of energy.
		 * Node has to shut down.
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
		
		
		case APPLICATION_2_MAC_SETRADIOTXPOWER: 	case APPLICATION_2_MAC_SETRADIOTXMODE:
		case APPLICATION_2_MAC_SETRADIOSLEEP: 		case APPLICATION_2_MAC_SETRADIOLISTEN:
		case APPLICATION_2_MAC_SETDUTYCYCLE: 		case APPLICATION_2_MAC_SETLISTENINTERVAL:
		case APPLICATION_2_MAC_SETBEACONINTERVALFRACTION: case APPLICATION_2_MAC_SETPROBTX:
		case APPLICATION_2_MAC_SETNUMTX: 		case APPLICATION_2_MAC_SETRNDTXOFFSET:
		case APPLICATION_2_MAC_SETRETXINTERVAL: 	case APPLICATION_2_MAC_SETBACKOFFTYPE:
		case APPLICATION_2_MAC_SETBACKOFFBASEVALUE: case APPLICATION_2_MAC_SETCARRIERSENSE:
		{ 
			App_ControlMessage *appCtrlMsg = check_and_cast<App_ControlMessage*>(msg);
			App_ControlMessage *dupAppCtrlMsg = (App_ControlMessage *) appCtrlMsg->dup();
			send(dupAppCtrlMsg, "toMacModule");
			break;
		}
		
		default:
		{
			CASTALIA_DEBUG << "\n[Network_"<< self <<"] t= " << simTime() << ": WARNING: received packet of unknown type\n";
			
			break;
		}

	}//end_of_switch
	
	delete msg;
	
	msg = NULL;		// safeguard
}



void simpleTreeRoutingModule::finish()
{	
	Network_GenericFrame *netMsg;

	while(!BUFFER_IS_EMPTY)
	{
		netMsg = popTxBuffer();

		cancelAndDelete(netMsg);

	  	netMsg = NULL;
	}
	
	delete [] schedTXBuffer;
	
}


void simpleTreeRoutingModule::readIniFileParameters(void)
{
	printDebugInfo = par("printDebugInfo");

	maxNetFrameSize = par("maxNetFrameSize");

	netBufferSize = par("netBufferSize");
	
	netSetupTimeout = ((double) par("netSetupTimeout"))/1000.0;
	
	netDataFrameOverhead = par("netDataFrameOverhead");
	
	netTreeSetupFrameOverhead   = par("netTreeSetupFrameOverhead");// in bytes
	if(netTreeSetupFrameOverhead > maxNetFrameSize)
		opp_error("Error in value of parameter \"netTreeSetupFrameOverhead\" provided in omnetpp.ini for simpleTreeRoutingModule.");
	
	topoSetupUdateTimeout = ((double)par("topoSetupUdateTimeout"))/1000.0;
	
	maxNumberOfParents = par("maxNumberOfParents");
	
	maxNeighborsTableSize = par("maxNeighborsTableSize");
	
	rssiBased_NeighborQuality = par("rssiBased_NeighborQuality"); 
	
	neighbor_RSSIThreshold = par("neighbor_RSSIThreshold");	
}



int simpleTreeRoutingModule::pushBuffer(Network_GenericFrame *theFrame)
{		
	if(theFrame == NULL)
	{
		CASTALIA_DEBUG << "\n[Network_" << self << "] t= " << simTime() << ": WARNING: Trying to push NULL Network_GenericFrame to the Net_Buffer!!\n";
		
		return 0;
	}

	tailTxBuffer = (++tailTxBuffer)%(NET_BUFFER_ARRAY_SIZE); //increment the tailTxBuffer pointer. If reached the end of array, then start from position [0] of the array

	if (tailTxBuffer == headTxBuffer)
	{
		// reset tail pointer
		if(tailTxBuffer == 0)
			tailTxBuffer = NET_BUFFER_ARRAY_SIZE-1;
		else
			tailTxBuffer--;
		CASTALIA_DEBUG << "\n[Network_" << self << "] t= " << simTime() << ": WARNING: SchedTxBuffer FULL!!! value to be Tx not added to buffer\n";
		
		return 0;
	}

	theFrame->setKind(NETWORK_FRAME);

	if (tailTxBuffer==0)
		schedTXBuffer[NET_BUFFER_ARRAY_SIZE-1] = theFrame;
	else
		schedTXBuffer[tailTxBuffer-1] = theFrame;


	int currLen = getTXBufferSize();
	if (currLen > maxSchedTXBufferSizeRecorded)
		maxSchedTXBufferSizeRecorded = currLen;

	return 1;
}


Network_GenericFrame* simpleTreeRoutingModule::popTxBuffer(void)
{

	if (tailTxBuffer == headTxBuffer) {
		CASTALIA_DEBUG << "\n[Network_" << self << "] \nTrying to pop  EMPTY TxBuffer!!";
		tailTxBuffer--;  // reset tail pointer
		return NULL;
	}

	Network_GenericFrame *pop_message = schedTXBuffer[headTxBuffer];

	headTxBuffer = (++headTxBuffer)%(NET_BUFFER_ARRAY_SIZE);

	return pop_message;
}


int simpleTreeRoutingModule::getTXBufferSize(void)
{
	int size = tailTxBuffer - headTxBuffer;
	if ( size < 0 )
		size += NET_BUFFER_ARRAY_SIZE;

	return size;
}


int simpleTreeRoutingModule::encapsulateAppPacket(App_GenericDataPacket *appPacket, simpleTreeRouting_DataFrame *retFrame)
{
	// Set the ByteLength of the frame 
	int totalMsgLen = appPacket->byteLength() + netDataFrameOverhead; // the byte-size overhead for a Data packet is fixed (always netDataFrameOverhead)
	if(totalMsgLen > maxNetFrameSize)
		return 0;
	retFrame->setByteLength(netDataFrameOverhead); // extra bytes will be added after the encapsulation
	App_GenericDataPacket *dupAppPacket = check_and_cast<App_GenericDataPacket *>(appPacket->dup());
	retFrame->encapsulate(dupAppPacket);
	
	
	// --- Set the Simulation-Specific fields of Network_GenericFrame 
	retFrame->setRssi(0.0);
	retFrame->setCurrentPathFromSource("");
	
	
	// --- Set the Network_GenericFrame -> header  fields 
	retFrame->getHeader().frameType = (unsigned short)NET_PROTOCOL_DATA_FRAME; // Set the frameType field
	retFrame->getHeader().source = appPacket->getHeader().source;
	retFrame->getHeader().destinationCtrl = appPacket->getHeader().destination; // simply copy the destination of the App_GenericDataPacket
	retFrame->getHeader().lastHop = strSelfID.c_str();
	retFrame->getHeader().nextHop = ""; // next-hop is handled by function "routingProtocol_forwardPacket()"
	// No ttl is used (retFrame->getHeader().ttl = ??? )
	
	
	// --- Set the simpleTreeRouting_DataFrame fields
	retFrame->setSinkID((unsigned short)currentSinkID);
	retFrame->setSenderLevel( (short) currentLevel);
	if((isSink) || (!isConnected))
		retFrame->setSenderParentID((unsigned short)NO_PARENT);
	else
	if(isConnected)
		retFrame->setSenderParentID((unsigned short)parentIDs[0]);

	return 1;
}





void simpleTreeRoutingModule::simpleTreeRoute_forwardPacket(simpleTreeRouting_DataFrame *theMsg)
{	
	string pckDest(theMsg->getHeader().destinationCtrl.c_str());
	
	// --- Set the Network_GenericFrame -> header->nextHop  field 
	if(pckDest.compare(BROADCAST) == 0) // this IF statement could be neglected, but we keep it for sake of code descriptiveness
	{
		theMsg->getHeader().nextHop = BROADCAST;
	}
	else
	if(pckDest.find(SINK) !=  string::npos)
	{
		int totalParents = parentIDs.size();
		
		if(totalParents > 1)
		{
			string destCtrlInfo(SINK);
			for(int i=0; i < totalParents; i++)
			{
				char destCtrlBuff[512];
				sprintf(destCtrlBuff, "%s%i", ROUTE_DEST_DELIMITER, (int)parentIDs[i]);
				destCtrlInfo += destCtrlBuff;
			}
			 
			theMsg->getHeader().destinationCtrl = destCtrlInfo.c_str();
			
			theMsg->getHeader().nextHop = BROADCAST;
		}
		else
		{
			char nextHopBuff[512];
			sprintf(nextHopBuff, "%i", (int)parentIDs[0]);
			
			theMsg->getHeader().nextHop = nextHopBuff;
			
			theMsg->getHeader().destinationCtrl = SINK;
		}
	}
	else
	if(pckDest.find(PARENT_LEVEL) != string::npos)
	{
		int totalParents = parentIDs.size();
		
		if(totalParents > 1)
		{			
			string destCtrlInfo(PARENT_LEVEL);
			for(int i=0; i < totalParents; i++)
			{
				char destCtrlBuff[512];
				sprintf(destCtrlBuff, "%s%i", ROUTE_DEST_DELIMITER, (int)parentIDs[i]);
				destCtrlInfo += destCtrlBuff;
			}
			 
			theMsg->getHeader().destinationCtrl = destCtrlInfo.c_str();
			
			theMsg->getHeader().nextHop = BROADCAST;
		}
		else
		{
			char nextHopBuff[512];
			sprintf(nextHopBuff, "%i", (int)parentIDs[0]);
			
			theMsg->getHeader().nextHop = nextHopBuff;
			
			theMsg->getHeader().destinationCtrl = PARENT_LEVEL;
		}
	}
	else
	{		
		theMsg->getHeader().nextHop = pckDest.c_str();
	}
	
	
	// --- Set the Network_GenericFrame -> header->lastHop  field
	theMsg->getHeader().lastHop = strSelfID.c_str();
	
	
	// --- Set the Simulation-Specific field currentPathFromSource of Network_GenericFrame 
	char pathBuff[1024];
	sprintf(pathBuff, "%s%s%s", theMsg->getCurrentPathFromSource(), ROUTE_DEST_DELIMITER, strSelfID.c_str());
	theMsg->setCurrentPathFromSource(pathBuff);
	
//printf("\nNode[%i], path=%s\n", self, theMsg->getCurrentPathFromSource());
	
	// --- Set the simpleTreeRouting_DataFrame fields
	// No need to set the SinkID again, because simpleTreeRoute_forwardPacket() is
	// called only if currentSinkID == theMsg->sinkID
	// or it has already been set, by the encapsulateAppPacket()
	// theMsg->setSinkID((unsigned short)currentSinkID);
	theMsg->setSenderLevel((short)currentLevel);
	if((isSink) || (!isConnected))
		theMsg->setSenderParentID((unsigned short)NO_PARENT);
	else
	if(isConnected)
		theMsg->setSenderParentID((unsigned short)parentIDs[0]);
	
		
	// Send the frame to MAC module
	pushBuffer(theMsg);
						
	scheduleAt(simTime() + epsilon, new simpleTreeRouting_DataFrame("initiate a TX", NETWORK_SELF_CHECK_TX_BUFFER));
}





void simpleTreeRoutingModule::filterIncomingNetworkDataFrames(Network_GenericFrame *theFrame)
{	
	simpleTreeRouting_DataFrame *rcvFrame = check_and_cast<simpleTreeRouting_DataFrame *>(theFrame);
	
	string tmpDest(rcvFrame->getHeader().destinationCtrl.c_str());
	int senderLevel = (int)rcvFrame->getSenderLevel();
	int sinkID = (int)rcvFrame->getSinkID();
	
	if( (tmpDest.compare(BROADCAST) == 0) | 
		(tmpDest.compare(strSelfID) == 0) )
	{
		// Deliver it to the Application
		decapsulateAndDeliverToApplication(rcvFrame);
	}
	else
	if(tmpDest.find(SINK) !=  string::npos)
	{		
		if(senderLevel == currentLevel+1)
		{			
			if(self == sinkID)
			{
				// Simply deliver the message to the Application
				decapsulateAndDeliverToApplication(rcvFrame);
			}
			else
			if(sinkID == currentSinkID)
			{
				bool selfIsNextHop = false;
				
				if(tmpDest.find(ROUTE_DEST_DELIMITER) != string::npos)
				{
					vector<string> splittedDestCtrl;
					destinationCtrlExplode(tmpDest, ROUTE_DEST_DELIMITER, splittedDestCtrl);
					
					for(int i=0; i < splittedDestCtrl.size(); i++)
					{
						if(((string)splittedDestCtrl[i]).compare(strSelfID) == 0)
							selfIsNextHop = true;
					}
				}
				else
					selfIsNextHop = (strSelfID.compare(rcvFrame->getHeader().nextHop.c_str()) == 0);
				
				if(selfIsNextHop)
				{
					//FORWARD THE MESSAGE
					simpleTreeRouting_DataFrame * dupRcvFrame = check_and_cast<simpleTreeRouting_DataFrame *>(rcvFrame->dup());
					
					simpleTreeRoute_forwardPacket(dupRcvFrame);
				}	
			}
		}
	}
	else
	if(tmpDest.compare(PARENT_LEVEL) ==  0)
	{		
		if( (senderLevel == currentLevel+1) &&
			((self == sinkID) || (sinkID == currentSinkID)) )
		{
			bool selfIsNextHop = false;
				
			if(tmpDest.find(ROUTE_DEST_DELIMITER) != string::npos)
			{
				vector<string> splittedDestCtrl;
				destinationCtrlExplode(tmpDest, ROUTE_DEST_DELIMITER, splittedDestCtrl);
				
				for(int i=0; i < splittedDestCtrl.size(); i++)
				{
					if(((string)splittedDestCtrl[i]).compare(strSelfID) == 0)
						selfIsNextHop = true;
				}
				
			}
			else
				selfIsNextHop = (strSelfID.compare(rcvFrame->getHeader().nextHop.c_str()) == 0);
			
//printf("\nNode[%i], tmpDest=%s\n", self, tmpDest.c_str());
			
			if(selfIsNextHop)
			{
				// Simply deliver the message to the Application
				decapsulateAndDeliverToApplication(rcvFrame);
			}
		}
			
					
	}
}




void simpleTreeRoutingModule::decapsulateAndDeliverToApplication(simpleTreeRouting_DataFrame * parFrame2Deliver)
{
	//DELIVER THE DATA PACKET TO APPLICATION
	App_GenericDataPacket *appDataPacket;
	
	//No need to create a new message because of the decapsulation: appDataPacket = new App_GenericDataPacket("Application Packet Network->Application", APP_DATA_PACKET);
	//decaspulate the received Network frame and create a valid App_GenericDataPacket
	
	appDataPacket = check_and_cast<App_GenericDataPacket *>(parFrame2Deliver->decapsulate());
	
	appDataPacket->setRssi(parFrame2Deliver->getRssi());
	appDataPacket->setCurrentPathFromSource(parFrame2Deliver->getCurrentPathFromSource());

	//send the App_GenericDataPacket message to the Application module
	send(appDataPacket, "toCommunicationModule");
}




void simpleTreeRoutingModule::sendTopoSetupMesage(int parSinkID, int parSenderLevel)
{
	char buff[50];
	sprintf(buff, "Node[%i] TopoSetupMessage", self);
	simpleTreeRouting_TopoSetup_Frame *setupMsg = new simpleTreeRouting_TopoSetup_Frame(buff, NETWORK_FRAME);
	
	
	// set the byteLength of the frame
	setupMsg->setByteLength(netTreeSetupFrameOverhead);
	
	
	// --- Set the Simulation-Specific fields of Network_GenericFrame 
	setupMsg->setRssi(0.0);
	char pathBuff[1024];
	sprintf(pathBuff, "%s%s%s", setupMsg->getCurrentPathFromSource(), ROUTE_DEST_DELIMITER, strSelfID.c_str());
	setupMsg->setCurrentPathFromSource(pathBuff);
	
	
	// --- Set the Network_GenericFrame -> header  fields 
	setupMsg->getHeader().frameType = (unsigned short)SIMPLE_TREE_ROUTING_TOPOLOGY_SETUP_FRAME; // Set the frameType field
	setupMsg->getHeader().source = strSelfID.c_str();
	setupMsg->getHeader().destinationCtrl = BROADCAST; // simply copy the destination of the App_GenericDataPacket
	setupMsg->getHeader().lastHop = strSelfID.c_str();
	setupMsg->getHeader().nextHop = BROADCAST;
	// No ttl is used (setupMsg->getHeader().ttl = ??? )
	
	
	// --- Set the simpleTreeRouting_DataFrame fields
	setupMsg->setSinkID((unsigned short)parSinkID);
	setupMsg->setSenderLevel((short)parSenderLevel);
	if((isSink) || (!isConnected))
		setupMsg->setSenderParentID((unsigned short)NO_PARENT);
	else
	if(isConnected)
		setupMsg->setSenderParentID((unsigned short)parentIDs[0]);
	
	
	pushBuffer(setupMsg);
	
	scheduleAt(simTime() + epsilon, new simpleTreeRouting_DataFrame("initiate a TX", NETWORK_SELF_CHECK_TX_BUFFER));
}


void simpleTreeRoutingModule::storeNeighbor(neighborRec parNeighREC, bool rssiBasedQuality)
{
	int neighIndex = -1;
	neighborRec currRec;
	
	if( (neighbor_RSSIThreshold != 0.0) && (parNeighREC.rssi < neighbor_RSSIThreshold) )
		return;
	
	for(int i=0; i < neighborsTable.size(); i++)
	{
		currRec = (neighborRec)neighborsTable[i];
		
		if(currRec.id == parNeighREC.id)
			neighIndex = i;
	}
	
	if(neighIndex != -1) // if neighbor already exists in the neighborsTable, update the record
		neighborsTable[neighIndex] = parNeighREC;
	else // new neighbor
	{
		if(neighborsTable.size() < maxNeighborsTableSize) //if there is enough room to store the new neighbor data
			neighborsTable.push_back(parNeighREC);
		else //delete the less important neighbor and store the new one, it the former worths more that
			applyNeigborEvictionRule(parNeighREC, rssiBasedQuality);
			
	}
}

void simpleTreeRoutingModule::applyNeigborEvictionRule(neighborRec parNeighREC, bool rssiBasedQuality)
{
	int maxLevel = -BIG_INT_NUM;
	double maxLevel_RSSI = BIG_DBL_NUM;
	
	double minRSSI = BIG_DBL_NUM;
	int minRSSI_level = -BIG_INT_NUM;
	
	int deleteIndex = -1;
	neighborRec currRec;
	
	for(int i=0; i < neighborsTable.size(); i++)
	{
		currRec = (neighborRec)neighborsTable[i];
		
		if(!rssiBasedQuality)
		{
			if( (currRec.level > maxLevel) ||
		    ( (currRec.level == maxLevel) && (currRec.rssi < maxLevel_RSSI) ) )
			{
				maxLevel = currRec.level;
				maxLevel_RSSI = currRec.rssi;
				deleteIndex = i;
			}
		}
		else //rssiBasedQuality
		{
			if( (currRec.rssi < minRSSI) ||
			    ( (currRec.rssi == minRSSI) && (currRec.level > minRSSI_level) ) )
			{
				minRSSI = currRec.rssi;
				minRSSI_level = currRec.level;
				deleteIndex = i;
			}
		}
		

	}
	
	if(deleteIndex == -1)
		return;
	
	if( (parNeighREC.level <= maxLevel))
    {
    	vector<neighborRec>::iterator eraseLocation = neighborsTable.begin();
    	
    	eraseLocation += deleteIndex;
    	
    	neighborsTable.erase(eraseLocation);
    	
    	neighborsTable.push_back(parNeighREC);
    }
	
	
}


int simpleTreeRoutingModule::getBestQualityNeighbors(int bestN, vector <neighborRec> & parResult, bool rssiBasedQuality)
{
	neighborRec currRec;
	int totalRecordsPushed = 0;
	int desiredLevel = -1;
	int desiredSinkID = -1;

	if(!rssiBasedQuality)
		sort( neighborsTable.begin(), neighborsTable.end(), cmpNeigh_level );
	else //rssiBasedQuality
		sort( neighborsTable.begin(), neighborsTable.end(), cmpNeigh_rssi );
	
	
	currRec = (neighborRec)neighborsTable[0];
	desiredLevel = currRec.level;
	desiredSinkID = currRec.sinkID;
	
	
	for(int i=0; (i < neighborsTable.size()) && (totalRecordsPushed < bestN); i++)
	{
		currRec = (neighborRec)neighborsTable[i];
		
		parResult.push_back(currRec);
		
		totalRecordsPushed++;
		
		/*if( (currRec.level == desiredLevel) && (currRec.sinkID == desiredSinkID) )
		{	
			parResult.push_back(currRec);
			totalRecordsPushed++;
		}*/
	}
	
	return desiredLevel;
}


bool cmpNeigh_level(neighborRec a, neighborRec b)
{
	if( a.level == b.level )
	{
		return (a.rssi > b.rssi);
	}
	else
		return (a.level < b.level);
}


bool cmpNeigh_rssi(neighborRec a, neighborRec b)
{
	if( a.rssi == b.rssi )
	{
		return (a.level < b.level);
	}
	else
		return (a.rssi > b.rssi);
}


void destinationCtrlExplode(const string &inString, const string &separator, vector<string> &returnVector)
{
	returnVector.clear();
	
	string::size_type start = 0;
	string::size_type end = 0;
	
	while ((end = inString.find (separator, start)) != string::npos)
	{
		returnVector.push_back(inString.substr(start, end-start));
		
		start = end + separator.size();
	}
	returnVector.push_back (inString.substr (start));
}
