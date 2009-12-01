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
 
#ifndef MAC_802154_MODULE
#define MAC_802154_MODULE

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
    MAC_STATE_SETUP = 1000,
    MAC_STATE_SLEEP = 1001,
    MAC_STATE_IDLE = 1002,
    MAC_STATE_CSMA_CA = 1003,
    MAC_STATE_CCA = 1004,
    MAC_STATE_IN_TX = 1005,
    MAC_STATE_WAIT_FOR_ASSOCIATE_RESPONSE = 1006,
    MAC_STATE_WAIT_FOR_DATA_ACK = 1007,
    MAC_STATE_WAIT_FOR_BEACON = 1008,
    MAC_STATE_WAIT_FOR_GTS = 1009,
    MAC_STATE_IN_GTS = 1010,
    MAC_STATE_PROCESSING = 1011
    
};

class Mac802154Module : public cSimpleModule 
{
	private: 
	// parameters and variables
		
	/*--- A map from int value of state to its description (used in debug) ---*/
	map <int, string> stateDescr;
	
	int nextPacketTry;
	double nextPacketTime;
	int sentBeacons;
	int recvBeacons;
	

	/*--- A map for packet breakdown statistics ---*/
	map <string, int> packetBreak;
	
	/*--- The .ned file's parameters ---*/
	bool printDebugInfo;
	bool printStateTransitions;


	int maxMACFrameSize;		//in bytes
	int macFrameOverhead;		//in bytes
	int macBufferSize;		//in # of messages
		
	bool isPANCoordinator;
	bool isFFD;
	bool batteryLifeExtention;
	bool enableSlottedCSMA;
	bool enableCAP;
	bool lockedGTS;
	
	int macMinBE;
	int macMaxBE;
	int macMaxCSMABackoffs;
	int macMaxFrameRetries;
	int maxLostBeacons;
	int minCAPLength;
	int unitBackoffPeriod;
	int baseSlotDuration;
	int numSuperframeSlots;
	int baseSuperframeDuration;
	int beaconOrder;
	int frameOrder;
	int requestGTS;

	/*--- General MAC variable ---*/
	bool isSink;
	int phyLayerOverhead;
	int self;				// the node's ID
	int disabled;
	RadioModule *radioModule;		// a pointer to the object of the Radio Module (used for direct method calls)
	ResourceGenericManager *resMgrModule;	// a pointer to the object of the Resource manager Module (used for direct method calls)
	simtime_t radioDelayForValidCS;      	// delay for valid CS
	simtime_t radioDelayForSleep2Listen;	// delay to switch from sleep state to listen state
	simtime_t radioDelayForListen2Tx;
	double radioDataRate;
	double epsilon;				//used to keep/manage the order between two messages that are sent at the same simulation time
	double cpuClockDrift;
	simtime_t totalSimTime;
	
	/*--- 802154Mac state variables  ---*/
	int associatedPAN;		// ID of current PAN (-1 if not associated)
	int macState;			// current MAC state
	int nextMacState;		// next MAC state (will be switched after CSMA-CA algorithm ends)
	int CAPlength;			// duration of CAP interval (in number of superframe slots)
	int macBSN;			// beacon sequence number (unused)
	int nextPacketRetries;		// number of retries left for next packet to be sent
	int lostBeacons;		// number of consequitive lost beacon packets
	int frameInterval;		// duration of active part of the frame (in symbols)
	int beaconInterval;		// duration of the whole frame (in symbols)

	int NB, CW, BE;			// CSMA-CA algorithm parameters (Number of Backoffs, 
					// Contention Window length and Backoff Exponent)

	simtime_t CAPend;			// Absolute time of end of CAP period for current frame
	simtime_t currentFrameStart;	// Absolute recorded start time of the current frame
	simtime_t GTSstart;
	simtime_t GTSend;
	
	simtime_t lastCarrierSense;	// Absolute recorded time of last carrier sense message from the radio
	simtime_t nextPacketResponse;	// Duration of timeout for receiving a reply after sending a packet
	simtime_t ackWaitDuration;		// Duration of timeout for receiving an ACK
	simtime_t symbolLen;		// Duration of transmittion of a single symbol

	string nextPacketState;
	simtime_t desyncTime;
	simtime_t desyncTimeStart;

	map <int, bool> associatedDevices;	// map of assoicated devices (for PAN coordinator)

	/*--- 802154Mac message pointers (to cancel it and reschedule if necessary) ---*/
	MAC_ControlMessage *beaconTimeoutMsg;
	MAC_ControlMessage *txResetMsg;
	
	/*--- 802154Mac packet pointers (sometimes packet is created not immediately before sending) ---*/
	MAC_GenericFrame *beaconPacket;
	MAC_GenericFrame *associateRequestPacket;
	MAC_GenericFrame *nextPacket;

	/*--- 802154Mac transmission buffer ---*/
	queue <Network_GenericFrame *> TXBuffer;
	
	/*--- 802154Mac GTS list --- */
	vector <GTSspec> GTSlist;		// list of GTS specifications (for PAN coordinator, currently unused)
		
	protected:
	virtual void initialize();
	virtual void handleMessage(cMessage *msg);
	virtual void finish();
	void readIniFileParameters(void);
	void setRadioState(MAC_ControlMessageType typeID, double delay=0);
	void setRadioTxMode(Radio_TxMode txTypeID, double delay=0);
	void setRadioPowerLevel(int powLevel, double delay=0);
	void setMacState(int newState);	
	int performCarrierSense();
	void processMacFrame(MAC_GenericFrame *);
	void handleAckFrame(MAC_GenericFrame *);
	void carrierIsClear();
	void initiateCSMACA(int, int, simtime_t);
	void initiateCSMACA();
	void continueCSMACA();
	void attemptTransmission(simtime_t delay = 0);
	void transmitNextPacket();
	void issueGTSrequest();
	
};

#endif //MAC_802154_MODULE
