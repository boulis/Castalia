//***************************************************************************************
//*  Copyright: National ICT Australia,  2007 - 2010					*
//*  Developed at the Networks and Pervasive Computing program				*
//*  Author(s): Dimosthenis Pediaditakis, Yuriy Tselishchev				*
//*  This file is distributed under the terms in the attached LICENSE file.		*
//*  If you do not find this file, copies can be found by writing to:			*
//*											*
//*      NICTA, Locked Bag 9013, Alexandria, NSW 1435, Australia			*
//*      Attention:  License Inquiry.							*
//*											*
//***************************************************************************************


#include "multipathRingsRoutingModule.h"

Define_Module(multipathRingsRoutingModule);

void multipathRingsRoutingModule::startup() {
    netSetupTimeout = (double)par("netSetupTimeout")/1000.0;
    mpathRingsSetupFrameOverhead = par("mpathRingsSetupFrameOverhead");

    // make sure that in your omnetpp.ini you have used an Application module that has the boolean parameter "isSink"
    isSink =  gate("toCommunicationModule")->getNextGate()->getOwnerModule()->gate("toApplicationModule")->getNextGate()->getOwnerModule()->par("isSink");
    currentLevel = tmpLevel = isSink ? 0 : NO_LEVEL;
    currentSinkID = tmpSinkID = isSink ? self : NO_SINK;

    isConnected = (isSink) ? true : false;
    isScheduledNetSetupTimeout = false;
    currentSequenceNumber = 0;
    packetFilter.clear();

    if (isSink) sendTopologySetupPacket();
}

void multipathRingsRoutingModule::sendTopologySetupPacket() {
    multipathRingsRoutingPacket *setupPkt = new  multipathRingsRoutingPacket("Multipath rings routing packet",NETWORK_LAYER_PACKET);

    setupPkt->setMultipathRingsRoutingPacketKind(MPRINGS_TOPOLOGY_SETUP_PACKET);
    setupPkt->setSource(SELF_NETWORK_ADDRESS);
    setupPkt->setDestination(BROADCAST_NETWORK_ADDRESS);
    setupPkt->setSinkID(currentSinkID);
    setupPkt->setSenderLevel(currentLevel);

    toMacLayer(setupPkt,BROADCAST_MAC_ADDRESS);
}


void multipathRingsRoutingModule::timerFiredCallback(int index) {
    if (index != TOPOLOGY_SETUP_TIMEOUT) return;
    
    isScheduledNetSetupTimeout = false;
    if(tmpLevel == NO_LEVEL) {
	setTimer(TOPOLOGY_SETUP_TIMEOUT,netSetupTimeout);
	isScheduledNetSetupTimeout = true;
    } else if( (currentLevel == NO_LEVEL) ) {
	//Broadcast to all nodes of currentLevel-1
	currentLevel = tmpLevel+1;
	currentSinkID = tmpSinkID;
	
	if(!isConnected) {
	    isConnected = true;
	    createAndSendControlMessage(CONNECTED_TO_TREE);
	    trace() << "Connected to " << currentSinkID << " at level = " << currentLevel;
	    if (!TXBuffer.empty()) processBufferedPacket();
	} else {
	    createAndSendControlMessage(TREE_LEVEL_UPDATED);
	    trace() << "Changed Level, new level is \"" << currentLevel << "\"";
	}

	sendTopologySetupPacket();
    }

    tmpLevel = (isSink)?0:NO_LEVEL;
    tmpSinkID = (isSink)?self:NO_SINK;
}

void multipathRingsRoutingModule::processBufferedPacket() {
    while (!TXBuffer.empty()) {
	toMacLayer(TXBuffer.front(),BROADCAST_MAC_ADDRESS);
	TXBuffer.pop();
    }
}

void multipathRingsRoutingModule::fromApplicationLayer(cPacket * pkt, const char * destination) {
    string dst(destination);

    multipathRingsRoutingPacket *netPacket = new multipathRingsRoutingPacket("Multipath rings routing packet",NETWORK_LAYER_PACKET);

    netPacket->setMultipathRingsRoutingPacketKind(MPRINGS_DATA_PACKET);
    netPacket->setSource(SELF_NETWORK_ADDRESS);
    netPacket->setDestination(destination);
    netPacket->setSinkID(currentSinkID);
    netPacket->setSenderLevel(currentLevel);

    encapsulatePacket(netPacket,pkt);

    if (dst.compare(SINK_NETWORK_ADDRESS) == 0 || dst.compare(PARENT_NETWORK_ADDRESS) == 0) {
	netPacket->setSequenceNumber(currentSequenceNumber);
	currentSequenceNumber++;
	if (bufferPacket(netPacket)) {
	    if (isConnected) {
		processBufferedPacket();
	    } else {
		createAndSendControlMessage(NOT_CONNECTED);
	    }
	} else {
	    cancelAndDelete(netPacket);
	    //Here we could send a control message to upper layer informing that our buffer is full
	}
    } else {
	toMacLayer(netPacket, BROADCAST_MAC_ADDRESS);
    }
}

void multipathRingsRoutingModule::fromMacLayer(cPacket *pkt, int macAddress, double rssi, double lqi) {
    multipathRingsRoutingPacket *netPacket = dynamic_cast<multipathRingsRoutingPacket*>(pkt);
    if (!netPacket) {
	delete pkt;
	return;
    }
    
    int kind = netPacket->getMultipathRingsRoutingPacketKind();
    
    switch (kind) {
    
	case MPRINGS_TOPOLOGY_SETUP_PACKET: {
	    if (isSink) break;
	    if (!isScheduledNetSetupTimeout) {
		isScheduledNetSetupTimeout = true;
		setTimer(TOPOLOGY_SETUP_TIMEOUT,netSetupTimeout);
		tmpLevel = NO_LEVEL;
		tmpSinkID = NO_SINK;
	    }
	    if (tmpLevel == NO_LEVEL || tmpLevel > netPacket->getSenderLevel()) {
		tmpLevel = netPacket->getSenderLevel();
		tmpSinkID = netPacket->getSinkID();
	    }
	    break;
	}
	
	case MPRINGS_DATA_PACKET: {
	    string dst(netPacket->getDestination());
	    string src(netPacket->getSource());
	    int senderLevel = netPacket->getSenderLevel();
	    int sinkID = netPacket->getSinkID();
	    int seq = netPacket->getSequenceNumber();

	    if (dst.compare(BROADCAST_NETWORK_ADDRESS) == 0 ||
		    dst.compare(SELF_NETWORK_ADDRESS) == 0) {
		//We are not filtering packets that are sent to this node directly or to broadcast address
		toApplicationLayer(pkt->decapsulate());

	    } else if (dst.compare(SINK_NETWORK_ADDRESS) == 0) {
		if (senderLevel == currentLevel+1) {
		    if (self == sinkID) {
			if (filterIncomingPacket(src,seq)) toApplicationLayer(pkt->decapsulate());
		    } else if (sinkID == currentSinkID) {
			toMacLayer(netPacket,BROADCAST_MAC_ADDRESS);
			return;
		    }
		}

	    } else if (dst.compare(PARENT_NETWORK_ADDRESS) == 0) {
		if (senderLevel == currentLevel+1 && sinkID == currentSinkID) {
		    if (filterIncomingPacket(src,seq)) toApplicationLayer(pkt->decapsulate());
		}
	    }
	    break;
	}
    }
    
    delete pkt;
}

void multipathRingsRoutingModule::createAndSendControlMessage(multipathRingsRoutingControlDef kind) {
    multipathRingsRoutingControlMessage *connectedMsg = new multipathRingsRoutingControlMessage("Multipath routing control message, Network->Application", NETWORK_CONTROL_MESSAGE);
    connectedMsg->setMultipathRingsRoutingControlMessageKind(kind);
    connectedMsg->setLevel(currentLevel);
    connectedMsg->setSinkID(currentSinkID);
    toApplicationLayer(connectedMsg);
}

bool multipathRingsRoutingModule::filterIncomingPacket(string source, int seq) {
    if (packetFilter[source] >= seq) return false;
    packetFilter[source] = seq;
    return true;
}


