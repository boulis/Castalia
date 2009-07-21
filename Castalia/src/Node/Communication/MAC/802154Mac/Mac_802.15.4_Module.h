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
    MAC_STATE_WAIT_FOR_BEACON = 1008

    
};

class Mac802154Module : public cSimpleModule 
{
	private: 
	// parameters and variables
		
	/*--- A map from int value of state to its description (used in debug) ---*/
	map <int, string> stateDescr;
	
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
	
	int macMinBE;
	int macMaxBE;
	int macMaxCSMABackoffs;
	int macMaxFrameRetries;
	int maxLostBeacons;
	int unitBackoffPeriod;
	int baseSlotDuration;
	int numSuperframeSlots;
	int baseSuperframeDuration;
	int beaconOrder;
	int frameOrder;

	/*--- General MAC variable ---*/
	bool isSink;
	int phyLayerOverhead;
	int self;				// the node's ID
	int disabled;
	RadioModule *radioModule;		// a pointer to the object of the Radio Module (used for direct method calls)
	ResourceGenericManager *resMgrModule;	// a pointer to the object of the Resource manager Module (used for direct method calls)
	double radioDelayForValidCS;      	// delay for valid CS
	double radioDelayForSleep2Listen;	// delay to switch from sleep state to listen state
	double radioDelayForListen2Tx;
	double radioDataRate;
	double epsilon;				//used to keep/manage the order between two messages that are sent at the same simulation time
	double cpuClockDrift;
	double totalSimTime;
	
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

	double CAPend;			// Absolute time of end of CAP period for current frame
	double currentFrameStart;	// Absolute recorded start time of the current frame
	double lastCarrierSense;	// Absolute recorded time of last carrier sense message from the radio
	double nextPacketResponse;	// Duration of timeout for receiving a reply after sending a packet
	double ackWaitDuration;		// Duration of timeout for receiving an ACK
	double symbolLen;		// Duration of transmittion of a single symbol

	map <int, bool> associatedDevices;	// map of assoicated devices (for PAN coordinator)

	/*--- 802154Mac message pointers (to cancel it and reschedule if necessary) ---*/
	MAC_ControlMessage *beaconTimeoutMsg;
	
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
	void performCarrierSense();
	void processMacFrame(MAC_GenericFrame *);
	void handleAckFrame(MAC_GenericFrame *);
	void carrierIsClear();
	void initiateCSMACA(int, int, double);
	void initiateCSMACA();
	void continueCSMACA();
	void attemptTransmission(double delay = 0);
};

#endif //MAC_802154_MODULE
