//***************************************************************************************
//*  Copyright: National ICT Australia, 2009, 2010					*
//*  Developed at the Networks and Pervasive Computing program				*
//*  Author(s): Yuriy Tselishchev							*
//*  This file is distributed under the terms in the attached LICENSE file.		*
//*  If you do not find this file, copies can be found by writing to:			*
//*											*
//*      NICTA, Locked Bag 9013, Alexandria, NSW 1435, Australia			*
//*      Attention:  License Inquiry.							*
//*											*
//***************************************************************************************


#ifndef VIRTUALMACMODULE
#define VIRTUALMACMODULE

#include <queue>
#include <map>
#include <omnetpp.h>

#include "App_ControlMessage_m.h"
#include "NetworkGenericFrame_m.h"
#include "NetworkControlMessage_m.h"
#include "MacGenericFrame_m.h"
#include "MacControlMessage_m.h"
#include "RadioControlMessage_m.h"
#include "ResourceGenericManager.h"
#include "RadioModule.h"

#include "VirtualCastaliaModule.h"

using namespace std;

class VirtualMacModule : public VirtualCastaliaModule
{
    private:
	double cpuClockDrift;
	int disabled;
	MAC_ControlMessage *selfCarrierSenseMsg;

    protected:
	int self;				// the node's ID
    	RadioModule *radioModule;		//a pointer to the object of the Radio Module (used for direct method calls)
	ResourceGenericManager *resMgrModule;	//a pointer to the object of the Resource Manager Module (used for direct method calls)
	queue <MAC_GenericFrame *> TXBuffer;
	map <int, MAC_ControlMessage *> timerMessages;
		
	void initialize();
	void handleMessage(cMessage *msg);
	void finish();
	virtual void startup();
	
	void toNetworkLayer(MAC_GenericFrame*);
	void toRadioLayer(MAC_GenericFrame*);
	virtual void fromNetworkLayer(MAC_GenericFrame*) = 0;
	virtual void fromRadioLayer(MAC_GenericFrame*) = 0;
	int bufferFrame(MAC_GenericFrame*);
	
	void setTimer(int index, simtime_t time);
	void cancelTimer(int index);
	virtual void timerFiredCallback(int index);
	
	void carrierSense(simtime_t time = 0.0);
	virtual void carrierSenseCallback(int returnCode);
	
	virtual void processOverheardFrame(MAC_GenericFrame*); // ???
	virtual void radioBufferFullCallback();
	void setRadioState(MAC_ControlMessageType typeID, simtime_t delay=0.0); 
	void setRadioTxMode(Radio_TxMode txTypeID, simtime_t delay=0.0);
	void setRadioPowerLevel(int powLevel, simtime_t delay=0.0);
	
	void encapsulateNetworkFrame(Network_GenericFrame *networkFrame, MAC_GenericFrame *retFrame);
	int resolveNextHop(const char *nextHop);
};

#endif 
