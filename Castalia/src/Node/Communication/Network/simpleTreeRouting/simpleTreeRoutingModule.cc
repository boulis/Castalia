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

#define DRIFTED_TIME(time) ((time) * cpuClockDrift)
//#define EV   ev.isDisabled() ? (ostream&)ev : ev ==> EV is now part of <omnetpp.h>
#define CASTALIA_DEBUG (!printDebugInfo)?(ostream&)DebugInfoWriter::getStream():DebugInfoWriter::getStream()

#define NO_PARENT 65535 // max value for unsigned short
#define NO_LEVEL  -110
#define NO_SINK   -120
#define MOTES_BOOT_TIMEOUT 0.05

#define BIG_INT_NUM 9999999
#define BIG_DBL_NUM 9999999.0

Define_Module(simpleTreeRoutingModule);

void simpleTreeRoutingModule::startup() {
    netSetupTimeout = ((double) par("netSetupTimeout"))/1000.0;
    netTreeSetupFrameOverhead   = par("netTreeSetupFrameOverhead");// in bytes
    topoSetupUdateTimeout = ((double)par("topoSetupUdateTimeout"))/1000.0;
    maxNumberOfParents = par("maxNumberOfParents");
    maxNeighborsTableSize = par("maxNeighborsTableSize");
    rssiBased_NeighborQuality = par("rssiBased_NeighborQuality"); 
    neighbor_RSSIThreshold = par("neighbor_RSSIThreshold");	

    isSink =  gate("toCommunicationModule")->getNextGate()->getOwnerModule()->gate("toApplicationModule")->getNextGate()->getOwnerModule()->par("isSink");

    currentLevel = isSink ? 0 : NO_LEVEL;
    currentSinkID = isSink ? self : NO_SINK;
    isConnected = isSink ? true : false;
    isScheduledNetSetupTimeout = false;

    currParents.clear();
    neighborsTable.clear();
    parentIDs.clear();
    
    if(isSink) sendTopoSetupMesage(self, 0);
}

void simpleTreeRoutingModule::timerFiredCallback(int index) {
    if (index != TOPOLOGY_SETUP_TIMEOUT) return;

    isScheduledNetSetupTimeout = false;

    if(neighborsTable.size() == 0) {
	setTimer(TOPOLOGY_SETUP_TIMEOUT,netSetupTimeout);
	isScheduledNetSetupTimeout = true;
	return;
    }

    vector <neighborRec> selectedParents;
    int bestLevel = getBestQualityNeighbors(maxNumberOfParents, selectedParents, rssiBased_NeighborQuality);
    
    if(selectedParents.size() > 0 && currentLevel == NO_LEVEL) {
	currentLevel = bestLevel+1;
	currParents.swap(selectedParents);
	currentSinkID = ((neighborRec)currParents[0]).sinkID;
	parentIDs.clear();
	for(int i=0; i < (int)currParents.size(); i++) {
	    parentIDs.push_back(((neighborRec)currParents[i]).id);
	}

	// Send NETWORK_2_APP_CONNECTED_2_TREE message to application
	simpleTreeRoutingControlMessage *connectedMsg = new simpleTreeRoutingControlMessage("Node routing connected, Network->Application", NETWORK_CONTROL_MESSAGE);
	connectedMsg->setLevel(currentLevel);
	connectedMsg->setSinkID(currentSinkID);
	string strParentIDs;
	for(int i=0; i < (int)parentIDs.size(); i++) {
	    char buff[512];
	    sprintf(buff, "%i%s", (int)parentIDs[i], ROUTE_DEST_DELIMITER);
	    strParentIDs += buff;
	}
	connectedMsg->setParents(strParentIDs.c_str());
	send(connectedMsg, "toCommunicationModule");

	if(!isConnected) isConnected = true;

	sendTopoSetupMesage(currentSinkID, currentLevel);

	trace() << "Connected to SinkID " << currentSinkID << " \tLevel = " << currentLevel;
    }
}

void simpleTreeRoutingModule::fromApplicationLayer(cPacket *msg, const char * destination) {
    if((int)TXBuffer.size() < netBufferSize) {
//	App_GenericDataPacket *rcvAppDataPacket = check_and_cast<App_GenericDataPacket*>(msg);
	string appPktDest(destination);

	if( (isConnected) || ((appPktDest.compare(PARENT_NETWORK_ADDRESS) != 0) && (appPktDest.compare(SINK_NETWORK_ADDRESS) !=  0)) ) {
	
	    char buff[50];
	    sprintf(buff, "Network Data frame (%f)", SIMTIME_DBL(simTime()));
	    simpleTreeRoutingPacket *newDataFrame = new simpleTreeRoutingPacket(buff, NETWORK_LAYER_PACKET);

	    //create the NetworkFrame from the Application Data Packet (encapsulation)
	    if(encapsulateAppPacket(msg, newDataFrame)) {
		simpleTreeRoute_forwardPacket(newDataFrame);
	    } else {
		cancelAndDelete(newDataFrame);
		newDataFrame = NULL;
		trace() << "WARNING: Application sent to Network an oversized packet...packet dropped!!";
	    }
	} else {
	    simpleTreeRoutingControlMessage *notConnMsg = new simpleTreeRoutingControlMessage("Node is not connected, Network->Application", NETWORK_CONTROL_MESSAGE);
	    send(notConnMsg, "toCommunicationModule");
	}
    } else {
	simpleTreeRoutingControlMessage *fullBuffMsg = new simpleTreeRoutingControlMessage("Network buffer is full Network->Application", NETWORK_CONTROL_MESSAGE);
	send(fullBuffMsg, "toCommunicationModule");
	trace() << "WARNING: SchedTxBuffer FULL!!! Application data packet is discarded";
    }
}

void simpleTreeRoutingModule::fromMacLayer(cPacket *msg, int srcMacAddr, double rssi, double lqi) {
    simpleTreeRoutingPacket *rcvFrame = check_and_cast<simpleTreeRoutingPacket*>(msg);

    int incomingDataPacketType = (int)rcvFrame->getSimpleTreeRoutingPacketKind();
    switch(incomingDataPacketType) {
	
	case ST_ROUTING_DATA_PACKET: {
	    filterIncomingNetworkDataFrames(rcvFrame);
	    break;
	}

	case ST_ROUTING_TOPOLOGY_SETUP_PACKET: {
	    if(isSink) break;
	
	    if(!isScheduledNetSetupTimeout && !isConnected) {
		setTimer(TOPOLOGY_SETUP_TIMEOUT,netSetupTimeout);
		isScheduledNetSetupTimeout = true;
		neighborsTable.clear();
	    }
	
	    neighborRec tmpNeighREC;
	    //tmpNeighREC.id = atoi(setupMsg->getHeader().lastHop.c_str());
	    //tmpNeighREC.sinkID = (int) setupMsg->getSinkID();
	    //tmpNeighREC.level = (int) setupMsg->getSenderLevel();
	    //tmpNeighREC.parentID = (int) setupMsg->getSenderParentID();
	    tmpNeighREC.rssi = rssi;
	    storeNeighbor(tmpNeighREC, rssiBased_NeighborQuality);
	    break;
	}
    }
}

int simpleTreeRoutingModule::encapsulateAppPacket(cPacket *appPacket, simpleTreeRoutingPacket *retFrame)
{
	// Set the ByteLength of the frame 
	int totalMsgLen = appPacket->getByteLength() + netDataFrameOverhead; // the byte-size overhead for a Data packet is fixed (always netDataFrameOverhead)
	if(totalMsgLen > maxNetFrameSize)
		return 0;
	retFrame->setByteLength(netDataFrameOverhead); // extra bytes will be added after the encapsulation
	//App_GenericDataPacket *dupAppPacket = check_and_cast<App_GenericDataPacket *>(appPacket->dup());
	retFrame->encapsulate(appPacket);
	
	
	// --- Set the Simulation-Specific fields of Network_GenericFrame 
	//retFrame->setRssi(0.0);
	//retFrame->setCurrentPathFromSource("");
	
	
	// --- Set the Network_GenericFrame -> header  fields 
	//retFrame->getHeader().frameType = (unsigned short)NET_PROTOCOL_DATA_FRAME; // Set the frameType field
	//retFrame->getHeader().source = appPacket->getHeader().source;
	//retFrame->getHeader().destinationCtrl = appPacket->getHeader().destination; // simply copy the destination of the App_GenericDataPacket
	//retFrame->getHeader().lastHop = strSelfID.c_str();
	//retFrame->getHeader().nextHop = ""; // next-hop is handled by function "routingProtocol_forwardPacket()"
	// No ttl is used (retFrame->getHeader().ttl = ??? )
	
	
	// --- Set the simpleTreeRouting_DataFrame fields
	retFrame->setSinkID((unsigned short)currentSinkID);
	retFrame->setSenderLevel( (short) currentLevel);
//	if(isSink || !isConnected) retFrame->setSenderParentID((unsigned short)NO_PARENT);
//	else if(isConnected) retFrame->setSenderParentID((unsigned short)parentIDs[0]);

	return 1;
}

void simpleTreeRoutingModule::simpleTreeRoute_forwardPacket(simpleTreeRoutingPacket *theMsg) {
	string pckDest(""); //theMsg->getHeader().destinationCtrl.c_str());
	
	// --- Set the Network_GenericFrame -> header->nextHop  field 
	if(pckDest.compare(BROADCAST_NETWORK_ADDRESS) == 0) // this IF statement could be neglected, but we keep it for sake of code descriptiveness
	{
		//theMsg->getHeader().nextHop = BROADCAST;
	}/*
	else
	if(pckDest.find(SINK_NETWORK_ADDRESS) !=  string::npos)
	{
		int totalParents = parentIDs.size();
		
		if(totalParents > 1)
		{
			string destCtrlInfo(SINK_NETWORK_ADDRESS);
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
	
	theMsg->setKind(NETWORK_FRAME);
	
	// Send the frame to MAC module
	TXBuffer.push(theMsg);
							
	scheduleAt(simTime() + epsilon, new simpleTreeRouting_DataFrame("initiate a TX", NETWORK_SELF_CHECK_TX_BUFFER));*/
}

void simpleTreeRoutingModule::filterIncomingNetworkDataFrames(simpleTreeRoutingPacket *rcvFrame) {
//	simpleTreeRouting_DataFrame *rcvFrame = check_and_cast<simpleTreeRouting_DataFrame *>(theFrame);

    string tmpDest(""); //rcvFrame->getHeader().destinationCtrl.c_str());
    int senderLevel = (int)rcvFrame->getSenderLevel();
    int sinkID = (int)rcvFrame->getSinkID();

    if(tmpDest.compare(BROADCAST_NETWORK_ADDRESS) == 0 | tmpDest.compare(SELF_NETWORK_ADDRESS) == 0) {
	// Deliver it to the Application
	decapsulateAndDeliverToApplication(rcvFrame);
    } else if(tmpDest.find(SINK_NETWORK_ADDRESS) !=  string::npos) {
	if(senderLevel == currentLevel+1) {
	    if(self == sinkID) {
		// Simply deliver the message to the Application
		decapsulateAndDeliverToApplication(rcvFrame);
	    } else if(sinkID == currentSinkID) {
		bool selfIsNextHop = false;
		if(tmpDest.find(ROUTE_DEST_DELIMITER) != string::npos) {
		    vector<string> splittedDestCtrl;
		    destinationCtrlExplode(tmpDest, ROUTE_DEST_DELIMITER, splittedDestCtrl);
		
		    for(int i=0; i < (int)splittedDestCtrl.size(); i++) {
			if(((string)splittedDestCtrl[i]).compare(SELF_NETWORK_ADDRESS) == 0)
			    selfIsNextHop = true;
			}
		} 
		//else selfIsNextHop = (strSelfID.compare(rcvFrame->getHeader().nextHop.c_str()) == 0);
		
		if(selfIsNextHop) {
		    //FORWARD THE MESSAGE
		//    simpleTreeRoutingPacket * dupRcvFrame = check_and_cast<simpleTreeRouting_DataFrame *>(rcvFrame->dup());
		    simpleTreeRoute_forwardPacket(rcvFrame);
		}	
	    }
	}
    } else if(tmpDest.compare(PARENT_NETWORK_ADDRESS) ==  0) {
	if(senderLevel == currentLevel+1 && (self == sinkID || sinkID == currentSinkID)) {
	    bool selfIsNextHop = false;
	    if(tmpDest.find(ROUTE_DEST_DELIMITER) != string::npos) {
		vector<string> splittedDestCtrl;
		destinationCtrlExplode(tmpDest, ROUTE_DEST_DELIMITER, splittedDestCtrl);
		
		for(int i=0; i < (int)splittedDestCtrl.size(); i++) {
		    if(((string)splittedDestCtrl[i]).compare(SELF_NETWORK_ADDRESS) == 0)
			selfIsNextHop = true;
		}
	    } 
	    // else selfIsNextHop = (strSelfID.compare(rcvFrame->getHeader().nextHop.c_str()) == 0);
			
	    //printf("\nNode[%i], tmpDest=%s\n", self, tmpDest.c_str());

	    if(selfIsNextHop) {
		// Simply deliver the message to the Application
		decapsulateAndDeliverToApplication(rcvFrame);
	    }
	}
    }
}

void simpleTreeRoutingModule::decapsulateAndDeliverToApplication(simpleTreeRoutingPacket * parFrame2Deliver) {
	//DELIVER THE DATA PACKET TO APPLICATION
//	App_GenericDataPacket *appDataPacket;
	
	//No need to create a new message because of the decapsulation: appDataPacket = new App_GenericDataPacket("Application Packet Network->Application", APP_DATA_PACKET);
	//decaspulate the received Network frame and create a valid App_GenericDataPacket
	
//	appDataPacket = check_and_cast<App_GenericDataPacket *>(parFrame2Deliver->decapsulate());
	
//	appDataPacket->setRssi(parFrame2Deliver->getRssi());
//	appDataPacket->setCurrentPathFromSource(parFrame2Deliver->getCurrentPathFromSource());

	//send the App_GenericDataPacket message to the Application module
	send(parFrame2Deliver->decapsulate(), "toCommunicationModule");
}




void simpleTreeRoutingModule::sendTopoSetupMesage(int parSinkID, int parSenderLevel) {
    char buff[50];
    sprintf(buff, "Node[%i] TopoSetupMessage", self);
    simpleTreeRoutingPacket *setupMsg = new simpleTreeRoutingPacket(buff, NETWORK_LAYER_PACKET);


    // set the byteLength of the frame
    setupMsg->setByteLength(netTreeSetupFrameOverhead);

    // --- Set the Simulation-Specific fields of Network_GenericFrame 
/*
    setupMsg->setRssi(0.0);
    char pathBuff[1024];
    sprintf(pathBuff, "%s%s%s", setupMsg->getCurrentPathFromSource(), ROUTE_DEST_DELIMITER, strSelfID.c_str());
    setupMsg->setCurrentPathFromSource(pathBuff);
*/

    // --- Set the Network_GenericFrame -> header  fields 
/*
    setupMsg->getHeader().frameType = (unsigned short)SIMPLE_TREE_ROUTING_TOPOLOGY_SETUP_FRAME; // Set the frameType field
    setupMsg->getHeader().source = strSelfID.c_str();
    setupMsg->getHeader().destinationCtrl = BROADCAST; // simply copy the destination of the App_GenericDataPacket
    setupMsg->getHeader().lastHop = strSelfID.c_str();
    setupMsg->getHeader().nextHop = BROADCAST;
    // No ttl is used (setupMsg->getHeader().ttl = ??? )
*/

    // --- Set the simpleTreeRouting_DataFrame fields
    setupMsg->setSinkID((unsigned short)parSinkID);
    setupMsg->setSenderLevel((short)parSenderLevel);
//    if(isSink || !isConnected) setupMsg->setSenderParentID((unsigned short)NO_PARENT);
//    else if(isConnected) setupMsg->setSenderParentID((unsigned short)parentIDs[0]);

    toMacLayer(setupMsg,BROADCAST_MAC_ADDRESS);
}


void simpleTreeRoutingModule::storeNeighbor(neighborRec parNeighREC, bool rssiBasedQuality)
{
	int neighIndex = -1;
	neighborRec currRec;
	
	if( (neighbor_RSSIThreshold != 0.0) && (parNeighREC.rssi < neighbor_RSSIThreshold) )
		return;
	
	for(int i=0; i < (int)neighborsTable.size(); i++)
	{
		currRec = (neighborRec)neighborsTable[i];
		
		if(currRec.id == parNeighREC.id)
			neighIndex = i;
	}
	
	if(neighIndex != -1) // if neighbor already exists in the neighborsTable, update the record
		neighborsTable[neighIndex] = parNeighREC;
	else // new neighbor
	{
		if((int)neighborsTable.size() < maxNeighborsTableSize) //if there is enough room to store the new neighbor data
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
	
	for(int i=0; i < (int)neighborsTable.size(); i++)
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

//	if(!rssiBasedQuality) sort( neighborsTable.begin(), neighborsTable.end(), cmpNeigh_level );
//	else sort( neighborsTable.begin(), neighborsTable.end(), cmpNeigh_rssi );
	
	
	currRec = (neighborRec)neighborsTable[0];
	desiredLevel = currRec.level;
	desiredSinkID = currRec.sinkID;
	
	
	for(int i=0; (i < (int)neighborsTable.size()) && (totalRecordsPushed < bestN); i++)
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
