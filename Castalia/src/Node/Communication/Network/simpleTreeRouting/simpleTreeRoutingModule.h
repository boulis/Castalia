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


#ifndef SIMPLETREEROUTINGMODULE
#define SIMPLETREEROUTINGMODULE

#include "VirtualNetworkModule.h"
#include "simpleTreeRoutingControl_m.h"
#include "simpleTreeRoutingPacket_m.h"


using namespace std;

enum Timers {
    TOPOLOGY_SETUP_TIMEOUT = 1,
};

struct neighborRec {
    int id;
    int parentID;
    int level;
    int sinkID;
    double rssi;
};

class simpleTreeRoutingModule : public VirtualNetworkModule {
    private: 
	int netTreeSetupFrameOverhead;// in bytes
	double netSetupTimeout;
	double topoSetupUdateTimeout;
	int maxNumberOfParents;
	int maxNeighborsTableSize;
	bool rssiBased_NeighborQuality;
	double neighbor_RSSIThreshold;

	bool isSink; //is a .ned file parameter of the Application module 
	int currentSinkID;
	int currentLevel;
	vector <neighborRec> currParents;
	vector <int> parentIDs;
	bool isConnected; //attached under a parent node
	bool isScheduledNetSetupTimeout;
	
	vector <neighborRec> neighborsTable;

    protected:
	void startup();
	void fromApplicationLayer(cPacket*, const char*);
	void fromMacLayer(cPacket*, int, double, double);
	void timerFiredCallback(int);

	int encapsulateAppPacket(cPacket *, simpleTreeRoutingPacket *);
	void simpleTreeRoute_forwardPacket(simpleTreeRoutingPacket *);
	void filterIncomingNetworkDataFrames(simpleTreeRoutingPacket *);
	void decapsulateAndDeliverToApplication(simpleTreeRoutingPacket *);
	void sendTopoSetupMesage(int parSinkID, int parSenderLevel);
	void storeNeighbor(neighborRec parNeighREC, bool rssiBasedQuality);
	void applyNeigborEvictionRule(neighborRec parNeighREC, bool rssiBasedQuality);
	int getBestQualityNeighbors(int bestN, vector <neighborRec> & parResult, bool rssiBasedQuality);
};

bool cmpNeigh_level(neighborRec a, neighborRec b);
bool cmpNeigh_rssi(neighborRec a, neighborRec b);
void destinationCtrlExplode(const string &inString, const string &separator, vector<string> &returnVector);

#endif //SIMPLETREEROUTINGMODULE

