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
	isHub = par("isHub");
	if (isHub) {
		connectedHID = SELF_MAC_ADDRESS % (2<<16); // keep the 16 LS bits as a short address
		connectedNID = BROADCAST_NID; // default value, usually overwritten
		currentFreeConnectedNID = 16; // start assigning connected NID from ID 16
		allocationSlotLength = (double) par("allocationSlotLength")/1000.0; // convert msec to sec
		beaconPeriodLength = par("beaconPeriodLength");
		RAP1Length = currentScheduleAssignmentStart = par("RAP1Length");
		setTimer(SEND_BEACON,0);
	} else {
		connectedHID = UNCONNECTED;
 		connectedNID = UNCONNECTED;
		unconnectedNID = 1 + genk_intrand(0,14);    //we select random unconnected NID
		trace() << "Selected random unconnected NID " << unconnectedNID;
		scheduledAccessLength = par("scheduledAccessLength");
		scheduledAccessPeriod = par("scheduledAccessPeriod");
		pastSyncIntervalNominal = false;
		macState = MAC_SETUP;
		SInominal = -1;
	}

	pTIFS = (double) par("pTIFS")/1000.0;
	phyLayerOverhead = par("phyLayerOverhead");
	phyDataRate = par("phyDataRate");
	priority = getParentModule()->getParentModule()->getSubmodule("nodeApplication")->par("priority");
	mClockAccuracy = par("mClockAccuracy");
	enhanceGuardTime = par("enhanceGuardTime");

	contentionSlotLength = (double) par("contentionSlotLength")/1000.0; // convert msec to sec;
	maxPacketRetries = par("maxPacketRetries");

	CW = CWmin[priority];
	CWdouble = false;
	backoffCounter = 0;

	packetToBeSent = NULL;
	declareOutput("Data pkt breakdown");
	declareOutput("Mgmt & Ctrl pkt breakdown");
}

void MedWinMacModule::timerFiredCallback(int index) {
	switch (index) {

		case CARRIER_SENSING: {
			if (!canFitTx()) break;

			CCA_result CCAcode = radioModule->isChannelClear();
			if (CCAcode == CLEAR) {
				backoffCounter--;
				if (backoffCounter > 0) setTimer(CARRIER_SENSING, contentionSlotLength);
				else {
					sendPacket();
				}
			} else {
				/* spec states that we wait until the channel is not busy
				 * we cannot simply do that, we have to have periodic checks
				 * we arbitrarily choose 10*contention slot = 3.6msec
				 */
				setTimer(CARRIER_SENSING, contentionSlotLength*10.0);
			}
			break;
		}

		case START_ATTEMPT_TX: {
			attemptTX();
			break;
		}

		case ACK_TIMEOUT: {
			trace() << "ACK timeout fired";
			// double the Contention Window, after every second fail.
			CWdouble ? CWdouble=false : CWdouble=true;
			if ((CWdouble) && (CW < CWmax[priority])) CW *=2;

			// check if we reached the max number and if so delete the packet
			if (currentPacketRetries == maxPacketRetries) {
				// collect statistics
				if (packetToBeSent->getFrameType() == DATA)
					collectOutput("Data pkt breakdown", "Failed, No Ack");
				else collectOutput("Mgmt & Ctrl pkt breakdown", "Failed, No Ack");
				cancelAndDelete(packetToBeSent);
				packetToBeSent = NULL;
				currentPacketRetries = 0;
			}
			attemptTX();
			break;
		}

		case START_SLEEPING: {
			macState = MAC_SLEEP;
			toRadioLayer(createRadioCommand(SET_STATE,SLEEP));
			break;
		}

		case START_SCHEDULED_TX_ACCESS: {
			macState = MAC_SCHEDULED_TX_ACCESS;
			endTime = getClock() + (scheduledAccessEnd - scheduledAccessStart) * allocationSlotLength;
			if (beaconPeriodLength > scheduledAccessEnd)
				setTimer(START_SLEEPING, (scheduledAccessEnd - scheduledAccessStart) * allocationSlotLength);
			attemptTX();
			break;
		}

		/* this timer and corresponding state will be used for downlink traffic (sensors case)
		 * and uplink traffic (Hub case). Currently it is not used as we only have uplink traffic,
		 * (and the hub just uses the generic HUB_SCHEDULED_ACCESS timer).
		 */
		case START_SCHEDULED_RX_ACCESS: {
			macState = MAC_SCHEDULED_RX_ACCESS;
			toRadioLayer(createRadioCommand(SET_STATE,RX));
			if (beaconPeriodLength > scheduledRxAccessEnd)
				setTimer(START_SLEEPING, (scheduledRxAccessEnd - scheduledRxAccessStart) * allocationSlotLength);
			break;
		}

		case WAKEUP_FOR_BEACON: {
			macState = MAC_BEACON_WAIT;
			toRadioLayer(createRadioCommand(SET_STATE,RX));
			break;
		}

		case SYNC_INTERVAL_TIMEOUT: {
			pastSyncIntervalNominal = true;
			syncIntervalAdditionalStart = getClock();
			break;
		}

		// These timers are specific to a Hub
		case SEND_BEACON: {
			trace() << "BEACON SEND, next beacon in " << beaconPeriodLength * allocationSlotLength << "\n";
			macState = MAC_RAP;
			setTimer(SEND_BEACON, beaconPeriodLength * allocationSlotLength);
			setTimer(HUB_SCHEDULED_ACCESS, RAP1Length * allocationSlotLength);
			// the hub has to set its own endTime
			endTime = getClock() + RAP1Length * allocationSlotLength;

			MedWinBeaconPacket * beaconPkt = new MedWinBeaconPacket("MedWin beacon",MAC_LAYER_PACKET);
			setHeaderFields(beaconPkt,N_ACK_POLICY,MANAGEMENT,BEACON);
			beaconPkt->setNID(BROADCAST_NID);

			beaconPkt->setAllocationSlotLength((int)(allocationSlotLength*1000));
			beaconPkt->setBeaconPeriodLength(beaconPeriodLength);
			beaconPkt->setRAP1Length(RAP1Length);
			beaconPkt->setByteLength(MEDWIN_BEACON_SIZE);

			toRadioLayer(beaconPkt);
			toRadioLayer(createRadioCommand(SET_STATE,TX));

			setTimer(START_ATTEMPT_TX, (TX_TIME(beaconPkt->getByteLength()) + pTIFS) * (1 + mClockAccuracy));
			break;
		}

		case HUB_SCHEDULED_ACCESS: {
			macState = MAC_SCHEDULED_RX_ACCESS;
			// we should look at the schedule and setup timers to get in and out
			// of MAC_SCHEDULED_RX_ACCESS MAC_SCHEDULED_TX_ACCESS and finally MAC_SLEEP
			break;
		}
	}
}

void MedWinMacModule::fromNetworkLayer(cPacket *pkt, int dst) {
	MedWinMacPacket *medWinDataPkt = new MedWinMacPacket("MedWin data packet",MAC_LAYER_PACKET);
	encapsulatePacket(medWinDataPkt,pkt);
	if (bufferPacket(medWinDataPkt)) {
		attemptTX();
	} else {
		trace() << "WARNING MedWin MAC buffer overflow";
		collectOutput("Data pkt breakdown", "Fail, buffer overflow");
	}
}

void MedWinMacModule::fromRadioLayer(cPacket *pkt, double rssi, double lqi) {
	// if the incoming packet is not MedWin, return (VirtualMAC will delete it)
	MedWinMacPacket * medWinPkt = dynamic_cast<MedWinMacPacket*>(pkt);
	if (medWinPkt == NULL) return;

	// filter the incoming MedWin packet
    if (!isPacketForMe(medWinPkt)) return;

	// if the packet received requires an ACK, we should send it now
	if (medWinPkt->getAckPolicy() == I_ACK_POLICY) {
		MedWinMacPacket * ackPacket = new MedWinMacPacket("ACK packet",MAC_LAYER_PACKET);
		setHeaderFields(ackPacket,N_ACK_POLICY,CONTROL,I_ACK);
		ackPacket->setNID(medWinPkt->getNID());
		ackPacket->setByteLength(MEDWIN_HEADER_SIZE);
		// if we are unconnected (but the packet is for us since it was not filtered) we need to set a proper HID
		if (connectedHID == UNCONNECTED) ackPacket->setHID(medWinPkt->getHID());
		trace() << "transmitting ACK to/from NID:" << medWinPkt->getNID();
		toRadioLayer(ackPacket);
		toRadioLayer(createRadioCommand(SET_STATE,TX));
	}

	// Handle data packets
	if (medWinPkt->getFrameType() == DATA) {
		toNetworkLayer(decapsulatePacket(medWinPkt));
		// if this pkt requires a block ACK we should send it,
		// by looking what packet we have received
		// NOT IMPLEMENTED
		if (medWinPkt->getAckPolicy() == B_ACK_POLICY){
		}
		return;
	}

	// Handle management and control packets
	switch(medWinPkt->getFrameSubtype()) {
		case BEACON: {
			MedWinBeaconPacket * medWinBeacon = check_and_cast<MedWinBeaconPacket*>(medWinPkt);
			simtime_t beaconTxTime = TX_TIME(medWinBeacon->getByteLength());

			// get the allocation slot length, which is used in many calculations
			allocationSlotLength = medWinBeacon->getAllocationSlotLength() / 1000.0;
			SInominal = (allocationSlotLength/10.0 - pTIFS) / (2*mClockAccuracy);

			// a beacon is our synchronization event. Update relevant timer
			setTimer(SYNC_INTERVAL_TIMEOUT, SInominal);

			beaconPeriodLength = medWinBeacon->getBeaconPeriodLength();
			RAP1Length = medWinBeacon->getRAP1Length();
			if (RAP1Length > 0) {
				macState = MAC_RAP;
				endTime = getClock() + RAP1Length * allocationSlotLength - beaconTxTime;
			}
			trace() << "Beacon rx: reseting sync clock to " << SInominal << " secs";
			trace() << "           Slot= " << allocationSlotLength << " secs, beacon period= " << beaconPeriodLength << "slots";
			trace() << "           RAP1= " << RAP1Length << "slots, RAP ends at time: "<< endTime;

			if (connectedHID == UNCONNECTED) {
				/* We will try to connect to this BAN  if our scheduled access length
				 * is NOT set to unconnected (-1). If it is set to 0, it means we are
				 * establishing a sleeping pattern and waking up only to hear beacons
				 * and are only able to transmit in RAP periods.
				 */
				if (scheduledAccessLength >= 0) {
					// we are unconnected, and we need to connect to obtain scheduled access
					// we will create and send a connection request
					MedWinConnectionRequestPacket *connectionRequest = new MedWinConnectionRequestPacket("MedWin connection request packet",MAC_LAYER_PACKET);

					// This block takes care of general header fields
					setHeaderFields(connectionRequest,I_ACK_POLICY,MANAGEMENT,CONNECTION_REQUEST);
					// while setHeaderFields should take care of the HID field, we are currently unconnected.
					// We want to keep this state, yet send the request to the right hub.
					connectionRequest->setHID(medWinBeacon->getHID());

					// This block takes care of connection request specific fields
					connectionRequest->setRecipientAddress(medWinBeacon->getSenderAddress());
					connectionRequest->setSenderAddress(SELF_MAC_ADDRESS);
					// in this implementation our schedule always starts from the next beacon
					connectionRequest->setNextWakeup(medWinBeacon->getSequenceNumber() + 1);
					connectionRequest->setWakeupInterval(scheduledAccessPeriod);
					//uplink request is simplified in this implementation to only ask for a number of slots needed
					connectionRequest->setUplinkRequest(scheduledAccessLength);
					connectionRequest->setByteLength(MEDWIN_CONNECTION_REQUEST_SIZE);

					// Management packets go in their own buffer, and handled by attemptTX() with priority
					MgmtBuffer.push(connectionRequest);
					trace() << "           (unconnected): created connection request";
				}
			/* else we are connected already and previous filtering
			 * made sure that this beacon belongs to our BAN
			 */
			} else  {
				// schedule a timer to wake up for the next beacon (it might be m periods away
				setTimer(WAKEUP_FOR_BEACON, beaconPeriodLength * scheduledAccessPeriod * allocationSlotLength - beaconTxTime - GUARD_TIME );

				// if we have a schedule that does not start after RAP, or our schedule
				// is not assigned yet, then go to sleep after RAP.
				if ((scheduledAccessStart == UNCONNECTED && RAP1Length < beaconPeriodLength)
								|| (scheduledAccessStart > RAP1Length)) {
					setTimer(START_SLEEPING, RAP1Length * allocationSlotLength - beaconTxTime);
					trace() << "           --- start sleeping in: " << RAP1Length * allocationSlotLength - beaconTxTime << " secs";
				}
				// schedule the timer to go in scheduled access
				if ((scheduledAccessStart != UNCONNECTED) && (scheduledAccessLength > 0)) {
					setTimer(START_SCHEDULED_TX_ACCESS, scheduledAccessStart * allocationSlotLength - beaconTxTime + GUARD_TX_TIME);
					trace() << "           --- start scheduled TX access in: " << scheduledAccessStart * allocationSlotLength - beaconTxTime + GUARD_TX_TIME << " secs";
				}
			}
			attemptTX();
			break;
		}

		case I_ACK_POLL: {
			// handle the polling part and not use 'break' to let it roll over to the ACK part
		}
		case I_ACK: {
			// collect statistics
			if (currentPacketRetries == 1){
				if (packetToBeSent->getFrameType() == DATA)
					collectOutput("Data pkt breakdown", "Success, 1st try");
				else collectOutput("Mgmt & Ctrl pkt breakdown", "Success, 1st try");
			} else {
				if (packetToBeSent->getFrameType() == DATA)
					collectOutput("Data pkt breakdown", "Success, 2 or more tries");
				else collectOutput("Mgmt & Ctrl pkt breakdown", "Success, 2 or more tries");
			}
			cancelTimer(ACK_TIMEOUT);
			cancelAndDelete(packetToBeSent);
			packetToBeSent = NULL;
			currentPacketRetries = 0;
			CW = CWmin[priority];

			attemptTX();
			break;
		}

		case B_ACK_POLL: {
			// handle the polling part and not use 'break' to let it roll over to the ACK part
		}
		case B_ACK: {
			cancelTimer(ACK_TIMEOUT);
			cancelAndDelete(packetToBeSent);
			packetToBeSent = NULL;
			CW = CWmin[priority];

			// we need to analyze the bitmap and see if some of the LACK packets need to be retxed

			attemptTX();
			break;
		}

		case CONNECTION_ASSIGNMENT: {
			MedWinConnectionAssignmentPacket *connAssignment = check_and_cast<MedWinConnectionAssignmentPacket*>(medWinPkt);
			if (connAssignment->getStatusCode() == ACCEPTED || connAssignment->getStatusCode() == MODIFIED) {
				connectedHID = connAssignment->getHID();
				connectedNID = connAssignment->getAssignedNID();
				// set anew the header fields of the packet to be sent
				if (packetToBeSent){
					packetToBeSent->setHID(connectedHID);
					packetToBeSent->setNID(connectedNID);
				}
				// set the start and end times for the schedule
				scheduledAccessStart = connAssignment->getUplinkRequestStart();
				scheduledAccessEnd = connAssignment->getUplinkRequestEnd();
				trace() << "connected as NID " << connectedNID << "  --start TX access at slot: " << scheduledAccessStart << ", end at slot: " << scheduledAccessEnd;
			} // else we don't need to do anything - request is rejected
			else trace() << "Connection Request REJECTED, status code: " << connAssignment->getStatusCode();

			break;
		}

		case DISCONNECTION: {
			connectedHID = UNCONNECTED;
			connectedNID = UNCONNECTED;
			break;
		}

		case CONNECTION_REQUEST: {
			MedWinConnectionRequestPacket *connRequest = check_and_cast<MedWinConnectionRequestPacket*>(medWinPkt);
			trace() << "Connection request from NID " << connRequest->getNID() << " assigning new NID " << currentFreeConnectedNID;

			/* The ACK for the connection req packet is handled by the general code.
			 * Here we need to create the connection assignment packet and decide
			 * when to send it. We treat management packets that need ack similar
			 * to data packets, but with higher priority. They have their own buffer.
			 */
			MedWinConnectionAssignmentPacket *connAssignment = new MedWinConnectionAssignmentPacket("MedWin connection assignment",MAC_LAYER_PACKET);
			setHeaderFields(connAssignment,I_ACK_POLICY,MANAGEMENT,CONNECTION_ASSIGNMENT);
			// this is the assigned NID and it is part of the payload
			connAssignment->setAssignedNID(currentFreeConnectedNID);
			// this is the unconnected NID that goes in the header. Used for addressing
			connAssignment->setNID(connRequest->getNID());

			connAssignment->setRecipientAddress(connRequest->getSenderAddress());
			connAssignment->setByteLength(MEDWIN_CONNECTION_ASSIGNMENT_SIZE);

			if (connRequest->getUplinkRequest() > beaconPeriodLength - currentScheduleAssignmentStart) {
				connAssignment->setStatusCode(REJ_NO_RESOURCES);
				// can not accomodate request
			} else if (currentFreeConnectedNID > 239) {
				connAssignment->setStatusCode(REJ_NO_NID);
			} else {
				connAssignment->setStatusCode(ACCEPTED);
				currentFreeConnectedNID++;
				connAssignment->setUplinkRequestStart(currentScheduleAssignmentStart);
				connAssignment->setUplinkRequestEnd(currentScheduleAssignmentStart + connRequest->getUplinkRequest());
				currentScheduleAssignmentStart += connRequest->getUplinkRequest();
			}

			MgmtBuffer.push(connAssignment);
			// attempt to TX it after TXing the I-ACK to the connection req is finished
			// the MEDWIN_HEADER_SIZE below is the size of an ACK packet
			setTimer(START_ATTEMPT_TX, (TX_TIME(MEDWIN_HEADER_SIZE) + pTIFS) * (1 + mClockAccuracy));
			trace() << "Conn assgnmnt created, wait for " << (TX_TIME(MEDWIN_HEADER_SIZE) + pTIFS) * (1 + mClockAccuracy) << " to attempTX";
			break;
		}

		case T_POLL:
			// just read the time values from the payload, update relevant variables
			// and roll over to handle the POLL part (no break)
		case POLL: {
			// check if this is an immediate (not future) poll
			if (medWinPkt->getMoreData() == 0){
				setTimer(START_SCHEDULED_TX_ACCESS, 0);
				endTime = getClock() + pTIFS + medWinPkt->getSequenceNumber() * allocationSlotLength;
			}
			// else treat this as a POST: a post of the polling message coming in the future
			else {
				// seqNum holds the num of allocation slots in the future and fragNum the num of beacon periods in the future
				double postTime = (medWinPkt->getSequenceNumber() + medWinPkt->getFragmentNumber()* beaconPeriodLength) * allocationSlotLength ;
				setTimer(START_SCHEDULED_RX_ACCESS, postTime);
			}
			break;
		}

		case ASSOCIATION:
		case DISASSOCIATION:
		case PTK:
		case GTK: {
			trace() << "WARNING: unimplemented packet subtype in [" << medWinPkt->getName() << "]";
			break;
		}
	}
}

/* A function to filter incoming MedWin packets.
 * Works for both hub or sensor as a receiver.
 */
bool MedWinMacModule::isPacketForMe(MedWinMacPacket *pkt) {
	int pktHID = pkt->getHID();
	int pktNID = pkt->getNID();
	// trace() << "pktHID=" << pktHID << ", pktNID=" << pktNID << ", connectedHID=" << connectedHID << ", unconnectedNID=" << unconnectedNID;
	if (connectedHID == pktHID) {
		if (isHub) return true;
		if ((connectedNID == pktNID) || (pktNID == BROADCAST_NID)) return true;
	} else if ((connectedHID == UNCONNECTED) &&
		   ((unconnectedNID == pktNID) || (pktNID == UNCONNECTED_BROADCAST_NID) || (pktNID == BROADCAST_NID))){
		if (pkt->getFrameSubtype() == CONNECTION_ASSIGNMENT) {
			MedWinConnectionAssignmentPacket *connAssignment = check_and_cast<MedWinConnectionAssignmentPacket*>(pkt);
			if (connAssignment->getRecipientAddress() != SELF_MAC_ADDRESS) {
				// the packet is not for us, but the NID is the same, so we need to choose a new one.
				unconnectedNID = 1 + genk_intrand(0,14);
				trace() << "Choosing NEW unconnectedNID = " << unconnectedNID;
				return false;
			}
		}
		// for all other cases of HID == UNCONNECTED, return true
		return true;
	}

	// for all other cases return false
	return false;
}

/* A function to calculate the extra guard time, if we are past the Sync time nominal.
 */
simtime_t MedWinMacModule::extraGuardTime() {
	return (simtime_t) (getClock() - syncIntervalAdditionalStart) * mClockAccuracy;
}

/* A function to set the header fields of a packet.
 * It works with both hub- and sensor-created packets
 */
void MedWinMacModule::setHeaderFields(MedWinMacPacket * pkt, AcknowledgementPolicy_type ackPolicy, Frame_type frameType, Frame_subtype frameSubtype) {
	pkt->setHID(connectedHID);
	if (connectedNID != UNCONNECTED)
		pkt->setNID(connectedNID);
	else
		pkt->setNID(unconnectedNID);

	pkt->setAckPolicy(ackPolicy);
	pkt->setFrameType(frameType);
	pkt->setFrameSubtype(frameSubtype);
}

/* TX in RAP requires contending for the channel (a carrier sensing scheme)
 * this function prepares an important variable and starts the process.
 * It is used by the more generic attemptTX() function.
 */
void MedWinMacModule::attemptTxInRAP() {
	if (backoffCounter == 0) {
		backoffCounter = 1 + genk_intrand(0,CW);
	}
	setTimer(CARRIER_SENSING,0);
}

/* This function will attempt to TX in all TX access states(RAP, scheduled)
 * It will check whether we need to retransmit the current packet, or prepare
 * a new packet from the MAC data buffer or the Management buffer to be sent.
 */
void MedWinMacModule::attemptTX() {
	trace() << "ATTEMPT TX in state " << macState << "  Data buffer size= " << TXBuffer.size() << " Mgmt buffer size= " <<  MgmtBuffer.size();
	if (packetToBeSent && currentPacketRetries < maxPacketRetries) {
		//trace() << "ATTEMPT_TX trying to send previously stored packet";
		if (macState == MAC_RAP) attemptTxInRAP();
		if ((macState == MAC_SCHEDULED_TX_ACCESS) && (canFitTx())) sendPacket();
		return;
	}
	/* Else we check for cases to return without doing anything:
	 * 1) There is a packet to be sent (which has reached the max
	 * # of retries). In this case we are waiting for an I_ACK or
	 * ACK_TIMEOUT to delete the packet. 2) We are not in a state
	 * that allows us to TX. If none of these cases is true then
	 * we draw a new packet from the data or Management buffers.
	 */
	if (packetToBeSent) {return;}

	// If we are not in a TX access state, return
	if (macState != MAC_RAP && macState != MAC_SCHEDULED_TX_ACCESS) {return;}

	if (MgmtBuffer.size() !=0) {
		packetToBeSent = (MedWinMacPacket*)MgmtBuffer.front();  MgmtBuffer.pop();
		if (MgmtBuffer.size() > MGMT_BUFFER_SIZE)
			trace() << "WARNING: Management buffer reached a size of " << MgmtBuffer.size();
	} else if (TXBuffer.size() != 0){
		packetToBeSent = (MedWinMacPacket*)TXBuffer.front();   TXBuffer.pop();
		setHeaderFields(packetToBeSent, I_ACK_POLICY, DATA, RESERVED);
	}
	// if we found a packet in any of the buffers, try to TX it.
	if (packetToBeSent){
		if (macState == MAC_RAP) attemptTxInRAP();
		if ((macState == MAC_SCHEDULED_TX_ACCESS) && (canFitTx())) sendPacket();
	}
}

/* This function lets us know if a transmission fits in the time we have (scheduled or RAP)
 * It takes into account guard times too. A small issue exists with scheduled access:
 * Sleeping is handled at the timer code, which does not take into account the guard times.
 * In fact if we TX once then we'll stay awake for the whole duration of the scheduled slot.
 */
bool MedWinMacModule::canFitTx() {
	if (!packetToBeSent) return false;
	if ( endTime - getClock() - GUARD_FACTOR * GUARD_TIME - TX_TIME(packetToBeSent->getByteLength()) > 0) return true;
	return false;
}


/* Sends a packet to the radio and either waits for an ack or restart the attemptTX process
 */
void MedWinMacModule::sendPacket() {
	trace() << "transmitting [" << packetToBeSent->getName() << "]";

	if (packetToBeSent->getAckPolicy() == I_ACK_POLICY || packetToBeSent->getAckPolicy() == B_ACK_POLICY) {
		// need to wait for ack
		trace() << "setting ACK_TIMEOUT to " << TX_TIME(packetToBeSent->getByteLength()) + 2*pTIFS + TX_TIME(MEDWIN_HEADER_SIZE);
		setTimer(ACK_TIMEOUT, (TX_TIME(packetToBeSent->getByteLength()) + 2*pTIFS + TX_TIME(MEDWIN_HEADER_SIZE)) * (1 + mClockAccuracy));

		currentPacketRetries++;
		toRadioLayer(packetToBeSent->dup());
		toRadioLayer(createRadioCommand(SET_STATE,TX));

	} else {
		// no need to wait for ack, no need to dub the pkt, try TX other packets
		toRadioLayer(packetToBeSent); // no need to dup() we are only sending it once
		toRadioLayer(createRadioCommand(SET_STATE,TX));
		// start the process of attempting to TX again. This is more fair for contention-based periods
		setTimer(START_ATTEMPT_TX, TX_TIME(packetToBeSent->getByteLength()));
		packetToBeSent = NULL; // do not delete the message, just make the packetToBeSent placeholded available
	}
}

/* Not currently implemented. In the future useful if we implement the beacon shift sequences
 */
simtime_t MedWinMacModule::timeToNextBeacon(simtime_t interval, int index, int phase) {
	return interval;
}
