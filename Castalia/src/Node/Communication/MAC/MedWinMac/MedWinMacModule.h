/********************************************************************************
 *  Copyright: National ICT Australia,  2010					*
 *  Developed at the ATP lab, Networked Systems theme				*
 *  Author(s): Athanassios Boulis, Yuri Tselishchev				*
 *  This file is distributed under the terms in the attached LICENSE file.	*
 *  If you do not find this file, copies can be found by writing to:		*
 *										*
 *      NICTA, Locked Bag 9013, Alexandria, NSW 1435, Australia			*
 *      Attention:  License Inquiry.						*
 ********************************************************************************/

#ifndef MEDWIN_MAC_MODULE
#define MEDWIN_MAC_MODULE

#include "VirtualMacModule.h"
#include "MedWinMacPacket_m.h"

#define TX_TIME(x) (phyLayerOverhead + x)*1/(1000*phyDataRate/8.0) //x are in BYTES
#define UNCONNECTED -1

using namespace std;

enum MacStates {
    MAC_STATE_SETUP = 1000,
};

enum Timers {
};

static int CWmin[8] = { 16, 16, 8, 8, 4, 4, 2, 1 };
static int CWmax[8] = { 64, 32, 32, 16, 16, 8, 8, 4};

class MedWinMacModule : public VirtualMacModule {
	private:
	double allocationSlotLength;

    protected:
	void startup();
	void timerFiredCallback(int);
	void fromNetworkLayer(cPacket*, int);
	void fromRadioLayer(cPacket*,double,double);
	bool MedWinMacModule::isPacketForMe(MedWinPacket * pkt);
	void MedWinMacModule::setHeaderFields(MedWinPacket * pkt, AcknowledgementPolicy_type ackPolicy, Frame_type frameType, Frame_subtype frameSubtype);
	void MedWinMacModule::attempTxInRAP();
	simtime_t MedWinMacModule::timeToNextBeacon(simtime_t interval, int index, int phase);

}