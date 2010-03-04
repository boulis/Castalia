/********************************************************************************
 *  Copyright: National ICT Australia,  2010                                    *
 *  Developed at the ATP lab, Networked Systems theme                           *
 *  Author(s): Athanassios Boulis, Yuri Tselishchev                             *
 *  This file is distributed under the terms in the attached LICENSE file.      *
 *  If you do not find this file, copies can be found by writing to:            *
 *                                                                              *
 *      NICTA, Locked Bag 9013, Alexandria, NSW 1435, Australia                 *
 *      Attention:  License Inquiry.                                            *
 ********************************************************************************/

#include "MedWinMacModule.h"

Define_Module(MedWinMacModule);

void MedWinMacModule::startup() {
    connectedHID = UNCONNECTED;
    connectedNID = UNCONNECTED;
    unconnectedNID = 1 + genk_intrand(0,14);    //we select random unconnected NID
    scheduledAccessLength = par("scheduledAccessLength");
    scheduledAccessPeriod = par("scheduledAccessPeriod");
    phyLayerOverhead = par("phyLayerOverhead");
    phyDataRate = par("phyDataRate");
    priority = getParentModule()->getParentModule()->findSubModule("nodeApplication")->par("priority");

    contentionSlot = par("contentionSlot");
    maxPacketRetries = par("packetRetries");
	CW = CWmin[priority];
	CWdouble = false;
	pastSyncIntervalNominal = false;
}

void MedWinMacModule::timerFiredCallback(int index) {
    switch (index) {

        case CARRIER_SENSING: {
            if (!canFitTx()) {
                // Do not change it? macState = MAC_STATE_DEFAULT;
                break;
            }

            CCA_result CCAcode = radioModule->isChannelClear();
            if (CCAcode == CLEAR) {
                backoffCounter--;
                if (backoffCounter > 0) setTimer(CARRIER_SENSING,contentionSlot);
                else sendPacket();
            } else {
				// spec states that we wait until the channel is not busy
				// we cannot simply do that, we have to have periodic checks
				// we arbitrarily choose 10*contention slot = 3.6msec
                setTimer(CARRIER_SENSING,contentionSlot*10);
            }
            break;
        }

        case SEND_PACKET: {
            if (needToTx() && canFitTx()) sendPacket();
            break;
        }

        case ACK_TIMEOUT: {

			// double the Contention Window, after every second fail.
			CWdouble? CWdouble=false: CWdouble=true;
			if ((CWdouble) && (CW < CWmax[priority])) CW *=2;

            if (!needToTx()) break;
            if (macState == MAC_RAP) attemptTxInRAP();
            if ((macState == SCHEDULED_ACCESS) && (canFitTx())) sendPacket();
            break;
        }

		case START_SLEEPING: {
			macstate = MAC_SLEEP;
			toRadioLayer(createRadioCommand(SET_STATE,SLEEP));
			break;
		}

		case START_SCHEDULED_ACCESS: {
			macState = MAC_SCHEDULED_ACCESS;
			if (needToTx() && canFitTx()) sendPacket();
			break;
		}

        case WAKEUP_FOR_BEACON: {
			macstate = MAC_BEACON_WAIT;
			toRadioLayer(createRadioCommand(SET_STATE,RX));
			break;
		}

		case SYNC_INTERVAL_TIMEOUT: {
			pastSyncIntervalNominal = true;
			syncIntervalAdditionalStart = getClock();
			break;
		}
    }
}

void MedWinMacModule::fromNetworkLayer(cPacket pkt, int dst) {
}

void MedWinMacModule::fromRadioLayer(cPacket pkt, double rssi, double lqi) {
    // if the incoming packet is not MedWin, return (VirtualMAC will delete it)
    MedWinMacPacket * medWinPkt = dynamic_cast<MedWinMacPacket*>(pkt);
    if (medWinPkt == NULL) return;

    // filter the incoming MedWin packet
    if (!isPacketForMe(medWinPkt)) return;

	switch(medWinPkt->getFrameSubtype()) {
		case BEACON: {
			MedWinBeaconPacket * medWinBeacon = check_and_cast<MedWinBeaconPacket*>(medWinPkt);
			simtime_t beaconTxTime = TX_TIME(MedWinBeaconPacket->getByteLength());

			// get the allocation slot length, which is used in many calculations
			allocationSlotLength = medWinBeacon->getAllocationSlotLength() / 1000.0;
			SInominal = (allocationSlotLength/10.0 - 2*pTIFS) / 2*mClockAccuracy;

			// a beacon is our synchronization event. Update relevant timer
			setTimer(SYNC_INTERVAL_TIMEOUT, SInominal);


			RAP1Length = medWinBeacon->getRAP1Length();
			if (RAP1Length > 0)
				endTime = getClock() + RAP1Length * allocationSlotLength - beaconTxTime;

            if (connectedHID == UNCONNECTED) {
                if (scheduledAccessLength > 0) {
                    connectedHID = MedWinBeaconPacket->getHID();
                    // we are unconnected, and we need to connect to obtain scheduled access
                    // we will create and send a connection request
                    MedWinConnectionRequestPacket *connectionRequest = new MedWinConnectionRequestPacket("MedWin connection request packet",MAC_LAYER_PACKET);
                    connectionRequest->setRecipientAddress(medWinBeacon->getSenderAddress());
                    connectionRequest->setSenderAddress(SELF_MAC_ADDRESS);
                    // in this implementation our schedule always starts from the next beacon
                    connectionRequest->setNextWakeup(medWinBeacon->getSequenceNumber() + 1);
                    connectionRequest->setWakeupInterval(scheduledAccessPeriod);
                    //uplink request is simplified in this implementation to only ask for a number of slots needed
                    connectionRequest->setUplinkRequest(scheduledAccessLength);
                    setHeaderFields(connectionRequest,I_ACK,MANAGEMENT,CONNECTION_REQUEST);
                    packetToBeSent = connectionRequest;
                    currentPacketRetries = 0;
                    attemptTxInRAP();
                } else if (needToTx()) {
                    attemptTxInRAP();
                }

			// else we are connected already and previous filtering made sure
 			// that this beacon belongs to our BAN
            } else  {

                // schedule a timer to wake up for the next beacon (it might be m periods away
                setTimer(WAKEUP_FOR_BEACON, medWinBeacon->getBeaconPeriodLength()* scheduledAccessPeriod * allocationSlotLength - beaconTxTime - GUARD_TIME);

				// if we have a schedule that does not start after RAP, or our schedule
				// is not assigned yet, then go to sleep after RAP.  RAP === period length???
				if ((scheduledAccessStart == UNCONNECTED) || (scheduledAccessStart!= RAP1Length +1))
					setTimer(START_SLEEPING, RAP1Length * allocationSlotLength - beaconTxTime);

				// schedule the timer to go in and out of scheduled access
                if (scheduledAccessStart != UNCONNECTED) {
                    setTimer(SCHEDULED_ACCESS_START, scheduledAccessStart * allocationSlotLength - beaconTxTime - GUARD_TIME);
                    if (medWinBeacon->getBeaconPeriodLength()* scheduledAccessPeriod > scheduledAccessStart + scheduledAccessLength)
                    	setTimer(START_SLEEPING, (scheduledAccessStart + scheduledAccessLength) * allocationSlotLength - beaconTxTime);
                }
                // We are in Tx if you can
                if (needToTx()) attemptTxInRAP();
            }


            if (medWinBeacon->getBeaconPeriodLength() == medWinBeacon->getRAP1Length()) break;

            simtime_t interval = medWinBeacon->getBeaconPeriodLength() * allocationSlotLength;

            interval = timeToNextBeacon(interval,medWinBeacon->getBeaconShiftingSequenceIndex(),
                    medWinBeacon->getBeaconShiftingSequencePhase());
            interval -= guardTime() + beaconTxTime;
            setTimer(BEACON_RX_TIME,interval);

            break;
        }

        case ASSOCIATION:
        case DISSASSOCIATION:
        case PTK:
        case GTK: {
            trace() << "WARNING: unimplemented packet subtype in [" << medWinPkt->getName() << "]";
            break;
        }
    }
}

bool MedWinMacModule::isPacketForMe(MedWinPacket *pkt)

    int pktHID = pkt->getHID();
    int pktNID = pkt->getNID();

    if (connectedHID == pktHID)
        if ((connectedNID == pktNID) || (pktNID == BROADCAST)) return true;
        if (connectedNID != UNCONNECTED) return false;
        if ((unconnectedNID == pktNID) || (pktNID == UNCONNECTED_BROADCAST)) return true;
    } else if ((connectedHID == UNCONNECTED) && (pkt->getFrameSubtype()==BEACON)) return true;

    // for all other cases return false
    return false;
}

void MedWinMacModule::guardTime(){
    return allocationSlotLength / 10.0 + (getClock() - syncIntervalAdditionalStart) * mClockAccuracy;
}

void MedWinMacModule::setHeaderFields(MedWinPacket * pkt, AcknowledgementPolicy_type ackPolicy, Frame_type frameType, Frame_subtype frameSubtype) {
    pkt->setHID(connectedHID);
    if (connectedNID != UNCONNECTED)
        pkt->setNID(connectedNID);
    else
        pkt->setNID(unconnectedNID);
    pkt->setAckPolicy(ackPolicy);
    pkt->setFrameType(frameType);
    pkt->setFrameSubtype(frameSubtype);
}

void MedWinMacModule::attempTxInRAP() {
    if (backoffCounter == 0) {
        backoffCounter = 1 + genk_intrand(0,CW);
    }
    setTimer(CARRIER_SENSING,0);
}

simtime_t MedWinMacModule::timeToNextBeacon(simtime_t interval, int index, int phase) {
    return interval;
}

// This function will examine the current packet ...
//
bool MedWinMacModule::needToTx() {
    if (packetToBeSent) {
        if (currentPacketRetries < maxPacketRetries) return true;
        cancelAndDelete(packetToBeSent);
        packetToBeSent = NULL;
    }

    if (TXBuffer.size() == 0) return false;

    packetToBeSent = TXBuffer.front(); TXBuffer.pop();
    currentPacketRetries = 0;
    return true;
}


bool MedWinMacModule::canFitTx() {
    if (endTime - getClock() - guardTime - TX_TIME(packetToBeSent->getByteLength()) > 0) return true;
    return false;
}


void MedWinMacModule::sendPacket() {
    toRadioLayer(packetToBeSent);
    toRadioLayer(createRadioCommand(SET_STATE,TX));
    currentPacketRetries++;

    if (packetToBeSent->getAckPolicy() == I_ACK || packetToBeSent->getAckPolicy() == B_ACK) {
        // need to wait for ack
        setTimer(ACK_TIMEOUT,TX_TIME(packetToBeSent->getByteLength()) + 2*pTIFS + TX_TIME(MEDWIN_HEADER_SIZE));
    } else {
        // no need to wait
        setTimer(SEND_PACKET,TX_TIME(packetToBeSent->getByteLength()));
    }
}
