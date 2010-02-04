//***************************************************************************************
//*  Copyright: National ICT Australia,  2007, 2008, 2009, 2010				*
//*  Developed at the Networks and Pervasive Computing program				*
//*  Author(s): Athanassios Boulis, Dimosthenis Pediaditakis, Yuriy Tselishchev		*
//*  This file is distributed under the terms in the attached LICENSE file.		*
//*  If you do not find this file, copies can be found by writing to:			*
//*											*
//*      NICTA, Locked Bag 9013, Alexandria, NSW 1435, Australia			*
//*      Attention:  License Inquiry.							*
//*											*
//***************************************************************************************


#include "connectivityMap_ApplicationModule.h"

Define_Module(connectivityMap_ApplicationModule);

void connectivityMap_ApplicationModule::startup() {

    printConnMap = par("printConnectivityMap");
    packetSpacing = (double)par("packetSpacing")/1000.0;
    packetsPerNode = par("packetsPerNode");
    packetSize = par("packetSize");
    
    neighborTable.clear();
    packetsSent = 0;
    totalSNnodes = getParentModule()->getParentModule()->par("numNodes");

    txInterval_perNode = packetsPerNode * packetSpacing;
    txInterval_total = (txInterval_perNode * totalSNnodes);

    if (strtod(ev.getConfig()->getConfigValue("sim-time-limit"),NULL) < txInterval_total) {
	trace() << "ERROR: Total sim time should be at least = " << txInterval_total;
	opp_error("\nError: simulation time not large enough for the conectivity map application\n");
    }

    double startTxTime = txInterval_perNode * self;
    setTimer(SEND_PACKET,startTxTime);
}

void connectivityMap_ApplicationModule::fromNetworkLayer(ApplicationGenericDataPacket* rcvPacket, const char * source, const char * path, double rssi, double lqi) {
    double theData = rcvPacket->getData(); // should always be 0, but we wont check
    int sequenceNumber = rcvPacket->getSequenceNumber();

    updateNeighborTable(atoi(source),sequenceNumber);
}

void connectivityMap_ApplicationModule::timerFiredCallback(int timerIndex) {
    switch (timerIndex) {
	case SEND_PACKET: {
	    if (packetsSent < packetsPerNode) {
		toNetworkLayer(createGenericDataPacket(0.0,packetsSent,packetSize),BROADCAST_NETWORK_ADDRESS);
		packetsSent++;
		setTimer(SEND_PACKET,packetSpacing);
	    }
	    break;
	}
    }
}

void connectivityMap_ApplicationModule::finishSpecific() {
    EV << "\n\n** Node [" << self << "] received from:\n";
    for(int i=0; i < (int)neighborTable.size(); i++) {
	declareOutput("Packets received",neighborTable[i].id);
	collectOutput("Packets received",neighborTable[i].id,"Success",neighborTable[i].receivedPackets);
	collectOutput("Packets received",neighborTable[i].id,"Fail",packetsPerNode-neighborTable[i].receivedPackets);
	EV << "[" << self << "<--" << neighborTable[i].id << "]\t -->\t " << neighborTable[i].receivedPackets << "\t out of \t" << packetsPerNode << "\n";
    }
}

void connectivityMap_ApplicationModule::updateNeighborTable(int nodeID, int theSN) {
    int i=0, pos = -1;
    int tblSize = (int)neighborTable.size();

    for(i=0; i < tblSize; i++)
	if(neighborTable[i].id == nodeID) pos = i;

    if(pos == -1) {
	neighborRecord newRec;
	newRec.id = nodeID;
	newRec.timesRx = 1;

	if( (theSN >= 0) && (theSN < packetsPerNode) )
	    newRec.receivedPackets = 1;

	neighborTable.push_back(newRec);
    } else {
	neighborTable[pos].timesRx++;

	if( (theSN >= 0) && (theSN < packetsPerNode) )
	    neighborTable[pos].receivedPackets++;
    }
}
