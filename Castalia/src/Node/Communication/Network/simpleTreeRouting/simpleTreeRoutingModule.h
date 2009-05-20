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

#include <vector>
#include <algorithm>
#include <omnetpp.h>
#include "App_GenericDataPacket_m.h"
#include "App_ControlMessage_m.h"
#include "simpleTreeRoutingControlMessage_m.h"
#include "simpleTreeRoutingFrame_m.h"
#include "MacGenericFrame_m.h"
#include "MacControlMessage_m.h"
#include "ResourceGenericManager.h"
#include "RadioModule.h"
#include "DebugInfoWriter.h"
using namespace std;

struct neighborRec
{
	int id;
	int parentID;
	int level;
	int sinkID;
	double rssi;
};

class simpleTreeRoutingModule : public cSimpleModule 
{
	private: 
	// parameters and variables
		
		/*--- The .ned file's parameters ---*/
		int maxNetFrameSize;	//in bytes
		int netDataFrameOverhead;	//in bytes
		int netBufferSize;	//in # of messages
		int macFrameOverhead;
		bool printDebugInfo;
		
		int netTreeSetupFrameOverhead;// in bytes
		double netSetupTimeout;
		double topoSetupUdateTimeout;
		int maxNumberOfParents;
		int maxNeighborsTableSize;
		bool rssiBased_NeighborQuality;
		double neighbor_RSSIThreshold;
		
		/*--- Custom class parameters ---*/
		int self;					// the node's ID
		RadioModule *radioModule;	//a pointer to the object of the Radio Module (used for direct method calls)
		double radioDataRate;
		ResourceGenericManager *resMgrModule;	//a pointer to the object of the Radio Module (used for direct method calls)
		// a buffer that can hold up to 9 values to get trasmitted
		Network_GenericFrame **schedTXBuffer;		
		int headTxBuffer;
		int tailTxBuffer;
		//used to keep/manage the order between two messages that are sent at the same simulation time
		double epsilon;				
		int maxSchedTXBufferSizeRecorded;
		double cpuClockDrift;
		int disabled;
		string strSelfID;
		
		// simpleTreeRouting-related member variables
		bool isSink; //is a .ned file parameter of the Application module 
		int currentSinkID;
		int currentLevel;
		vector <neighborRec> currParents;
		vector <int> parentIDs;
		bool isConnected; //attached under a parent node
		bool isScheduledNetSetupTimeout;
		
		vector <neighborRec> neighborsTable;		

		
	protected:
		virtual void initialize();
		virtual void handleMessage(cMessage *msg);
		virtual void finish();
		void readIniFileParameters(void);
		int encapsulateAppPacket(App_GenericDataPacket *appPacket, simpleTreeRouting_DataFrame *retFrame);
		int pushBuffer(Network_GenericFrame *theMsg);
		Network_GenericFrame* popTxBuffer(void);
		int getTXBufferSize(void);
		void simpleTreeRoute_forwardPacket(simpleTreeRouting_DataFrame *theMsg);
		void filterIncomingNetworkDataFrames(Network_GenericFrame *theFrame);
		void decapsulateAndDeliverToApplication(simpleTreeRouting_DataFrame * parFrame2Deliver);
		void sendTopoSetupMesage(int parSinkID, int parSenderLevel);
		
		void storeNeighbor(neighborRec parNeighREC, bool rssiBasedQuality);
		void applyNeigborEvictionRule(neighborRec parNeighREC, bool rssiBasedQuality);
		int getBestQualityNeighbors(int bestN, vector <neighborRec> & parResult, bool rssiBasedQuality);
};

bool cmpNeigh_level(neighborRec a, neighborRec b);
bool cmpNeigh_rssi(neighborRec a, neighborRec b);
void destinationCtrlExplode(const string &inString, const string &separator, vector<string> &returnVector);

#endif //SIMPLETREEROUTINGMODULE

