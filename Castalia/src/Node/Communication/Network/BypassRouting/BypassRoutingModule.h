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



#ifndef BYPASSROUTINGMODULE
#define BYPASSROUTINGMODULE

#include <vector>
#include <omnetpp.h>
#include "App_GenericDataPacket_m.h"
#include "App_ControlMessage_m.h"
#include "NetworkControlMessage_m.h"
#include "NetworkGenericFrame_m.h"
#include "MacGenericFrame_m.h"
#include "MacControlMessage_m.h"
#include "ResourceGenericManager.h"
#include "RadioModule.h"
#include "DebugInfoWriter.h"
using namespace std;

enum RoutingDecisions
{
	DELIVER_MSG_2_APP = 3200,
	FORWARD_MSG_2_MAC = 3201,
};

class BypassRoutingModule : public cSimpleModule 
{
	private: 
	// parameters and variables
		
		/*--- The .ned file's parameters ---*/
		int maxNetFrameSize;	//in bytes
		int netDataFrameOverhead;	//in bytes
		int netBufferSize;	//in # of messages
		int macFrameOverhead;
		bool printDebugInfo;
		
		/*--- Custom class parameters ---*/
		int self;					// the node's ID
		RadioModule *radioModule;	//a pointer to the object of the Radio Module (used for direct method calls)
		double radioDataRate;
		ResourceGenericManager *resMgrModule;	//a pointer to the object of the Radio Module (used for direct method calls)
		
		Network_GenericFrame **schedTXBuffer;		// a buffer that can hold up to 9 values to get trasmitted
		int headTxBuffer;
		int tailTxBuffer;
		
		double epsilon;				//used to keep/manage the order between two messages that are sent at the same simulation time
		
		int maxSchedTXBufferSizeRecorded;
		
		double cpuClockDrift;
		int disabled;
		string strSelfID;

	protected:
		virtual void initialize();
		virtual void handleMessage(cMessage *msg);
		virtual void finish();
		void readIniFileParameters(void);
		int encapsulateAppPacket(App_GenericDataPacket *appPacket, Network_GenericFrame *retFrame);
		int pushBuffer(Network_GenericFrame *theMsg);
		Network_GenericFrame* popTxBuffer(void);
		int getTXBufferSize(void);
};

#endif //BYPASSROUTINGMODULE

