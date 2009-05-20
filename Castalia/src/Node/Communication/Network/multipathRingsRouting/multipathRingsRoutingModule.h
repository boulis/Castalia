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


#ifndef MULTIPATHRINGSROUTINGMODULE
#define MULTIPATHRINGSROUTINGMODULE

#include <vector>
#include <omnetpp.h>
#include "App_GenericDataPacket_m.h"
#include "App_ControlMessage_m.h"
#include "multipathRingsRoutingControlMessage_m.h"
#include "multipathRingsRoutingFrame_m.h"
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


class multipathRingsRoutingModule : public cSimpleModule 
{
	private: 
	// parameters and variables
		
		/*--- The .ned file's parameters ---*/
		int maxNetFrameSize;	//in bytes
		int netDataFrameOverhead;	//in bytes
		int netBufferSize;	//in # of messages
		int macFrameOverhead;
		bool printDebugInfo;
		
		int mpathRingsSetupFrameOverhead;// in bytes
		double netSetupTimeout;
		
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
		
		// multipathRingsRouting-related member variables
		bool isSink; //is a .ned file parameter of the Application module 
		int currentSinkID;
		int currentLevel;
		int tmpSinkID;
		int tmpLevel;
		bool isConnected; //attached under a parent node
		bool isScheduledNetSetupTimeout;

		
	protected:
		virtual void initialize();
		virtual void handleMessage(cMessage *msg);
		virtual void finish();
		void readIniFileParameters(void);
		int encapsulateAppPacket(App_GenericDataPacket *appPacket, multipathRingsRouting_DataFrame *retFrame);
		int pushBuffer(Network_GenericFrame *theMsg);
		Network_GenericFrame* popTxBuffer(void);
		int getTXBufferSize(void);
		void multipathRingsRoute_forwardPacket(multipathRingsRouting_DataFrame *theMsg);
		void filterIncomingNetworkDataFrames(Network_GenericFrame *theFrame);
		void decapsulateAndDeliverToApplication(multipathRingsRouting_DataFrame * parFrame2Deliver);
		void sendTopoSetupMesage(int parSinkID, int parSenderLevel);
};

#endif //MULTIPATHRINGSROUTINGMODULE

