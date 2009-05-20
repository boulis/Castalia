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



#ifndef TUNABLEMACMODULE
#define TUNABLEMACMODULE

#include <vector>
#include <omnetpp.h>
//#include "App_GenericDataPacket_m.h"
#include "App_ControlMessage_m.h"
#include "NetworkGenericFrame_m.h"
#include "NetworkControlMessage_m.h"
#include "MacGenericFrame_m.h"
#include "MacControlMessage_m.h"
#include "RadioControlMessage_m.h"
#include "ResourceGenericManager.h"
#include "RadioModule.h"
#include "DebugInfoWriter.h"
using namespace std;


class BypassMacModule : public cSimpleModule 
{
	private: 
	// parameters and variables
		
		/*--- The .ned file's parameters ---*/
		int maxMACFrameSize;	//in bytes
		int macFrameOverhead;	//in bytes
		int macBufferSize;	//in # of messages
		int phyLayerOverhead;
		bool printDebugInfo;
		
		/*--- Custom class parameters ---*/
		int self;					// the node's ID
		RadioModule *radioModule;	//a pointer to the object of the Radio Module (used for direct method calls)
		double radioDelayForValidCS;
		ResourceGenericManager *resMgrModule;	//a pointer to the object of the Radio Module (used for direct method calls)
		double radioDataRate;
		
		MAC_GenericFrame **schedTXBuffer;		// a buffer that can hold up to 9 values to get trasmitted
		int headTxBuffer;
		int tailTxBuffer;
		
		int maxSchedTXBufferSizeRecorded;
		double epsilon;				//used to keep/manage the order between two messages that are sent at the same simulation time
		double cpuClockDrift;
		int disabled;

	protected:
		virtual void initialize();
		virtual void handleMessage(cMessage *msg);
		virtual void finish();
		void readIniFileParameters(void);
		void setRadioState(MAC_ControlMessageType typeID, double delay=0.0);
		void setRadioTxMode(Radio_TxMode txTypeID, double delay=0.0);
		void setRadioPowerLevel(int powLevel, double delay=0.0);
		int encapsulateNetworkFrame(Network_GenericFrame *networkFrame, MAC_GenericFrame *retFrame);
		int pushBuffer(MAC_GenericFrame *theMsg);
		MAC_GenericFrame* popTxBuffer();
		int getTXBufferSize(void);
		int deliver2NextHop(const char *nextHop);
};

#endif //TUNABLEMACMODULE
