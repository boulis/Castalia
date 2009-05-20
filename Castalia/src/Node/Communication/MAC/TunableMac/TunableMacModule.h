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

enum backoffTypes
{
	BACKOFF_SLEEP_INT = 2200,
	BACKOFF_CONSTANT = 2201,
	BACKOFF_MULTIPLYING = 2202,
	BACKOFF_EXPONENTIAL = 2203
};

enum MacStates
{
	MAC_STATE_DEFAULT = 2204,
	MAC_STATE_TX = 2205,
	MAC_STATE_CARRIER_SENSING = 2206,
	MAC_STATE_EXPECTING_RX = 2207 // because of the duty cycle there is a mode when we are expecting a train of beacons and data
};


class TunableMacModule : public cSimpleModule 
{
	private: 
	// parameters and variables
		
		/*--- The .ned file's parameters ---*/
		double dutyCycle;			// sleeping interval / sleeping + listening intervals
		double listenInterval;		// in secs, note: parammeter in omnetpp.ini in msecs
		double beaconIntervalFraction;
		double probTx;				// probability of a single transmission to happen
		int numTx;					// when we have something to send, how many times do we try to try to transmit it. We say "try" because probTx might be < 1
		double randomTxOffset;		// when have somethingnto transmit, don't do it immediatelly
		double reTxInterval;		// the interval between retransmissions, in msec but after a time [0..randomTxOffset] chosen randomly (uniform)
		int maxMACFrameSize;		//in bytes
		int macFrameOverhead;		//in bytes
		int beaconFrameSize;		//in bytes
		int ACKFrameSize;			//in bytes
		int macBufferSize;			//in # of messages
		bool carrierSense;			//true or false for using CS before every checkTXBuffer
		int backoffType;			//can be 0 or 1 or 2 or 3
		double backoffBaseValue;		//the backoff value
		bool randomBackoff;
		int phyLayerOverhead;
		bool printDebugInfo;
		bool printStateTransitions;
		bool printPotentiallyDropped;
		
		/*--- Custom class parameters ---*/
		int self;					// the node's ID
		RadioModule *radioModule;	//a pointer to the object of the Radio Module (used for direct method calls)
		ResourceGenericManager *resMgrModule;	//a pointer to the object of the Radio Module (used for direct method calls)
		double radioDelayForValidCS;
		double radioDataRate;
		int macState;
		
		
		MAC_GenericFrame **schedTXBuffer;		// a buffer that can hold up to 9 values to get trasmitted
		int headTxBuffer;
		int tailTxBuffer;
		
		int maxSchedTXBufferSizeRecorded;
		double sleepInterval;		// in secs
		double epsilon;				//used to keep/manage the order between two messages that are sent at the same simulation time
		double cpuClockDrift;
		int disabled;
		int beaconSN;
		
		int backoffTimes;		// number of consequtive backoff times
		
		//Duty Cycle scheduled message references
		MAC_ControlMessage *dutyCycleSleepMsg;	
		MAC_ControlMessage *dutyCycleWakeupMsg;
		
		// Check TX buffer message reference
		MAC_ControlMessage *selfCheckTXBufferMsg;
		
		// Exit Carrier Sense message reference 
		MAC_ControlMessage *selfExitCSMsg;
		
		// Exit STATE_RX message reference
		MAC_ControlMessage *rxOutMessage;		// keep a pointer to a message so we can cancel it and reschedule it
		
		int potentiallyDroppedBeacons;
		int potentiallyDroppedDataFrames;

	protected:
		virtual void initialize();
		virtual void handleMessage(cMessage *msg);
		virtual void finish();
		void readIniFileParameters(void);
		void setRadioState(MAC_ControlMessageType typeID, double delay=0.0); 
		void setRadioTxMode(Radio_TxMode txTypeID, double delay=0.0);
		void setRadioPowerLevel(int powLevel, double delay=0.0);
		void createBeaconDataFrame(MAC_GenericFrame *retFrame);
		void createACKFrame(MAC_GenericFrame *retFrame);
		int isBeacon(MAC_GenericFrame *theFrame);
		int encapsulateNetworkFrame(Network_GenericFrame *networkFrame, MAC_GenericFrame *retFrame);
		int pushBuffer(MAC_GenericFrame *theMsg);
		MAC_GenericFrame* popTxBuffer();
		int getTXBufferSize(void);
		int deliver2NextHop(const char *nextHop);
};

#endif //TUNABLEMACMODULE
