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

#include "MedWinMacModule.h"

Define_Module(MedWinMacModule);

void MedWinMacModule::startup() {
    connectedHID = UNCONNECTED;
    connectedNID = UNCONNECTED;
    unconnectedNID = 1 + genk_intrand(0,14);	//we select random unconnected NID
    scheduledAccessLength = par("ScheduledAccessLength");
    scheduledAccessPeriod = par("scheduledAccessPeriod");
    phyLayerOverhead = par("phyLayerOverhead");
    phyDataRate = par("phyDataRate");
    priority = getParentModule()->getParentModule()->findSubModule("nodeApplication")->par("priority");

    contentionSlot = par("contentionSlot");
    maxPacketRetries = par("packetRetries");
}

void MedWinMacModule::timerFiredCallback(int index) {
    switch (index) {
    
	case CARRIER_SENSING: {
	    if (!canFitTx()) { 
		macState = MAC_STATE_DEFAULT;
		break;
	    }
	    
	    CCA_result CCAcode = radioModule->isChannelClear();
	    if (CCAcode == CLEAR) {
		backoffCounter--;
		if (backoffCounter > 0) setTimer(CARRIER_SENSING,contentionSlot);
		else sendPacket();
	    } else {
		setTimer(CARRIER_SENSING,contentionSlot*10);
	    }
	    break;
	}
	
	case SEND_PACKET: {
	    if (needToTx() && canFitTx()) sendPacket(); 
	    break;
	}
	
	case ACK_TIMEOUT: {
	    if (!needToTx()) break;
	    if (macState == MAC_RAP) {
		attemptTxInRAP();
	    } else {
		sendPacket();
	    }
	    break;
	}
	
	case GO_TO_SLEEP: {
	    macstate = MAC_SLEEP;
	    toRadioLayer(createRadioCommand(SET_STATE,SLEEP));
	    break;
	}
	
	case SCHEDULED_ACCESS_START: {
	    macState = MAC_SCHEDULED_ACCESS;
	    if (needToTx() && canFitTx()) sendPacket();
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
		    //uplink request is simplified in this implementation to only ask for a number of sloots needed
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
		allocationSlotLength = medWinBeacon->getAllocationSlotLength() / 1000.0;

		if (scheduledAccessLength > 0) {
		    setTimer(SCHEDULED_ACCESS_START,scheduledAccessStart * allocationSlotLength - beaconTxTime - guardTime());
		    setTimer(GO_TO_SLEEP,(scheduledAccessStart + scheduledAccessLength) * allocationSlotLength - beaconTxTime - guardTime());
		}
		
		if (needToTx()) attemptTxInRAP();
	    }

	    endTime = RAPEndTime = getClock() + medWinBeacon->getRAP1Length() * allocationSlotLength - beaconTxTime
	    guardTime = medWinBeacon->getAllocationSlotLength() / 10;
	    if (medWinBeacon->getBeaconPeriodLength() == medWinBeacon->getRAP1Length()) break;

	    simtime_t interval = medWinBeacon->getBeaconPeriodLength() * allocationSlotLength;

	    interval = timeToNextBeacon(interval,medWinBeacon->getBeaconShiftingSequenceIndex(),
		    medWinBeacon->getBeaconShiftingSequencePhase());
	    interval -= guardTime() + beaconTxTime;
	    setTimer(BEACON_RX_TIME,interval);

	    if (scheduledAccessLength == 0 || scheduledAccessStart != medWinBeacon->getRAP1Length()) {
		setTimer(GO_TO_SLEEP, medWinBeacon->getRAP1Length() * allocationSlotLength - beaconTxTime);
	    }

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

    if (getClock - lastSyncTime < SInominal) return allocationSlotLength / 10.0;
    else return allocationSlotLength / 10.0 + ???;
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
	CW = CWmin[priority];
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
