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

#ifndef MULTIPATHRINGSROUTINGMODULE
#define MULTIPATHRINGSROUTINGMODULE

#include <map>
#include "VirtualNetworkModule.h"
#include "multipathRingsRoutingFrame_m.h"
#include "multipathRingsRoutingControl_m.h"

#define NO_LEVEL  -110
#define NO_SINK   -120

using namespace std;

enum Timers {
    TOPOLOGY_SETUP_TIMEOUT = 1,
};

class multipathRingsRoutingModule : public VirtualNetworkModule {
    private: 
	int mpathRingsSetupFrameOverhead;	// in bytes
	double netSetupTimeout;
	
	map<string,int> packetFilter;
	
	// multipathRingsRouting-related member variables
	int currentSequenceNumber;
	int currentSinkID;
	int currentLevel;
	int tmpSinkID;
	int tmpLevel;
	bool isSink; 				//is a .ned file parameter of the Application module 
	bool isConnected; 			//attached under a parent node
	bool isScheduledNetSetupTimeout;

    protected:
	void startup();
	void fromApplicationLayer(cPacket *, const char *);
	void fromMacLayer(cPacket *,int,double,double);
	void sendTopologySetupPacket();
	void timerFiredCallback(int);
	void createAndSendControlMessage(multipathRingsRoutingControlDef);
	void processBufferedPacket();
	bool filterIncomingPacket(string,int);
};

#endif //MULTIPATHRINGSROUTINGMODULE

