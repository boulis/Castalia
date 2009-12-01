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
#include "DebugInfoWriter.h"
using namespace std;

enum MacStates {
    MAC_STATE_SETUP = 100,
    MAC_STATE_SLEEP = 101,
    MAC_STATE_ACTIVE = 102,
    MAC_STATE_ACTIVE_SILENT = 103,
    MAC_STATE_IN_TX = 104,

    MAC_CARRIER_SENSE_FOR_TX_RTS = 110,    //These states are used to distinguish whether we are trying to TX
    MAC_CARRIER_SENSE_FOR_TX_DATA = 111,   // Data OR Cts OR Rts OR Ack
    MAC_CARRIER_SENSE_FOR_TX_CTS = 112,
    MAC_CARRIER_SENSE_FOR_TX_ACK = 113,
    MAC_CARRIER_SENSE_FOR_TX_SYNC = 114,
    MAC_CARRIER_SENSE_BEFORE_SLEEP = 115,

    MAC_STATE_WAIT_FOR_DATA = 120,
    MAC_STATE_WAIT_FOR_CTS = 121,
    MAC_STATE_WAIT_FOR_ACK = 122,
};

struct TMacSchedule {
    simtime_t offset;
    int ID;
    int SN;
};

class TMacModule : public cSimpleModule
{
	private:
	// parameters and variables

	/*--- A map from int value of state to its description (used in debug) ---*/
	map <int, string> stateDescr;
	map <string, int> packetsSent;

	/*--- The .ned file's parameters ---*/
	bool printDebugInfo;
	bool printDroppedPackets;
	bool printStateTransitions;

	int maxMACFrameSize;		//in bytes
	int maxTxRetries;
	int macFrameOverhead;		//in bytes
	int macBufferSize;		//in # of messages

	int ackFrameSize;		//in bytes
	int syncFrameSize;		//in bytes
	int rtsFrameSize;		//in bytes
	int ctsFrameSize;		//in bytes

	bool allowSinkSync;
	int resyncTime;
	simtime_t contentionPeriod;
	simtime_t listenTimeout;
	simtime_t waitTimeout;
	bool useRtsCts;
	bool useFRTS;
	simtime_t frameTime;
	bool disableTAextension;
	bool conservativeTA;
	int collisionResolution;

	/*--- General MAC variable ---*/
	bool isSink;
	int phyLayerOverhead;
	int self;				// the node's ID
	int disabled;
	RadioModule *radioModule;		//a pointer to the object of the Radio Module (used for direct method calls)
	ResourceGenericManager *resMgrModule;	//a pointer to the object of the Radio Module (used for direct method calls)
	simtime_t radioDelayForValidCS;      	// delay for valid CS
	simtime_t radioDelayForSleep2Listen;	// delay to switch from sleep state to listen state
	double radioDataRate;
	double epsilon;				//used to keep/manage the order between two messages that are sent at the same simulation time
	double cpuClockDrift;
	double totalSimTime;

	/*--- TMAC state variables  ---*/
	int macState;
	int txAddr;			//current communication peer (can be BROADCAST)
	int txRetries;			//number of transmission attempts to txAddr (when reaches 0 - packet is dropped)
	bool primaryWakeup;		//used to distinguish between primary and secondary schedules
	bool needResync;		//set to 1 when a SYNC frame has to be sent
	simtime_t currentFrameStart;	//recorded start time of the current frame

	/*--- TMAC message pointers (to cancel it and reschedule if necessary) ---*/
	MAC_ControlMessage *carrierSenseMsg;
	MAC_ControlMessage *nextFrameMsg;
	MAC_ControlMessage *macTimeoutMsg;

	/*--- TMAC activation timeout variable ---*/
	simtime_t activationTimeout;	//time untill MAC_CHECK_TA message arrival

	/*--- TMAC frame pointers (sometimes frame is created not immediately before sending) ---*/
	MAC_GenericFrame *syncFrame;
	MAC_GenericFrame *rtsFrame;
	MAC_GenericFrame *ctsFrame;
	MAC_GenericFrame *ackFrame;

	/*--- TMAC transmission times ---*/
	simtime_t syncTxTime;
	simtime_t rtsTxTime;
	simtime_t ctsTxTime;
	simtime_t ackTxTime;

	/*--- TMAC transmission buffer ---*/
	queue <Network_GenericFrame *> TXBuffer;

	/*--- TMAC Schedule table (list of effective schedules) ---*/
	vector <TMacSchedule> scheduleTable;

	protected:
	virtual void initialize();
	virtual void handleMessage(cMessage *msg);
	virtual void finish();
	void readIniFileParameters(void);
	void setRadioState(MAC_ControlMessageType typeID, double delay=0);
	void setRadioTxMode(Radio_TxMode txTypeID, double delay=0);
	void setRadioPowerLevel(int powLevel, double delay=0);
	
	void resetDefaultState();
	void setMacState(int newState);
	void createPrimarySchedule();
	void scheduleSyncFrame(simtime_t when);
	void processMacFrame(MAC_GenericFrame *rcvFrame);
	void carrierIsClear();
	void updateScheduleTable(simtime_t wakeup, int ID, int SN);
	void performCarrierSense(int newState, simtime_t delay = 0);
	void extendActivePeriod(simtime_t extra = 0);
	void checkTxBuffer();
	void popTxBuffer();
	void updateTimeout(simtime_t t);
	void clearTimeout();

};

#endif //TMACMODULE
