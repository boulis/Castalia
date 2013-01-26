/****************************************************************************
 *  Copyright: National ICT Australia,  2007 - 2012                         *
 *  Developed at the ATP lab, Networked Systems research theme              *
 *  Author(s): Athanassios Boulis, Yuriy Tselishchev                        *
 *  This file is distributed under the terms in the attached LICENSE file.  *
 *  If you do not find this file, copies can be found by writing to:        *
 *                                                                          *
 *      NICTA, Locked Bag 9013, Alexandria, NSW 1435, Australia             *
 *      Attention:  License Inquiry.                                        *
 *                                                                          *  
 ****************************************************************************/

#ifndef BASIC_802154_H_
#define BASIC_802154_H_

#include <map>
#include <vector>

#include "VirtualMac.h"
#include "Basic802154Packet_m.h"

#define ACK_PKT_SIZE 6
#define COMMAND_PKT_SIZE 10
#define GTS_SPEC_FIELD_SIZE 3
#define BASE_BEACON_PKT_SIZE 12

#define TX_TIME(x) (phyLayerOverhead + x)*1/(1000*phyDataRate/8.0)	//x are in BYTES

using namespace std;

enum MacStates {
	MAC_STATE_SETUP = 1000,
	MAC_STATE_SLEEP = 1001,
	MAC_STATE_CAP = 1002,
	MAC_STATE_GTS = 1003,
};

enum Mac802154Timers {
	PERFORM_CCA = 1,
	ATTEMPT_TX = 2,
	BEACON_TIMEOUT = 3,
	ACK_TIMEOUT = 4,
	GTS_START = 5,
	SLEEP_START = 6,
	FRAME_START = 7,
	BACK_TO_SETUP = 8,
};

class Basic802154: public VirtualMac {
 private:
    /*--- A map from int value of state to its description (used in debug) ---*/
	map<int,string> stateDescr;

    /*--- A map for packet breakdown statistics ---*/
	map<string,int> packetBreak;

    /*--- General MAC variable ---*/
	simtime_t phyDelayForValidCS;		// delay for valid CS
	simtime_t phyDelaySleep2Tx;
	simtime_t phyDelayRx2Tx;
	int phyLayerOverhead;
	int phyBitsPerSymbol;
	double phyDataRate;

	map<int,bool> associatedDevices;	// map of assoicated devices (for PAN coordinator)

    /*--- 802154Mac state variables  ---*/
	int macState;		// current MAC state
	int CAPlength;		// duration of CAP interval (in number of superframe slots)
	int macBSN;			// beacon sequence number (unused)
	int lostBeacons;	// number of consequitive lost beacon packets
	int frameInterval;	// duration of active part of the frame (in symbols)
	int beaconInterval;	// duration of the whole frame (in symbols)
	int seqNum;			// sequence number for data packets

	int NB, CW, BE;		// CSMA-CA algorithm parameters (Number of Backoffs, 
						// Contention Window length and Backoff Exponent)

	double currentPacketLimit;	// maximum delay limit for current packet
	int currentPacketRetries;	// number of retries left for next packet to be sent
	bool currentPacketGtsOnly;	// transmission of current packet can be done in GTS only
	bool currentPacketSuccess; 	// outcome of transmission for current packet
	string currentPacketHistory;// history of transmission attempts for current packet

	simtime_t currentPacketResponse;// Duration of timeout for receiving a reply after sending a packet
	simtime_t ackWaitDuration;		// Duration of timeout for receiving an ACK
	simtime_t symbolLen;			// Duration of transmittion of a single symbol
	simtime_t guardTime;

	// packet statistics
	int sentBeacons;
	int recvBeacons;
	int packetoverflow;
	
	// syncronisation statistics
	simtime_t desyncTime;
	simtime_t desyncTimeStart;

    /*--- 802154Mac packet pointers (sometimes packet is created not immediately before sending) ---*/
	Basic802154Packet *beaconPacket;
	Basic802154Packet *currentPacket;

	void fromNetworkLayer(cPacket *, int);
	void fromRadioLayer(cPacket *, double, double);

	void readIniFileParameters(void);
	void setMacState(int newState);
	void handleAckPacket(Basic802154Packet *);
	void performCSMACA();
	void attemptTransmission(const char *);
	void transmitCurrentPacket();
	void collectPacketHistory(const char * s);
	void clearCurrentPacket(const char * s = NULL, bool success = false);

     /*--- The .ned file's parameters ---*/
	bool printStateTransitions;
	bool isPANCoordinator;
	bool isFFD;
	bool batteryLifeExtention;
	bool enableSlottedCSMA;

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

	simtime_t CAPend;				// Absolute time of end of CAP period for current frame
	simtime_t currentFrameStart;	// Absolute recorded start time of the current frame
	simtime_t GTSstart;				// Absolute time of the start of GTS period for current node
	simtime_t GTSend;				// Absolute time of the end of GTS period for current node
	simtime_t GTSlength;			// length of the GTS period for current node

	int associatedPAN;	// ID of current PAN (-1 if not associated)

  protected:

	/*--- Interface functions of the 802.15.4 MAC ---*/
	Basic802154Packet *newConnectionRequest(int);
	Basic802154Packet *newGtsRequest(int,int);
	Basic802154Packet *getCurrentPacket() { return currentPacket; }
	void transmitPacket(Basic802154Packet *pkt, int retries = 0, bool GTSonly = false, double delay = 0);
	int getCurrentMacState() { return macState; }
	int getAssociatedPAN() { return associatedPAN; }

	/*--- Virtual interface functions can be overwritten by a decision module ---*/
	virtual void startup();
	virtual void timerFiredCallback(int);
	virtual void finishSpecific();
	
	/*--- HUB-specific desicions ---*/
	virtual void dataReceived_hub(Basic802154Packet *) {}
	virtual int gtsRequest_hub(Basic802154Packet *) { return 0; }			//default: always reject
	virtual int associationRequest_hub(Basic802154Packet *) { return 1; }	//default: always accept
	virtual void prepareBeacon_hub(Basic802154Packet *) {}

	/*--- Node-specific desicions ---*/
	virtual void connectedToPAN_node() {}
	virtual void disconnectedFromPAN_node() {}
	virtual void assignedGTS_node(int) {}
	virtual void receiveBeacon_node(Basic802154Packet *);
	virtual void startedGTS_node() {}

	/*--- General desicions ---*/
	virtual bool acceptNewPacket(Basic802154Packet *);
	virtual void transmissionOutcome(Basic802154Packet *, bool, string);
};

#endif	//BASIC_802154
