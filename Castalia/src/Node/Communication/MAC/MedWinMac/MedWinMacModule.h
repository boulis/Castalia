/********************************************************************************
 *  Copyright: National ICT Australia,  2010									*
 *  Developed at the ATP lab, Networked Systems theme							*
 *  Author(s): Athanassios Boulis, Yuri Tselishchev								*
 *  This file is distributed under the terms in the attached LICENSE file.		*
 *  If you do not find this file, copies can be found by writing to:			*
 *																				*
 *      NICTA, Locked Bag 9013, Alexandria, NSW 1435, Australia					*
 *      Attention:  License Inquiry.											*
 ********************************************************************************/

#ifndef MEDWIN_MAC_MODULE_H
#define MEDWIN_MAC_MODULE_H

#include "VirtualMacModule.h"
#include "MedWinMacPacket_m.h"

#define TX_TIME(x) (phyLayerOverhead + x)*1/(1000*phyDataRate/8.0) //x are in BYTES
#define UNCONNECTED -1
#define GUARD_TIME pastSyncIntervalNominal ? guardTime() : allocationSlotLength/10.0;

using namespace std;

enum MacStates {
	MAC_STATE_SETUP = 1000,
	MAC_RAP = 1001,
	MAC_SCHEDULED_TX_ACCESS = 1002,
	MAC_SCHEDULED_RX_ACCESS = 1003,
	MAC_SLEEP = 1010
};

enum Timers {
	CARRIER_SENSING = 1,
	SEND_PACKET = 2,
	ACK_TIMEOUT = 3,
	START_SLEEPING = 4,
	START_SCHEDULED_TX_ACCESS = 5,
	WAKEUP_FOR_BEACON = 6,
	SYNC_INTERVAL_TIMEOUT = 7,
	SEND_BEACON = 8,
	HUB_ATTEMPT_TX_IN_RAP = 9,
	HUB_SCHEDULED_ACCESS = 10
};

static int CWmin[8] = { 16, 16, 8, 8, 4, 4, 2, 1 };
static int CWmax[8] = { 64, 32, 32, 16, 16, 8, 8, 4};

class MedWinMacModule : public VirtualMacModule {
	private:
	int maxPacketRetries;
	int currentPacketRetries;

	int currentScheduleAssignmentStart;
	int currentFreeConnectedNID;

	double allocationSlotLength;
	int RAP1Length;
	int beaconPeriodLength;
	bool CWdouble;
	bool pastSyncIntervalNominal;

	int scheduledAccessStart;
	int scheduledAccessEnd;
	int scheduledAccessLength;

	simtime_t endTime;
	simtime_t RAPEndTime;

	bool isHub;

	protected:
	void startup();
	void timerFiredCallback(int);
	void fromNetworkLayer(cPacket*, int);
	void fromRadioLayer(cPacket*,double,double);
	bool isPacketForMe(MedWinPacket * pkt);
	void setHeaderFields(MedWinPacket * pkt, AcknowledgementPolicy_type ackPolicy, Frame_type frameType, Frame_subtype frameSubtype);
	void attempTxInRAP();
	bool needToTx();
	simtime_t timeToNextBeacon(simtime_t interval, int index, int phase);
}

#endif // MEDWIN_MAC_MODULE_H
