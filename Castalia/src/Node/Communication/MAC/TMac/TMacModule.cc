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

#include <cmath>
#include "TMacModule.h"

#define BROADCAST_ADDR -1
#define NAV_EXTENSION 		0.0001
#define TX_TIME(x)		(phyLayerOverhead + x)*1/(1000*radioDataRate/8.0)		//x are in BYTES

Define_Module(TMacModule);

void TMacModule::startup() {
    printDebugInfo = par("printDebugInfo");
    printDroppedPackets = par("printDroppedPackets");
    printStateTransitions = par("printStateTransitions");
    maxMACFrameSize = par("maxMACFrameSize");
    macFrameOverhead = par("macFrameOverhead");
    ackFrameSize = par("ackFrameSize");
    macBufferSize = par("macBufferSize");
    frameTime = ((double)par("frameTime"))/1000.0;		// just convert msecs to secs
    syncFrameSize = par("syncFrameSize");
    rtsFrameSize = par("rtsFrameSize");
    ctsFrameSize = par("ctsFrameSize");
    resyncTime =  par("resyncTime");
    allowSinkSync = par("allowSinkSync");
    contentionPeriod = ((double)par("contentionPeriod"))/1000.0;		// just convert msecs to secs
    listenTimeout = ((double)par("listenTimeout"))/1000.0;		// TA: just convert msecs to secs (15ms default);
    waitTimeout = ((double)par("waitTimeout"))/1000.0;
    useFRTS = par("useFrts");
    useRtsCts = par("useRtsCts");
    maxTxRetries = par("maxTxRetries");
    
    disableTAextension = par("disableTAextension");
    conservativeTA = par("conservativeTA");
    if (conservativeTA != 1 && conservativeTA != 0) {
	trace() << "Unknown value for parameter 'ConservativeTA', will default to 0";
	conservativeTA = 0;
    }
    collisionResolution = par("collisionResolution");
    if (collisionResolution != 2 && collisionResolution != 1 && collisionResolution != 0) {
	trace() << "Unknown value for parameter 'collisionResolution', will default to 0";
	collisionResolution = 0;
    }

    //Initialise state descriptions	used in debug output
    if (printStateTransitions) {
        stateDescr[100] = "MAC_STATE_SETUP";
        stateDescr[101] = "MAC_STATE_SLEEP";
        stateDescr[102] = "MAC_STATE_ACTIVE";
        stateDescr[103] = "MAC_STATE_ACTIVE_SILENT";
        stateDescr[104] = "MAC_STATE_IN_TX";
        stateDescr[110] = "MAC_CARRIER_SENSE_FOR_TX_RTS";
        stateDescr[111] = "MAC_CARRIER_SENSE_FOR_TX_DATA";
        stateDescr[112] = "MAC_CARRIER_SENSE_FOR_TX_CTS";
        stateDescr[113] = "MAC_CARRIER_SENSE_FOR_TX_ACK";
        stateDescr[114] = "MAC_CARRIER_SENSE_FOR_TX_SYNC";
        stateDescr[115] = "MAC_CARRIER_SENSE_BEFORE_SLEEP";
        stateDescr[120] = "MAC_STATE_WAIT_FOR_DATA";
        stateDescr[121] = "MAC_STATE_WAIT_FOR_CTS";
        stateDescr[122] = "MAC_STATE_WAIT_FOR_ACK";
    }

    radioDataRate = (double)radioModule->par("dataRate");
    radioDelayForSleep2Listen = ((double) radioModule->par("delaySleep2Listen"))/1000.0;
    radioDelayForValidCS = ((double) radioModule->par("delayCSValid"))/1000.0; //parameter given in ms in the omnetpp.ini
    phyLayerOverhead = radioModule->par("phyFrameOverhead");

    //try to obtain the value of isSink parameter from application module
    if(getParentModule()->getParentModule()->findSubmodule("nodeApplication") != -1) {
        cModule *tmpApplication = getParentModule()->getParentModule()->getSubmodule("nodeApplication");
        isSink = tmpApplication->hasPar("isSink") ? tmpApplication->par("isSink") : false;
    } else {
        isSink = false;
    }

    syncTxTime = ((double)(syncFrameSize + macFrameOverhead + phyLayerOverhead)) * 8.0 / (1000.0 * radioDataRate);
    rtsTxTime = ((double)(rtsFrameSize + macFrameOverhead + phyLayerOverhead)) * 8.0 / (1000.0 * radioDataRate);
    ackTxTime = ((double)(ackFrameSize + macFrameOverhead + phyLayerOverhead)) * 8.0 / (1000.0 * radioDataRate);
    ctsTxTime = ((double)(ctsFrameSize + macFrameOverhead + phyLayerOverhead)) * 8.0 / (1000.0 * radioDataRate);

    syncFrame = NULL;
    rtsFrame = NULL;
    ackFrame= NULL;
    ctsFrame = NULL;

    macState = MAC_STATE_SETUP;
    scheduleTable.clear();
    primaryWakeup = true;
    needResync = 0;
    currentFrameStart = -1;
    activationTimeout = 0;

    declareOutput("Sent packets breakdown");

    if (isSink && allowSinkSync) {
	createPrimarySchedule();
    } else {
	setTimer(SYNC_SETUP,allowSinkSync ? 2*frameTime : 0);
    }
}

void TMacModule::timerFiredCallback(int timer) {
    switch (timer) {

	case SYNC_SETUP: {
	    /* Timeout to hear a schedule packet has expired at this stage, 
	     * MAC is able to create its own schedule after a random offset
	     * within the duration of 1 frame
	     */
	    if (macState == MAC_STATE_SETUP) setTimer(SYNC_CREATE,genk_dblrand(1)*frameTime);
	    break;
	}

	case SYNC_CREATE: {
	    /* Random offset selected for creating a new schedule has expired. 
	     * If at this stage still no schedule was received, MAC creates 
	     * its own schedule and tries to broadcast it
	     */
	    if (macState == MAC_STATE_SETUP) createPrimarySchedule();
	    break;
	}
	
	case SYNC_RENEW: {
	    /* This node is the author of its own primary schedule
	     * It is required to rebroadcast a SYNC packet and also 
	     * schedule a self message for the next RESYNC procedure.
	     */
	    trace() << "Initiated RESYNC procedure";
	    scheduleTable[0].SN++;
	    needResync = 1;
	    setTimer(SYNC_RENEW,resyncTime);
	    break;
	}
	
	case FRAME_START: {
	    /* primaryWakeup variable is used to distinguish between primary and secondary schedules
	     * since the primary schedule is always the one in the beginning of the frame, we set
	     * primaryWakeup to true here.
	     */
	    primaryWakeup = true;

	    // record the current time and extend activation timeout
	    currentFrameStart = activationTimeout = simTime();
	    extendActivePeriod();

	    // schedule the message to start the next frame. Also check for frame offsets
	    // (if we received a RESYNC packet, frame start time could had been shifted due to
	    // clock drift - in this case it is necessary to rebroadcast this resync further)
	    setTimer(FRAME_START,frameTime);
	    if (scheduleTable[0].offset != 0) {
	        trace() << "New frame started, shifted by " << scheduleTable[0].offset;
		scheduleTable[0].offset = 0;
		needResync = 1;
	    } else {
		trace() << "New frame started";
	    }

	    // schedule wakeup messages for secondary schedules within the current frame only
	    for (int i = 1; i < (int)scheduleTable.size(); i++) {
		if (scheduleTable[i].offset < 0) { scheduleTable[i].offset += frameTime; }
		setTimer(WAKEUP_SILENT+i,scheduleTable[i].offset);
	    }

	    // finally, if we were sleeping, need to wake up the radio. And reset the internal MAC
	    // state (to start contending for transmissions if needed)
	    if (macState == MAC_STATE_SLEEP) setRadioState(MAC_2_RADIO_ENTER_LISTEN);
	    if (macState == MAC_STATE_SLEEP || macState == MAC_STATE_ACTIVE ||
		    macState == MAC_STATE_ACTIVE_SILENT) {
		resetDefaultState();
	    }
	    break;
	}
	
	case CHECK_TA: {
	    /* It is necessary to check activation timeout.
	     * We may need to extend the timeout here based on the current MAC state, or
	     * if timeout expired and we have no reason to extend it, then we need to go to sleep.
	     */
	    if (activationTimeout <= simTime()) {
		
		//if disableTAextension is on, then we will behave as SMAC - simply go to sleep if the active period is over
		if (disableTAextension) {
		    primaryWakeup = false;
		    // update MAC and RADIO states
		    setRadioState(MAC_2_RADIO_ENTER_SLEEP);
		    setMacState(MAC_STATE_SLEEP);
		}
		
		//otherwise, check MAC state and extend active period or go to sleep
		else if (conservativeTA) {
		    if (macState != MAC_STATE_ACTIVE && macState != MAC_STATE_ACTIVE_SILENT && macState != MAC_STATE_SLEEP) {
			extendActivePeriod();
		    } else {
			performCarrierSense(MAC_CARRIER_SENSE_BEFORE_SLEEP);
		    }
		}
		
		else {
		    primaryWakeup = false;
		    // update MAC and RADIO states
		    setRadioState(MAC_2_RADIO_ENTER_SLEEP);
		    setMacState(MAC_STATE_SLEEP);
		} 
	    }
	    break;
	}
	
	case CARRIER_SENSE: {
	    /* First it is important to check for valid MAC state
	     * If we heard something on the radio while waiting to start carrier sense,
	     * then MAC was set to MAC_STATE_ACTIVE_SILENT. In this case we can not transmit
	     * and there is no point to perform carrier sense
	     */
	    if(macState == MAC_STATE_ACTIVE_SILENT || macState == MAC_STATE_SLEEP) break;

	    // At this stage MAC can only be in one of the states MAC_CARRIER_SENSE_...
	    if(macState != MAC_CARRIER_SENSE_FOR_TX_RTS && macState != MAC_CARRIER_SENSE_FOR_TX_CTS &&
		    macState != MAC_CARRIER_SENSE_FOR_TX_SYNC && macState != MAC_CARRIER_SENSE_FOR_TX_DATA &&
		    macState != MAC_CARRIER_SENSE_FOR_TX_ACK && macState != MAC_CARRIER_SENSE_BEFORE_SLEEP) {
		trace() << "WARNING: bad MAC state for MAC_SELF_PERFORM_CARRIER_SENSE";
		break;
	    }
	    carrierSense();
	    break;
	}
	
	case TRANSMISSION_TIMEOUT: {
	    resetDefaultState();
	    break;
	}
	
	case WAKEUP_SILENT: {
	    /* This is the wakeup timer for secondary schedules.
	     * here we only wake up the radio and extend activation timeout for listening.
	     * NOTE that default state for secondary schedules is MAC_STATE_ACTIVE_SILENT
	     */
	
	    activationTimeout = simTime();
	    extendActivePeriod();
	    if(macState == MAC_STATE_SLEEP) {
		setRadioState(MAC_2_RADIO_ENTER_LISTEN);
		resetDefaultState();
	    }
	    break;
	}
	

	default: {
	    int tmpTimer = timer - WAKEUP_SILENT;
	    if (tmpTimer > 0 && tmpTimer < (int)scheduleTable.size()) {
	        activationTimeout = simTime();
		extendActivePeriod();
		if(macState == MAC_STATE_SLEEP) {
		    setRadioState(MAC_2_RADIO_ENTER_LISTEN);
		    resetDefaultState();
		}
	    } else {
	        trace() << "Unknown timer " << timer;
	    }
	}
    }
}

void TMacModule::carrierSenseCallback(int returnCode) {
    switch (returnCode) {
	
	case CARRIER_CLEAR: {
	    carrierIsClear();
	    break;
	}
	
	case CARRIER_BUSY: {
	    /* Since we are hearing some communication on the radio we need to do two things:
	     * 1 - extend our active period
	     * 2 - set MAC state to MAC_STATE_ACTIVE_SILENT unless we are actually expecting to receive
	     *     something (or sleeping)
	     */
	    if (macState == MAC_STATE_SETUP || macState == MAC_STATE_SLEEP) break;
	    if (!disableTAextension) extendActivePeriod();
	    if (collisionResolution == 0) {
		if (macState == MAC_CARRIER_SENSE_FOR_TX_RTS || macState == MAC_CARRIER_SENSE_FOR_TX_DATA ||
		    macState == MAC_CARRIER_SENSE_FOR_TX_CTS || macState == MAC_CARRIER_SENSE_FOR_TX_ACK ||
		    macState == MAC_CARRIER_SENSE_FOR_TX_SYNC || macState == MAC_CARRIER_SENSE_BEFORE_SLEEP) {
			resetDefaultState();
		}
	    } else {
		if (macState != MAC_STATE_WAIT_FOR_ACK && macState != MAC_STATE_WAIT_FOR_DATA &&
		    macState != MAC_STATE_WAIT_FOR_CTS) 
			setMacState(MAC_STATE_ACTIVE_SILENT);
	    }
	    break;
	}
	
	case RADIO_IN_TX_MODE: {
	    /* This can happen if we did not allow sufficient time to the radio
	     * to finish the previous transmission. So we try to resend the
	     * carrier sense message with a delay
	     */
	    trace() << "WARNING: carrier sense while radio is in TX mode";
	    setTimer(CARRIER_SENSE,radioDelayForValidCS);
	    break;
	}
	
	case RADIO_SLEEPING: {
	    // This should not happen unless MAC and radio states get out of sync for an
	    // unknown reason. We will try to wake up the radio to synchronise the states
	    trace() << "Radio is not ready to carrier sense yet ... will retry shortly";
	    if (macState != MAC_STATE_SLEEP) {
		setRadioState(MAC_2_RADIO_ENTER_LISTEN);
		setTimer(CARRIER_SENSE,radioDelayForValidCS);
	    }
	    break;
	}
	
	case RADIO_NON_READY: {
	    // Radio is not ready yet, resend the message to retry carrier sense after delay
	    setTimer(CARRIER_SENSE,radioDelayForValidCS);
	    break;
	}
	
	default: {
	    //Unknown reason for carrier sense fail
	    trace() << "Unknown carrier sense indication of Radio: "<<returnCode;
	    resetDefaultState();
	}
    }
}

void TMacModule::fromNetworkLayer(MAC_GenericFrame *macFrame) {
    if (bufferFrame(macFrame)) {
	if (TXBuffer.size() == 1) checkTxBuffer();
    }
}

void TMacModule::finishSpecific() {

    if (packetsSent.size() > 0) {
	trace() << "Sent packets breakdown: ";
	int total = 0;
	for (map<string,int>::iterator i = packetsSent.begin();
	    i != packetsSent.end(); i++) {
	    trace() << i->first << ": " << i->second;
	    total += i->second;
	}
	trace() << "Total: " << total << "\n";
    }
}


/* This function will reset the internal MAC state in the following way:
 * 1 -  Check if MAC is still in its active time, if timeout expired - go to sleep.
 * 2 -  Check if MAC is in the primary schedule wakeup (if so, MAC is able to start transmissions
 *	of either SYNC, RTS or DATA packets after a random contention offset.
 * 3 -  IF this is not primary wakeup, MAC can only listen, thus set state to MAC_STATE_ACTIVE_SILENT
 */
void TMacModule::resetDefaultState()  {
    if (activationTimeout <= simTime()) {
	performCarrierSense(MAC_CARRIER_SENSE_BEFORE_SLEEP);
    } else if (primaryWakeup) {
	if (needResync) {
	    scheduleSyncFrame(genk_dblrand(1)*contentionPeriod);
	    return;
	}
	while (!TXBuffer.empty()) {
	    if (txRetries <= 0) {
		trace() << "Transmission failed to " << txAddr;
		popTxBuffer();
	    } else {
		if (useRtsCts && txAddr != BROADCAST_ADDR) {
		    performCarrierSense(MAC_CARRIER_SENSE_FOR_TX_RTS,genk_dblrand(1)*contentionPeriod);
		} else {
		    performCarrierSense(MAC_CARRIER_SENSE_FOR_TX_DATA,genk_dblrand(1)*contentionPeriod);
		}
		return;
	    }
	}
	setMacState(MAC_STATE_ACTIVE);
    } else {
	//primaryWakeup == false
	setMacState(MAC_STATE_ACTIVE_SILENT);
    }
}

/* This function will create a new primary schedule for this node.
 * Most of the task is delegated to updateScheduleTable function
 * This function will only schedule a self message to resycnronise the newly created
 * schedule
 */
void TMacModule::createPrimarySchedule() {
    updateScheduleTable(frameTime,self,0);
    setTimer(SYNC_RENEW,resyncTime);
}

/* Helper function to change internal MAC state and print a debug statement if neccesary */
void TMacModule::setMacState(int newState) {
    if (macState == newState) return;
    if (printStateTransitions) {
	trace() << "state changed from " << stateDescr[macState] << " to " << stateDescr[newState];
    }
    macState = newState;
}

/* This function will create a SYNC frame based on the current schedule table
 * Note that only SYNC frames of primary schedule can be created. Secondary schedule
 * SYNC frames are never created
 */
void TMacModule::scheduleSyncFrame(simtime_t delay) {
    if (scheduleTable.size() < 1) {
	trace() << "WARNING: unable to schedule Sync frame without a schedule!";
	return;
    }
    TMacSchedule sch = scheduleTable[0];
    if (syncFrame) cancelAndDelete(syncFrame);
    syncFrame = new MAC_GenericFrame("SYNC message", MAC_FRAME);
    syncFrame->setKind(MAC_FRAME) ;
    syncFrame->getHeader().srcID = self;
    syncFrame->getHeader().destID = BROADCAST_ADDR;
    syncFrame->getHeader().SYNC = currentFrameStart + frameTime - simTime() - delay - syncTxTime;
    syncFrame->getHeader().SYNC_SN = sch.SN;
    syncFrame->getHeader().frameType = MAC_PROTO_SYNC_FRAME;
    syncFrame->setByteLength(syncFrameSize);
    performCarrierSense(MAC_CARRIER_SENSE_FOR_TX_SYNC,delay);
}


/* This function will update schedule table with the given values for wakeup time,
 * schedule ID and schedule SN
 */
void TMacModule::updateScheduleTable(simtime_t wakeup, int ID, int SN) {
    // First, search through existing schedules
    for (int i = 0; i < (int)scheduleTable.size(); i++) {
	//If schedule already exists
        if (scheduleTable[i].ID == ID) {
    	    //And SN is greater than ours, then update
    	    if (scheduleTable[i].SN < SN) {

    		//Calculate new frame offset for this schedule
    		simtime_t new_offset = simTime() - currentFrameStart + wakeup - frameTime;
    		trace() << "Resync successful for ID:"<<ID<<" old offset:"<<scheduleTable[i].offset<<" new offset:"<<new_offset;
    		scheduleTable[i].offset = new_offset;
    		scheduleTable[i].SN = SN;

    		if (i == 0) {
    		    //If the update came for primary schedule, then the next frame message has to be rescheduled
    		    setTimer(FRAME_START,wakeup);
    		    currentFrameStart += new_offset;
    		} else {
    		    //This is not primary schedule, check that offset value falls within the
    		    //interval: 0 < offset < frameTime
    		    if (scheduleTable[i].offset < 0) scheduleTable[i].offset += frameTime;
    		    if (scheduleTable[i].offset > frameTime) scheduleTable[i].offset -= frameTime;
    		}

	    } else if (scheduleTable[i].SN > SN) {
		/* TMAC received a sync with lower SN than what currently stored in the
		 * schedule table. With current TMAC implementation, this is not possible,
		 * however in future it may be neccesary to implement a unicast sync packet
		 * here to notify the source of this packet with the updated schedule
		 */
	    }

	    //found and updated the schedule, nothing else need to be done
	    return;
	}
    }

    //At this stage, the schedule was not found in the current table, so it has to be added
    TMacSchedule newSch;
    newSch.ID = ID;
    newSch.SN = SN;
    trace() << "Creating schedule ID:"<<ID<<", SN:"<<SN<<", wakeup:"<<wakeup;

    //Calculate the offset for the new schedule
    if (currentFrameStart == -1) {
        //If currentFrameStart is -1 then this schedule will be the new primary schedule
        //and it's offset will always be 0
	newSch.offset = 0;
    } else {
	//This schedule is not primary, it is necessary to calculate the offset from primary
	//schedule for this new schedule
	newSch.offset = simTime() - currentFrameStart + wakeup - frameTime;
    }

    //Add new schedule to the table
    scheduleTable.push_back(newSch);

    //If the new schedule is primary, more things need to be done:
    if (currentFrameStart == -1) {
	//This is new primary schedule, and since SYNC packet was received at this time, it is
	//safe to assume that nodes of this schedule are active and listening right now,
	//so active period can be safely extended
	currentFrameStart = activationTimeout = simTime();
	currentFrameStart += wakeup - frameTime;
	extendActivePeriod();

	//create and schedule the next frame message
	setTimer(FRAME_START,wakeup);

	//this flag indicates that this schedule has to be rebroadcasted
	needResync = 1;

	//MAC is reset to default state, allowing it to initiate and accept transmissions
	resetDefaultState();
    }
}

/* This function will handle a MAC frame received from the lower layer (physical or radio)
 */
void TMacModule::fromRadioLayer(MAC_GenericFrame *rcvFrame) {

    switch (rcvFrame->getHeader().frameType) {

	/* received a RTS frame */
	case MAC_PROTO_RTS_FRAME: {
	    //If this node is the destination, reply with a CTS, otherwise
	    //set a timeout and keep silent for the duration of communication
	    simtime_t NAV = rcvFrame->getHeader().NAV - rtsTxTime;
	    if (rcvFrame->getHeader().destID == self) {
		if (ctsFrame) cancelAndDelete(ctsFrame);
		ctsFrame = new MAC_GenericFrame("CTS message", MAC_FRAME);
		ctsFrame->setKind(MAC_FRAME) ;
		ctsFrame->getHeader().srcID = self;
		ctsFrame->getHeader().destID = rcvFrame->getHeader().srcID;
		ctsFrame->getHeader().NAV = NAV;
		ctsFrame->getHeader().frameType = MAC_PROTO_CTS_FRAME;
		ctsFrame->setByteLength(ctsFrameSize);
		performCarrierSense(MAC_CARRIER_SENSE_FOR_TX_CTS);
	    } else {
		if (collisionResolution != 2) setTimer(TRANSMISSION_TIMEOUT,NAV);
		extendActivePeriod(NAV);
		setMacState(MAC_STATE_ACTIVE_SILENT);
	    }
	    break;
	}

	/* received a CTS frame */
	case MAC_PROTO_CTS_FRAME: {
	    //If this CTS comes as a response to previously sent RTS, data transmission can be started
	    simtime_t NAV = rcvFrame->getHeader().NAV - ctsTxTime;
	    if(macState == MAC_STATE_WAIT_FOR_CTS && rcvFrame->getHeader().destID == self) {
		if(TXBuffer.empty()) {
		    trace() << "WARNING: invalid MAC_STATE_WAIT_FOR_CTS while buffer is empty";
		    resetDefaultState();
		} else if(rcvFrame->getHeader().srcID == txAddr) {
		    cancelTimer(TRANSMISSION_TIMEOUT);
		    performCarrierSense(MAC_CARRIER_SENSE_FOR_TX_DATA);
		} else {
		    trace() << "WARNING: recieved unexpected CTS from "<<rcvFrame->getHeader().srcID;
		    resetDefaultState();
		}

	    //If CTS is overheared from other transmission, keep silent
	    } else if (macState == MAC_CARRIER_SENSE_FOR_TX_RTS && useFRTS) {
	        //FRTS would need to be implemented here, for now just keep silent
	        if (collisionResolution != 2) setTimer(TRANSMISSION_TIMEOUT,NAV);
	        extendActivePeriod(NAV);
	        setMacState(MAC_STATE_ACTIVE_SILENT);
	    } else {
		if (collisionResolution != 2) setTimer(TRANSMISSION_TIMEOUT,NAV);
		extendActivePeriod(NAV);
		setMacState(MAC_STATE_ACTIVE_SILENT);
	    }

	    break;
	}

	/* received DATA frame */
	case MAC_PROTO_DATA_FRAME: {
	    int destAddr = rcvFrame->getHeader().destID;

	    // If this is not broadcast frame and destination is not this node, do nothing
	    if (destAddr != BROADCAST_ADDR && destAddr != self) break;

	    // Otherwise forward the frame to upper layer
	    toNetworkLayer(rcvFrame);

	    // If the frame was sent to broadcast address, nothing else needs to be done
	    if (destAddr == BROADCAST_ADDR) break;

	    // If MAC was expecting this frame, clear the timeout
	    if (macState == MAC_STATE_WAIT_FOR_DATA) cancelTimer(TRANSMISSION_TIMEOUT);

	    // Create and send an ACK frame (since this node is the destination for DATA frame)
	    if (ackFrame) cancelAndDelete(ackFrame);
	    ackFrame = new MAC_GenericFrame("ACK message", MAC_FRAME);
	    ackFrame->setKind(MAC_FRAME);
	    ackFrame->getHeader().srcID = self;
	    ackFrame->getHeader().destID = rcvFrame->getHeader().srcID;
	    ackFrame->getHeader().frameType = MAC_PROTO_ACK_FRAME;
	    ackFrame->setByteLength(ackFrameSize);
	    performCarrierSense(MAC_CARRIER_SENSE_FOR_TX_ACK);
	    break;
	}

	/* received ACK frame */
	case MAC_PROTO_ACK_FRAME: {
	    // If MAC was expecting this ack, then it is a successful transmission
	    // otherwise do nothing
	    if (macState == MAC_STATE_WAIT_FOR_ACK && rcvFrame->getHeader().destID == self) {
		if (rcvFrame->getHeader().srcID == txAddr) {
		    trace() << "Transmission succesful to " << txAddr;
		    cancelTimer(TRANSMISSION_TIMEOUT);
		    popTxBuffer();
		    resetDefaultState();
		} else {
		    trace() << "WARNING: recieved unexpected ACK from "<<rcvFrame->getHeader().srcID;
		}
	    }
	    break;
	}

	/* received SYNC frame */
	case MAC_PROTO_SYNC_FRAME: {
	    int destAddr = rcvFrame->getHeader().destID;
	    // In current implementation SYNC frames area always broadcast,
	    // however the destination address is checked anyway
	    if (destAddr != BROADCAST_ADDR && destAddr != self) break;

	    // Schedule table is updated with values from the SYNC frame
	    updateScheduleTable(rcvFrame->getHeader().SYNC, rcvFrame->getHeader().SYNC_ID, rcvFrame->getHeader().SYNC_SN);

	    // The state is reset to default allowing further transmissions in this frame
	    // (since SYNC frame does not intend to have an ACK or any other communications
	    // immediately after it)
	    resetDefaultState();
	    break;
	}

	default: {
	    trace() << "WARNING: unknown frame type received " << rcvFrame->getHeader().frameType;
	}
    }
}

/* This function handles carrier clear message, received from the radio module.
 * That is sent in a response to previous request to perform a carrier sense
 */
void TMacModule::carrierIsClear() {

    switch(macState) {

	/* MAC requested carrier sense to transmit an RTS packet */
	case MAC_CARRIER_SENSE_FOR_TX_RTS: {
	    if (TXBuffer.empty()) {
		trace() << "WARNING! BUFFER_IS_EMPTY in MAC_CARRIER_SENSE_FOR_TX_RTS, will reset state";
		resetDefaultState();
		break;
	    }

	    // create and send RTS frame
	    if (rtsFrame) cancelAndDelete(rtsFrame);
	    rtsFrame = new MAC_GenericFrame("RTS message", MAC_FRAME);
    	    rtsFrame->setKind(MAC_FRAME);
	    rtsFrame->getHeader().srcID = self;
	    rtsFrame->getHeader().destID = txAddr;
	    rtsFrame->getHeader().NAV = ctsTxTime + ackTxTime + TX_TIME(TXBuffer.front()->getByteLength() + macFrameOverhead) + NAV_EXTENSION;
 	    rtsFrame->getHeader().frameType = MAC_PROTO_RTS_FRAME;
	    rtsFrame->setByteLength(rtsFrameSize);
	    toRadioLayer(rtsFrame);
	    if (useRtsCts) txRetries--;
	    packetsSent["RTS"]++;
	    collectOutput("Sent packets breakdown","RTS");
	    rtsFrame = NULL;

	    // update MAC state
	    setMacState(MAC_STATE_WAIT_FOR_CTS);

	    // create a timeout for expecting a CTS reply
	    setTimer(TRANSMISSION_TIMEOUT,rtsTxTime + waitTimeout);
	    break;
	}

	/* MAC requested carrier sense to transmit a SYNC packet */
	case MAC_CARRIER_SENSE_FOR_TX_SYNC: {
	    // SYNC packet was created in scheduleSyncPacket function
	    if (syncFrame != NULL) {
		// Send SYNC packet to radio
		toRadioLayer(syncFrame);

		packetsSent["SYNC"]++;
		collectOutput("Sent packets breakdown","SYNC");
		syncFrame = NULL;

		// Clear the resync flag
		needResync = 0;

		// update MAC state
		setMacState(MAC_STATE_IN_TX);

		// create a timeout for this transmission - nothing is expected in reply
		// so MAC is only waiting for the RADIO to finish the packet transmission
		setTimer(TRANSMISSION_TIMEOUT,syncTxTime);

	    } else {
		trace() << "WARNING: Invalid MAC_CARRIER_SENSE_FOR_TX_SYNC while syncFrame undefined";
		resetDefaultState();
	    }
	    break;
	}

	/* MAC requested carrier sense to transmit a CTS packet */
	case MAC_CARRIER_SENSE_FOR_TX_CTS: {
	    // CTS packet was created when RTS was received
	    if (ctsFrame != NULL) {
		// Send CTS packet to radio
		toRadioLayer(ctsFrame);
		packetsSent["CTS"]++;
		collectOutput("Sent packets breakdown","CTS");
		ctsFrame = NULL;

		// update MAC state
		setMacState(MAC_STATE_WAIT_FOR_DATA);

		// create a timeout for expecting a DATA packet reply
		setTimer(TRANSMISSION_TIMEOUT,ctsTxTime + waitTimeout);

	    } else {
		trace() << "WARNING: Invalid MAC_CARRIER_SENSE_FOR_TX_CTS while ctsFrame undefined";
		resetDefaultState();
	    }
	    break;
	}

	/* MAC requested carrier sense to transmit DATA packet */
	case MAC_CARRIER_SENSE_FOR_TX_DATA: {
	    if (TXBuffer.empty()) {
		trace() << "WARNING: Invalid MAC_CARRIER_SENSE_FOR_TX_DATA while TX buffer is empty";
		resetDefaultState();
		break;
	    }

	    // Create a new MAC frame, encapsulate the first network data packet from MAC
	    // transmission buffer and send it to the radio
	    toRadioLayer(TXBuffer.front()->dup());
	    packetsSent["DATA"]++;
	    collectOutput("Sent packets breakdown","DATA");

	    //update MAC state based on transmission time and destination address
	    double txTime = TX_TIME(TXBuffer.front()->getByteLength());

	    if (txAddr == BROADCAST_ADDR) {
		// This packet is broadcast, so no reply will be received
		// The packet can be cleared from transmission buffer
		// and MAC timeout is only to allow RADIO to finish the transmission
		popTxBuffer();
		setMacState(MAC_STATE_IN_TX);
		setTimer(TRANSMISSION_TIMEOUT,txTime);
	    } else {
		// This packet is unicast, so MAC will be expecting an ACK
		// packet in reply, so the timeout is longer
		if (!useRtsCts) txRetries--;
	        setMacState(MAC_STATE_WAIT_FOR_ACK);
	        setTimer(TRANSMISSION_TIMEOUT,txTime + waitTimeout);
	    }

	    extendActivePeriod(txTime);

	    //update RADIO state
	    setRadioState(MAC_2_RADIO_ENTER_TX);
	    break;
	}

	/* MAC requested carrier sense to transmit ACK packet */
	case MAC_CARRIER_SENSE_FOR_TX_ACK: {
	    // ACK packet was created when MAC received a DATA packet.
	    if(ackFrame != NULL) {
		// Send ACK packet to the radio
		toRadioLayer(ackFrame);
		packetsSent["ACK"]++;
		collectOutput("Sent packets breakdown","ACK");
		ackFrame = NULL;

		// update MAC state
		setMacState(MAC_STATE_IN_TX);

		// create a timeout for this transmission - nothing is expected in reply
		// so MAC is only waiting for the RADIO to finish the packet transmission
		setTimer(TRANSMISSION_TIMEOUT,ackTxTime);
		extendActivePeriod(ackTxTime);
	    } else {
	    	trace() << "WARNING: Invalid MAC_STATE_WAIT_FOR_ACK while ackFrame undefined";
	    	resetDefaultState();
	    }
	    break;
	}

	/* MAC requested carrier sense before going to sleep */
	case MAC_CARRIER_SENSE_BEFORE_SLEEP: {
	    // primaryWakeup flag is cleared (in case the node will wake up in the current frame
	    // for a secondary schedule)
	    primaryWakeup = false;

	    // update MAC and RADIO states
	    setRadioState(MAC_2_RADIO_ENTER_SLEEP);
	    setMacState(MAC_STATE_SLEEP);
	    break;
	}
    }
}

/* This function will create a request to RADIO module to perform carrier sense.
 * MAC state is important when performing CS, so setMacState is always called here.
 * delay allows to perform a carrier sense after a choosen delay (useful for
 * randomisation of transmissions)
 */
void TMacModule::performCarrierSense(int newState, simtime_t delay) {
    setMacState(newState);
    setTimer(CARRIER_SENSE,delay);
}

/* This function will extend active period for MAC, ensuring that the remaining active
 * time it is not less than listenTimeout value. Also a check TA message is scheduled here
 * to allow the node to go to sleep if activation timeout expires
 */
void TMacModule::extendActivePeriod(simtime_t extra) {
    simtime_t curTime = simTime();
    if (conservativeTA) {
	curTime += extra;
	while (activationTimeout < curTime) { activationTimeout += listenTimeout; }
	if (curTime + listenTimeout < activationTimeout) return;
	activationTimeout += listenTimeout;
    } else if (activationTimeout < curTime + listenTimeout + extra) {
	activationTimeout = curTime + listenTimeout + extra;
    }
    setTimer(CHECK_TA,activationTimeout-curTime);
}

/* This function will check the transmission buffer, and if it is not empty, it will update
 * current communication parameters: txAddr and txRetries
 */
void TMacModule::checkTxBuffer() {
    if (TXBuffer.empty()) return;
    txAddr = TXBuffer.front()->getHeader().destID;
    txRetries = maxTxRetries;
}

/* This function will remove the first packet from MAC transmission buffer
 * checkTxBuffer is called in case there are still packets left in the buffer to transmit
 */
void TMacModule::popTxBuffer() {
    cancelAndDelete(TXBuffer.front());
    TXBuffer.pop();
    checkTxBuffer();
}
