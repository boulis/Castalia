/***************************************************************************
 *  Copyright: National ICT Australia,  2007 - 2010                        *
 *  Developed at the ATP lab, Networked Systems theme                      *
 *  Author(s): Athanassios Boulis, Yuriy Tselishchev                       *
 *  This file is distributed under the terms in the attached LICENSE file. *
 *  If you do not find this file, copies can be found by writing to:       *
 *                                                                         *
 *      NICTA, Locked Bag 9013, Alexandria, NSW 1435, Australia            *
 *      Attention:  License Inquiry.                                       *
 ***************************************************************************/

#include "TunableMAC.h"

Define_Module(TunableMAC);

void TunableMAC::startup()
{
	dutyCycle = par("dutyCycle");
	listenInterval = ((double)par("listenInterval")) / 1000.0;	// convert msecs to secs
	beaconIntervalFraction = par("beaconIntervalFraction");
	probTx = par("probTx");
	numTx = par("numTx");
	randomTxOffset = ((double)par("randomTxOffset")) / 1000.0;	// convert msecs to secs
	reTxInterval = ((double)par("reTxInterval")) / 1000.0;		// convert msecs to secs
	beaconFrameSize = par("beaconFrameSize");
	backoffType = par("backoffType");

	phyDataRate = par("phyDataRate");
	phyDelayForValidCS = (double)par("phyDelayForValidCS") / 1000.0;	//parameter given in ms in the omnetpp.ini
	phyLayerOverhead = par("phyFrameOverhead");

	switch (backoffType) {
		case 0:{
			if ((dutyCycle > 0.0) && (dutyCycle < 1.0))
				backoffType = BACKOFF_SLEEP_INT;
			else
				trace() << "Illegal value of parameter \"backoffType\" in omnetpp.ini.\n    Backoff timer = sleeping interval, but sleeping interval is not defined because duty cycle is zero, one, or invalid. Will use backOffBaseValue instead";
			backoffType = BACKOFF_CONSTANT;
			break;
		}
		case 1:{
			backoffType = BACKOFF_CONSTANT;
			break;
		}
		case 2:{
			backoffType = BACKOFF_MULTIPLYING;
			break;
		}
		case 3:{
			backoffType = BACKOFF_EXPONENTIAL;
			break;
		}
		default:{
			opp_error("\n[Mac]:\n Illegal value of parameter \"backoffType\" in omnetpp.ini.");
			break;
		}
	}
	backoffBaseValue = ((double)par("backoffBaseValue")) / 1000.0;	// just convert msecs to secs

	if ((dutyCycle > 0.0) && (dutyCycle < 1.0)) {
		sleepInterval = listenInterval * ((1.0 - dutyCycle) / dutyCycle);
		setTimer(START_SLEEPING, listenInterval);
	} else {
		sleepInterval = -1.0;
	}

	macState = MAC_STATE_DEFAULT;
	numTxTries = 0;
	remainingBeaconsToTx = 0;
	backoffTimes = 0;
}

void TunableMAC::timerFiredCallback(int timer)
{
	switch (timer) {

		case START_SLEEPING:{
			toRadioLayer(createRadioCommand(SET_STATE, SLEEP));
			setTimer(START_LISTENING, sleepInterval);
			break;
		}

		case START_LISTENING:{
			toRadioLayer(createRadioCommand(SET_STATE, RX));
			setTimer(START_SLEEPING, listenInterval);
			break;
		}

		case START_CARRIER_SENSING:{
			toRadioLayer(createRadioCommand(SET_STATE, RX));
			handleCarrierSenseResult(radioModule->isChannelClear());
			break;
		}

		case ATTEMPT_TX:{
			attemptTx();
			break;
		}

		case SEND_BEACONS_OR_DATA:{
			sendBeaconsOrData();
			break;
		}

		default:{
			trace() << "WARNING: unknown timer callback " << timer;
		}
	}
}

void TunableMAC::handleCarrierSenseResult(int returnCode)
{
	switch (returnCode) {

		case CLEAR:{
			/* We reset the backoff counter due to busy channel. Then we
			 * proceed to calculate the number of beacons required to be
			 * sent based on sleep interval and beacon interval fraction
			 */
			backoffTimes = 0;
			if ((dutyCycle > 0.0) && (dutyCycle < 1.0)) {
				double beaconTxTime = ((double)(beaconFrameSize +
				      phyLayerOverhead)) * 8.0 / (1000.0 * phyDataRate);
				double step = beaconTxTime + listenInterval / 2.0;
				remainingBeaconsToTx = (int)ceil(sleepInterval * beaconIntervalFraction / step);
			} else {
				remainingBeaconsToTx = 0;
			}
			macState = MAC_STATE_TX;
			trace() << "MAC_STATE_TX, sending " << remainingBeaconsToTx << " beacons";
			sendBeaconsOrData();
			break;
		}

		case BUSY:{
			double backoffTimer = 0;
			backoffTimes++;

			switch (backoffType) {
				case BACKOFF_SLEEP_INT:{
					backoffTimer = sleepInterval;
					break;
				}

				case BACKOFF_CONSTANT:{
					backoffTimer = backoffBaseValue;
					break;
				}

				case BACKOFF_MULTIPLYING:{
					backoffTimer = (double)(backoffBaseValue * backoffTimes);
					break;
				}

				case BACKOFF_EXPONENTIAL:{
					backoffTimer = backoffBaseValue * 
							pow(2.0, (double)(((backoffTimes - 1) < 0) ? 0 : backoffTimes - 1));
					break;
				}
			}

			backoffTimer = genk_dblrand(1) * backoffTimer;
			setTimer(START_CARRIER_SENSING, backoffTimer);
			trace() << "Channel busy, backing off and going to sleep for " << backoffTimer << " secs";

			/* Go directly to sleep. One could say "wait for listenInterval
			 * in the case we receive something". This is highly improbable
			 * though as most of the cases we start the process of carrier
			 * sensing from a sleep state (so most likely we have missed the
			 * start of the packet). In the case we are in listen mode, if 
			 * someone else is transmitting then we would have entered 
			 * MAC_STATE_RX and the carrier sense (and TX) would be postponed.
			 */
			toRadioLayer(createRadioCommand(SET_STATE, SLEEP));
			break;
		}

		case CS_NOT_VALID:
		case CS_NOT_VALID_YET:{
			setTimer(START_CARRIER_SENSING, phyDelayForValidCS);
			trace() << "CS not valid yet, trying again.";
			break;
		}
	}
}

void TunableMAC::fromNetworkLayer(cPacket * netPkt, int destination)
{
	TunableMacPacket *macPkt = new TunableMacPacket("TunableMac data packet", MAC_LAYER_PACKET);
	macPkt->setSource(SELF_MAC_ADDRESS);
	macPkt->setDestination(destination);
	macPkt->setFrameType(DATA_FRAME);
	encapsulatePacket(macPkt, netPkt);

	/* We always try to buffer the packet first */
	if (bufferPacket(macPkt)) {
		/* If the new packet is the only packet and we are in default state
		 * then we need to initiate transmission process for this packet.
		 * If there are more packets in the buffer, then transmission is
		 * already being handled
		 */
		if (macState == MAC_STATE_DEFAULT && TXBuffer.size() == 1) {
			/* First action to initiate new transmission is to deal with
			 * all scheduled timers. In particular we need to suspend
			 * duty cycle timers (if present) and attempt to transmit
			 * the new packet with a maximum number of retries left.
			 */
			if ((dutyCycle > 0.0) && (dutyCycle < 1.0)) {
				cancelTimer(START_LISTENING);
				cancelTimer(START_SLEEPING);
			}
			numTxTries = numTx;
			attemptTx();
		}
	} else {
		// bufferPacket failed, buffer is full
		// FULL_BUFFER control msg sent by virtualMAC code
		trace() << "WARNING Tunable MAC buffer overflow";
	}
}

void TunableMAC::attemptTx()
{
	trace() << "attemptTx(), buffer size: " << TXBuffer.size() << ", numTxTries: " << numTxTries;

	if (numTxTries <= 0) {
		/* We can enter attemptTx from many places, in some cases 
		 * the buffer may be empty. If its not empty but we have 
		 * 0 tries left, then the front message is deleted.
		 */
		if (TXBuffer.size() > 0) {
			cancelAndDelete(TXBuffer.front());
			TXBuffer.pop();
		}

		/* Check the buffer again to see if a new packet has to be 
		 * transmitted, otherwise change to default state and 
		 * reestablish the sleeping schedule.
		 */
		if (TXBuffer.size() > 0) {
			numTxTries = numTx;
			attemptTx();
		} else {
			macState = MAC_STATE_DEFAULT;
			trace() << "MAC_STATE_DEFAULT, no more pkts to attemptTx";
			/* We have nothing left to transmit, need to resume 
			 * sleep/listen pattern. Starting by going to sleep
			 * immediately (timer with 0 delay).
			 */
			if ((dutyCycle > 0.0) && (dutyCycle < 1.0))
				setTimer(START_SLEEPING, 0);
		}
		return;
	}

	macState = MAC_STATE_CONTENDING;

	if (genk_dblrand(0) < probTx) {
		// This transmission attempt will happen after random offset
		setTimer(START_CARRIER_SENSING, genk_dblrand(1) * randomTxOffset);
		trace() << "MAC_STATE_CONTENDING, attempt " << numTx - numTxTries +1 << "/" << numTx << " contending";
	} else {
		// Move on to the next attempt after reTxInterval
		setTimer(ATTEMPT_TX, reTxInterval);
		trace() << "MAC_STATE_CONTENDING, attempt " << numTx - numTxTries +1 << "/" << numTx << " skipped";
		numTxTries--;
	}
}

void TunableMAC::sendBeaconsOrData()
{

	if (remainingBeaconsToTx > 0) {
		remainingBeaconsToTx--;

		TunableMacPacket *beaconFrame =
			new TunableMacPacket("TunableMac beacon packet", MAC_LAYER_PACKET);
		beaconFrame->setSource(SELF_MAC_ADDRESS);
		beaconFrame->setDestination(BROADCAST_MAC_ADDRESS);
		beaconFrame->setFrameType(BEACON_FRAME);
		beaconFrame->setByteLength(beaconFrameSize);
		toRadioLayer(beaconFrame);
		toRadioLayer(createRadioCommand(SET_STATE, TX));

		/* Set timer to send next beacon (or data packet)
		 * We send beacons with half of the listen interval between them
		 */
		double beaconTxTime = ((double)(beaconFrameSize + phyLayerOverhead)) * 
				8.0 / (1000.0 * phyDataRate);
		setTimer(SEND_BEACONS_OR_DATA, beaconTxTime + listenInterval / 2.0);

	} else {
		if (TXBuffer.empty()) {
			trace() << "WARNING: sendBeaconsOrData called with empty TX buffer!";
			numTxTries = 0;
			attemptTx(); // this should set the sleeping patterns again.
			return;
		}

		toRadioLayer(TXBuffer.front()->dup());
		toRadioLayer(createRadioCommand(SET_STATE, TX));
		double packetTxTime = ((double)(TXBuffer.front()->getByteLength() +
		      phyLayerOverhead)) * 8.0 / (1000.0 * phyDataRate);
		numTxTries--;

		/* We are done sending a _copy_ of the packet from the front
		 * of the buffer. We now move on to either A) the next copy or
		 * B) the next packet if the current packet has no TX attempts
		 * remaining. In case A, we need to delay the attempt by
		 * reTxInterval. Otherwise, in case B, attempt to transmit the 
		 * next packet right away.
		 */
		if (numTxTries > 0) {
			setTimer(ATTEMPT_TX, packetTxTime + reTxInterval);
		} else {
			setTimer(ATTEMPT_TX, packetTxTime);
		}
	}
}

void TunableMAC::fromRadioLayer(cPacket * pkt, double rssi, double lqi)
{
	TunableMacPacket *macFrame = dynamic_cast <TunableMacPacket*>(pkt);
	if (macFrame == NULL)
		return;

	int destination = macFrame->getDestination();
	if (destination != SELF_MAC_ADDRESS && destination != BROADCAST_MAC_ADDRESS)
		return;

	switch (macFrame->getFrameType()) {

		case BEACON_FRAME:{
			if (macState == MAC_STATE_DEFAULT) {
				if ((dutyCycle > 0.0) && (dutyCycle < 1.0))
					cancelTimer(START_SLEEPING);
			} else if (macState == MAC_STATE_CONTENDING) {
				cancelTimer(START_CARRIER_SENSING);
				cancelTimer(ATTEMPT_TX);
			} else if (macState == MAC_STATE_TX) {
				/* We ignore the received beacon packet because we
				 * are in the process of sending our own data
				 */
				break;
			}
			macState = MAC_STATE_RX;
			trace() << "MAC_STATE_RX, received beacon";
			if ((dutyCycle > 0.0) && (dutyCycle < 1.0)) {
				setTimer(ATTEMPT_TX, sleepInterval);
			} else {
				trace() << "WARNING: received a beacon packet without duty cycle in place";
				/* This happens only when one node has duty cycle while
				 * another one does not. TunableMac was not designed for
				 * this case as more thought is required a possible 
				 * solution could be to include duration information
				 * in the beacon itself here we will just wait for 0.5
				 * secs to complete the reception  before trying to 
				 * transmit again.
				 */
				setTimer(ATTEMPT_TX, 0.5);
			}
			break;
		}

		case DATA_FRAME:{
			toNetworkLayer(macFrame->decapsulate());
			if (macState == MAC_STATE_RX) {
				cancelTimer(ATTEMPT_TX);
				attemptTx();
			}
			break;
		}
	}
}

int TunableMAC::handleControlCommand(cMessage * msg)
{
	TunableMacControlCommand *cmd = check_and_cast <TunableMacControlCommand*>(msg);

	switch (cmd->getTunableMacCommandKind()) {

		case SET_DUTY_CYCLE:{

			bool hadDutyCycle = false;
			// check if there was already a non zero valid duty cycle
			if ((dutyCycle > 0.0) && (dutyCycle < 1.0))
				hadDutyCycle = true;

			dutyCycle = cmd->getParameter();

			/* If a valid duty cycle is defined, calculate sleepInterval and
			 * start the periodic sleep/listen cycle using a timer
			 */
			if ((dutyCycle > 0.0) && (dutyCycle < 1.0)) {
				sleepInterval = listenInterval * ((1.0 - dutyCycle) / dutyCycle);
				if (!hadDutyCycle)
					setTimer(START_SLEEPING, listenInterval);

			} else {
				sleepInterval = -1.0;
				/* We now have no duty cycle, but we had it prior to this command
				 * Therefore we cancel all duty cycle related messages
				 * and ensure that radio is not stuck in sleeping state
				 * Since radio can sleep only when mac is in default state,
				 * we only wake up the radio in default state also
				 */
				if (hadDutyCycle) {
					cancelTimer(START_SLEEPING);
					cancelTimer(START_LISTENING);
					if (macState == MAC_STATE_DEFAULT)
						toRadioLayer(createRadioCommand(SET_STATE, RX));
				}
			}
			break;
		}

		case SET_LISTEN_INTERVAL:{

			double tmpValue = cmd->getParameter() / 1000.0;
			if (tmpValue < 0.0)
				trace() << "WARNING: invalid listen interval value sent to TunableMac";
			else {
				listenInterval = tmpValue;
				if ((dutyCycle > 0.0) && (dutyCycle < 1.0))
					sleepInterval = listenInterval * ((1.0 - dutyCycle) / dutyCycle);
				else
					sleepInterval = -1.0;
			}
			break;
		}

		case SET_BEACON_INTERVAL_FRACTION:{

			double tmpValue = cmd->getParameter();
			if ((tmpValue < 0.0) || (tmpValue > 1.0))
				trace() << "WARNING: invalid Beacon Interval Fraction value sent to TunableMac";
			else
				beaconIntervalFraction = tmpValue;
			break;
		}

		case SET_PROB_TX:{

			double tmpValue = cmd->getParameter();
			if ((tmpValue < 0.0) || (tmpValue > 1.0))
				trace() << "WARNING: invalid ProbTX value sent to TunableMac";
			else
				probTx = tmpValue;
			break;
		}

		case SET_NUM_TX:{

			double tmpValue = cmd->getParameter();
			if (tmpValue < 0 || tmpValue - ceil(tmpValue) != 0)
				trace() << "WARNING: invalid NumTX value sent to TunableMac";
			else
				numTx = (int)tmpValue;
			break;
		}

		case SET_RANDOM_TX_OFFSET:{

			double tmpValue = cmd->getParameter() / 1000.0;
			if (tmpValue <= 0.0)
				trace() << "WARNING: invalid randomTxOffset value sent to TunableMac";
			else
				randomTxOffset = tmpValue;
			break;
		}

		case SET_RETX_INTERVAL:{

			double tmpValue = cmd->getParameter() / 1000.0;
			if (tmpValue <= 0.0)
				trace() << "WARNING: invalid reTxInterval value sent to TunableMac";
			else
				reTxInterval = tmpValue;
			break;
		}

		case SET_BACKOFF_TYPE:{

			double tmpValue = cmd->getParameter();

			if (tmpValue == 0.0) {
				if ((dutyCycle > 0.0) && (dutyCycle < 1.0))
					backoffType = BACKOFF_SLEEP_INT;
				else
					trace() << "WARNING: invalid backoffType value sent to TunableMac. Backoff timer = sleeping interval, but sleeping interval is not defined because duty cycle is zero, one, or invalid.";
			}

			else if (tmpValue == 1.0) {
				if (backoffBaseValue <= 0.0)
					trace() << "WARNING: unable to set backoffType. Parameter backoffBaseValue has conflicting value";
				else
					backoffType = BACKOFF_CONSTANT;
			}

			else if (tmpValue == 2.0) {
				if (backoffBaseValue <= 0.0)
					trace() << "WARNING: unable to set backoffType. Parameter backoffBaseValue has conflicting value";
				else
					backoffType = BACKOFF_MULTIPLYING;
			}

			else if (tmpValue == 3.0) {
				if (backoffBaseValue <= 0.0)
					trace() << "WARNING: unable to set backoffType. Parameter backoffBaseValue has conflicting value";
				else
					backoffType = BACKOFF_EXPONENTIAL;
			}

			else
				trace() << "WARNING: invalid backoffType value sent to TunableMac";

			break;
		}

		case SET_BACKOFF_BASE_VALUE:{

			double tmpValue = cmd->getParameter() / 1000.0;
			if (tmpValue < 0)
				trace() << "WARNING: invalid backoffBaseValue sent to TunableMac";
			else
				backoffBaseValue = tmpValue;
			break;
		}
	}

	delete cmd;
	return 1;
}

