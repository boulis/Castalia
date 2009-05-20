/***************************************************************************
 *  Copyright: National ICT Australia,  2009
 *  Developed at the ATP lab, Networked Systems theme
 *  Author(s): Athanassios Boulis, Yuri Tselishchev
 *  This file is distributed under the terms in the attached LICENSE file.
 *  If you do not find this file, copies can be found by writing to:
 *
 *      NICTA, Locked Bag 9013, Alexandria, NSW 1435, Australia
 *      Attention:  License Inquiry.
 ***************************************************************************/
 
#ifndef TMACMODULE
#define TMACMODULE

#include <vector>
#include <omnetpp.h>
#include <cqueue.h>

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

enum MacStates {
    MAC_STATE_SETUP = 100,
    MAC_STATE_ACTIVE = 101,
    MAC_STATE_PASSIVE = 102,
    
    MAC_CARRIER_SENSE_FOR_TX_RTS = 111,    //These states are used to distinguish whether we are trying to TX
    MAC_CARRIER_SENSE_FOR_TX_DATA = 119,   // Data OR Cts OR Rts OR Ack  
    MAC_CARRIER_SENSE_FOR_TX_CTS = 125,
    MAC_CARRIER_SENSE_FOR_TX_ACK = 126,
    MAC_CARRIER_SENSE_FOR_TX_SYNC = 128,
    MAC_STATE_WAIT_FOR_DATA  = 113,        
    MAC_STATE_WAIT_FOR_CTS   = 120,        
    MAC_STATE_WAIT_FOR_ACK	 = 121,
    MAC_STATE_SLEEP = 130,			
    MAC_STATE_SILENT = 111
};

struct TMacSchedule {
    double offset;
    int ID;
    int SN;
};

class TMacModule : public cSimpleModule 
{
	private: 
	// parameters and variables
		
		/*--- The .ned file's parameters ---*/
				
		int maxMACFrameSize;		//in bytes
		int macFrameOverhead;		//in bytes
		
		int syncFrameSize;		//in bytes
		int rtsFrameSize;		//in bytes
		int ctsFrameSize;		//in bytes
		int ackFrameSize;		//in bytes
		int macBufferSize;		//in # of messages
		
		int resyncTime;
		bool waitForSync;
		bool allowSinkSync;
		bool secondaryWakeup;
		
		int phyLayerOverhead;
		bool printDebugInfo;
		bool printStateTransitions;
		//bool printDropped;
		bool printPotentiallyDropped;
		bool isSink;
		
		/*--- Custom class parameters ---*/
		
		int self;					// the node's ID
		RadioModule *radioModule;	//a pointer to the object of the Radio Module (used for direct method calls)
		ResourceGenericManager *resMgrModule;	//a pointer to the object of the Radio Module (used for direct method calls)
		double radioDelayForValidCS;      // delay for valid CS
		double radioDelayForSleep2Listen;// delay to switch from sleep state to listen state
		double radioDataRate;
		int macState;
				// keep a pointer to a message so we can cancel it and reschedule it
				
		MAC_ControlMessage *startupSchedMsg; // node startup message reference
		MAC_ControlMessage *waitForCtsTimeout;// exit wait for CTS state message reference
		MAC_ControlMessage *waitForDataTimeout;// exit wait for DATA state message reference
		MAC_ControlMessage *waitForAckTimeout;// exit wait for ACK state message reference
		MAC_ControlMessage *carrierSenseMsg;
		MAC_ControlMessage *nextFrameMsg;
		MAC_ControlMessage *selfCheckTXBufferMsg;
		MAC_GenericFrame   *Frame;	//keep a pointer to current frame will be sent		
		MAC_GenericFrame  *syncFrame;
		MAC_GenericFrame  *rtsFrame;
		MAC_GenericFrame  *ctsFrame;
		MAC_GenericFrame  *ackFrame;

		MAC_GenericFrame **schedTXBuffer;		// a buffer that can hold up to 9 values to get trasmitted
		int headTxBuffer;
		int tailTxBuffer;
		
		int maxSchedTXBufferSizeRecorded;
		double sleepInterval;		// in secs
		double epsilon;				//used to keep/manage the order between two messages that are sent at the same simulation time
		double cpuClockDrift;
		int disabled;
				
		bool primaryWakeup;
		bool useRtsCts;
		vector <TMacSchedule> scheduleTable;
		
		//Duty Cycle scheduled message references
		MAC_ControlMessage *currentSleepMsg;	
		MAC_ControlMessage *primaryWakeupMsg;
		
		
		MAC_ControlMessage *navTimeoutMsg;	//NAV (virtual Carrier Sense) scheduled message reference
		int potentiallyDroppedDataFrames;

	protected:
		virtual void initialize();
		virtual void handleMessage(cMessage *msg);
		virtual void finish();
		void readIniFileParameters(void);
		void setRadioState(MAC_ControlMessageType typeID, double delay=0);
		void setRadioTxMode(Radio_TxMode txTypeID, double delay=0);
		void setRadioPowerLevel(int powLevel, double delay=0);
		void createACKFrame(MAC_GenericFrame *retFrame);
		int encapsulateNetworkFrame(Network_GenericFrame *networkFrame, MAC_GenericFrame *retFrame);
		int pushBuffer(MAC_GenericFrame *theMsg);
		MAC_GenericFrame* popTxBuffer();
		MAC_GenericFrame* peekTxBuffer();
		int getTXBufferSize(void);
		int deliver2NextHop(const char *nextHop);
		void resetDefaultState();

	//T-MAC specific 
	bool useFRTS;		
	int scheduleState;		
	double frameTime;
	double listenTimeout;
	int dataFrameSize;	
	double totalSimTime;
	
	int currentAdr;    //contains the address where RTS is to be send
	int syncID;
	int syncSN;
	int rtsSN;
	int ctsSN;
	int ackSN;
	int dataSN;
	int syncReceived;

	double syncTxTime;
	double rtsTxTime;
	double ctsTxTime;
	double ackTxTime;
	double updateSYNC;				//received sync value from SYNC msg
	double currentFrameStart;	
	
	bool needResync;
	double contentionPeriod;

	double randomContendOffset;
	int ctsCarrierSenseRetries;
	int dataCarrierSenseRetries;

	void changeState(int newState);	
	void createPrimarySchedule();
	void scheduleSyncFrame(double when);
	void processMacFrame(MAC_GenericFrame *rcvFrame);
	void carrierIsClear();
	double nextWakeup();
	void updateScheduleTable(double wakeup, int ID, int SN);
	void updateNav(double t);	

};

#endif //TMACMODULE
