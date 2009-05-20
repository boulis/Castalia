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

#define MAC_BUFFER_ARRAY_SIZE macBufferSize+1
#define BUFFER_IS_EMPTY  (headTxBuffer==tailTxBuffer)
#define BUFFER_IS_FULL  (getTXBufferSize() >= macBufferSize)
#define DRIFTED_TIME(time) ((time) * cpuClockDrift)
#define EMPTY_ADDR -2
#define BROADCAST_ADDR -1
#define EV   ev.disabled() ? (ostream&)ev : ev
#define CASTALIA_DEBUG_OLD (!printDebugInfo)?(ostream&)DebugInfoWriter::getStream():DebugInfoWriter::getStream()
#define BASIC_INFO "\n[MAC-"<<self<<"-"<<simTime()<<"] "
#define CASTALIA_DEBUG CASTALIA_DEBUG_OLD << BASIC_INFO

/************************************************************************************************
 *						TMAC						*
 ************************************************************************************************/

#define NAV_EXTENSION 		0.0001

#define FLENGTH(x)		(phyLayerOverhead + x)*1/(1000*radioDataRate/8.0)		//x are in BYTES

Define_Module(TMacModule);


void TMacModule::initialize() {
    readIniFileParameters();
	//--------------------------------------------------------------------------------
	//------- Follows code for the initialization of the class member variables ------

	self = parentModule()->parentModule()->index();

	//get a valid reference to the object of the Radio module so that we can make direct calls to its public methods
	//instead of using extra messages & message types for tighlty coupled operations.
	radioModule = check_and_cast<RadioModule*>(gate("toRadioModule")->toGate()->ownerModule());
	radioDataRate = (double) radioModule->par("dataRate");
	radioDelayForSleep2Listen = ((double) radioModule->par("delaySleep2Listen"))/1000.0;	
	radioDelayForValidCS = ((double) radioModule->par("delayCSValid"))/1000.0; //parameter given in ms in the omnetpp.ini

	phyLayerOverhead = radioModule->par("phyFrameOverhead");
	totalSimTime = ev.config()->getAsTime("General", "sim-time-limit");
	//get a valid reference to the object of the Resources Manager module so that we can make direct calls to its public methods
	//instead of using extra messages & message types for tighlty coupled operations.
	cModule *parentParent = parentModule()->parentModule();
	if(parentParent->findSubmodule("nodeResourceMgr") != -1) {
	    resMgrModule = check_and_cast<ResourceGenericManager*>(parentParent->submodule("nodeResourceMgr"));
	} else
	    opp_error("\n[MAC]:\n Error in geting a valid reference to  nodeResourceMgr for direct method calls.");
	cpuClockDrift = resMgrModule->getCPUClockDrift();

	schedTXBuffer = new MAC_GenericFrame*[MAC_BUFFER_ARRAY_SIZE];
	headTxBuffer = 0;
	tailTxBuffer = 0;
	macState = MAC_STATE_SETUP;
    
	maxSchedTXBufferSizeRecorded = 0;

	epsilon = 0.000001f; //senza f 1*10^(-6)  con 1*16^(-6)

	disabled = 1;		//TMAC is ready to send only before at least one schedule is choose

	//duty cycle references
	currentSleepMsg = NULL;
	primaryWakeupMsg = NULL;
	primaryWakeup = false;
	
	potentiallyDroppedDataFrames = 0;
	

	/**************************************************************************************************
	*			TMAC specific intialize
	**************************************************************************************************/

        syncTxTime = ((double)(syncFrameSize + macFrameOverhead + phyLayerOverhead)) * 8.0 / (1000.0 * radioDataRate);
        rtsTxTime = ((double)(rtsFrameSize + macFrameOverhead + phyLayerOverhead)) * 8.0 / (1000.0 * radioDataRate);
	ackTxTime = ((double)(ackFrameSize + macFrameOverhead + phyLayerOverhead)) * 8.0 / (1000.0 * radioDataRate);
	ctsTxTime = ((double)(ctsFrameSize + macFrameOverhead + phyLayerOverhead)) * 8.0 / (1000.0 * radioDataRate);
	needResync = 0;
	syncFrame = NULL;
	rtsFrame = NULL;
	ackFrame= NULL;
	ctsFrame = NULL;
	navTimeoutMsg = NULL;
	carrierSenseMsg = NULL;
	nextFrameMsg = NULL;
	waitForCtsTimeout = NULL;		//schedule msg wait for cts timeout
	waitForDataTimeout = NULL;		//schedule msg wait for data timeout
	waitForAckTimeout = NULL;		//schedule msg wait for ack timeout
	currentAdr = EMPTY_ADDR;
	currentFrameStart = -1;
}

void TMacModule::handleMessage(cMessage *msg) {
    int msgKind = msg->kind();
    if((disabled) && (msgKind != APP_NODE_STARTUP)) {
    	delete msg;
    	msg = NULL;		// safeguard
    	return;
    }

    switch (msgKind) {
	/*--------------------------------------------------------------------------------------------------------------
	 * Sent by the Application submodule in order to start/switch-on the MAC submodule.
	 *--------------------------------------------------------------------------------------------------------------*/
	case APP_NODE_STARTUP: {
	    disabled = 0;
	    setRadioState(MAC_2_RADIO_ENTER_LISTEN);
	    MAC_ControlMessage *csMsg = new MAC_ControlMessage("carrier sense strobe MAC->radio", MAC_2_RADIO_SENSE_CARRIER);
	    csMsg->setSense_carrier_interval(totalSimTime);
	    sendDelayed(csMsg,radioDelayForSleep2Listen+radioDelayForValidCS+epsilon ,"toRadioModule");
		
	    //initialise schedule event
	    if (isSink && allowSinkSync) {
		createPrimarySchedule();
	    } else {	
		scheduleAt(simTime() + (waitForSync ? DRIFTED_TIME(resyncTime*2) : 0),
		    new MAC_ControlMessage("waiting for a SYNC msg", MAC_SELF_SYNC_SETUP));
	    }
	    break;
	}
	
	/*--------------------------------------------------------------------------------------------------------------
	 * Sent by the Application submodule. We need to push it into the buffer and schedule its forwarding to the Radio buffer for TX.
	 *--------------------------------------------------------------------------------------------------------------*/
	case NETWORK_FRAME: {
	    if (BUFFER_IS_FULL) {
		send(new MAC_ControlMessage("MAC buffer is full Radio->Mac", MAC_2_NETWORK_FULL_BUFFER), "toNetworkModule");
		CASTALIA_DEBUG << "WARNING: SchedTxBuffer FULL! Application packet is dropped";
		break;
	    }
    	    
    	    //create the MACFrame from the Application Data Packet (encapsulation)
	    Network_GenericFrame *rcvNetDataFrame = check_and_cast<Network_GenericFrame*>(msg);
	    MAC_GenericFrame *dataFrame;
	    char buff[50];
	    sprintf(buff, "MAC Data frame (%f)", simTime());
	    dataFrame = new MAC_GenericFrame(buff, MAC_FRAME);

	    //try to encapsulate packet and add it to TX buffer
	    if(encapsulateNetworkFrame(rcvNetDataFrame, dataFrame)) {
	    	pushBuffer(dataFrame);
		CASTALIA_DEBUG << "Application packet successfuly buffered";
	    } else {
		cancelAndDelete(dataFrame);
		dataFrame = NULL;
		CASTALIA_DEBUG << "WARNING: Application packet is dropped (oversized packet)";
    	    }
	}
	
	case RADIO_2_MAC_STARTED_TX: { break; }
	case RADIO_2_MAC_STOPPED_TX: { break; }

	/*--------------------------------------------------------------------------------------------------------------
	 * Control message sent by the MAC (ourself) to try to set the state of the Radio to RADIO_STATE_SLEEP.
	 *--------------------------------------------------------------------------------------------------------------*/
	case MAC_SELF_SET_RADIO_SLEEP: {
	    primaryWakeup = false;
    	    if (macState == MAC_STATE_ACTIVE || macState == MAC_STATE_PASSIVE) {
		setRadioState(MAC_2_RADIO_ENTER_SLEEP);
		changeState(MAC_STATE_SLEEP);
	    }
	    break;

	case MAC_SELF_FRAME_START:
	    primaryWakeup = true;
	    currentFrameStart = simTime();
	    nextFrameMsg = new MAC_ControlMessage("Frame started", MAC_SELF_FRAME_START);
	    scheduleAt(simTime() + DRIFTED_TIME(frameTime), nextFrameMsg);
	    for (int i = 1; i < scheduleTable.size(); i++) {
		scheduleAt(simTime() + DRIFTED_TIME(scheduleTable[i].offset),
		    new MAC_ControlMessage("Secondary schedule wakeup", MAC_SELF_WAKEUP_PASSIVE));
	    }
	    if (macState == MAC_STATE_SLEEP) {
	    	setRadioState(MAC_2_RADIO_ENTER_LISTEN);
	    	resetDefaultState();
	    }
	    break;
	}
	
	/*--------------------------------------------------------------------------------------------------------------
	 * Control message sent by the MAC (ourself) to try to wake up the radio.
	 *--------------------------------------------------------------------------------------------------------------*/
	case MAC_SELF_WAKEUP_PASSIVE: {
	    if(macState == MAC_STATE_SLEEP) {
		setRadioState(MAC_2_RADIO_ENTER_LISTEN);
		resetDefaultState();
	    }
	}
		
	/*--------------------------------------------------------------------------------------------------------------
	 * Control message sent by the MAC (ourself) to initiate a TX  
	 *--------------------------------------------------------------------------------------------------------------*/
	case MAC_SELF_INITIATE_TX: {
	    // can only initiate a transmission when in active state and buffer is not empty
	    if (macState != MAC_STATE_ACTIVE || (BUFFER_IS_EMPTY && !needResync)) break;
			
	    //generate randomContendOffset for this transmission
	    randomContendOffset = genk_dblrand(1)*contentionPeriod;
		    
	    if (needResync) {
		scheduleSyncFrame(randomContendOffset);
		break;
	    } 
	    changeState(useRtsCts ? MAC_CARRIER_SENSE_FOR_TX_RTS : MAC_CARRIER_SENSE_FOR_TX_DATA);
	    scheduleAt(simTime()+ DRIFTED_TIME(randomContendOffset), 
	        new MAC_ControlMessage("Enter CARRIER SENSE after waiting", MAC_SELF_PERFORM_CARRIER_SENSE));
	    break;
	}
	
	/*--------------------------------------------------------------------------------------------------------------
	 * Data Frame Received from the Radio submodule 
	 *--------------------------------------------------------------------------------------------------------------*/
	case MAC_FRAME: {
	    MAC_GenericFrame *rcvFrame;
	    rcvFrame = check_and_cast<MAC_GenericFrame*>(msg);
	    processMacFrame(rcvFrame);
	    break;
	}
	
	/*--------------------------------------------------------------------------------------------------------------
	 * Sent by the MAC submodule itself to start carrier sensing
	 *--------------------------------------------------------------------------------------------------------------*/
	case MAC_SELF_PERFORM_CARRIER_SENSE: {
	    //check for valid MAC state
	    if(macState == MAC_STATE_SILENT) {
		break;	
	    } else if(macState != MAC_CARRIER_SENSE_FOR_TX_RTS && macState != MAC_CARRIER_SENSE_FOR_TX_CTS && macState != MAC_CARRIER_SENSE_FOR_TX_SYNC) {
		CASTALIA_DEBUG << "WARNING: bad MAC state for MAC_SELF_PERFORM_CARRIER_SENSE";
		break;
	    }
		    
	    //check for validiry of carrier sense
	    int isCarrierSenseValid_ReturnCode = radioModule->isCarrierSenseValid();
	    if(isCarrierSenseValid_ReturnCode == 1) {
	        //carrier sense indication of Radio is Valid
	        // send the delayed message with the command to perform Carrier Sense to the radio
	        send(new MAC_ControlMessage("carrier sense strobe MAC->radio", MAC_2_RADIO_SENSE_CARRIER_INSTANTANEOUS), "toRadioModule");
	    } else {
	        //carrier sense indication of Radio is NOT Valid and isCarrierSenseValid_ReturnCode holds the cause for the non valid carrier sense indication
    		switch(isCarrierSenseValid_ReturnCode) {
		    case RADIO_IN_TX_MODE: break; 
		    case RADIO_SLEEPING: break;
		    case RADIO_NON_READY: 
		        scheduleAt(simTime() + DRIFTED_TIME(radioDelayForValidCS), new MAC_ControlMessage("Enter carrier sense state MAC->MAC", MAC_SELF_PERFORM_CARRIER_SENSE));
		        break;
		    default: 
		        CASTALIA_DEBUG << "Carrier sense indication of Radio is not valid: "<<isCarrierSenseValid_ReturnCode;
		        resetDefaultState();
		        break;
		}
	    }
	    break;
	}
	
	/*--------------------------------------------------------------------------------------------------------------
	 * Sent by the MAC submodule (ourself) to exit the MAC_STATE_CARRIER_SENSING (carrier is free)
	 *--------------------------------------------------------------------------------------------------------------*/
	case RADIO_2_MAC_NO_SENSED_CARRIER: {
	    carrierIsClear();
	    break;
	}

	/*--------------------------------------------------------------------------------------------------------------
	 * Sent by the Radio submodule as a response for our (MAC's) previously sent directive to perform CS (carrier is not clear)
	 *--------------------------------------------------------------------------------------------------------------*/
	case RADIO_2_MAC_SENSED_CARRIER: {
	    //need to prolong listening time 
	    if (currentSleepMsg != NULL && currentSleepMsg->isScheduled()) 
	        cancelAndDelete(currentSleepMsg);
	    currentSleepMsg =  new MAC_ControlMessage("put_radio_to_sleep", MAC_SELF_SET_RADIO_SLEEP);		
	    scheduleAt(simTime() + DRIFTED_TIME(listenTimeout), currentSleepMsg);
	    break;
	}

	case RADIO_2_MAC_FULL_BUFFER: {
	    /* TODO:
	     * add your code here to manage the situation of a full Radio buffer.
	     * Apparently we 'll have to stop sending messages and enter into listen or sleep mode (depending on the MAC protocol that we implement).
	     */
	    CASTALIA_DEBUG << "Mac module received RADIO_2_MAC_FULL_BUFFER because the Radio buffer is full.";
	    break;
	}

	case RESOURCE_MGR_OUT_OF_ENERGY: {
	    disabled = 1;
	    break;
	}	
		
	/*
	 *	These messages are handling initial schedule creation and further resyncs
	 */
		 
	case MAC_SELF_SYNC_SETUP: {
	    if (macState == MAC_STATE_SETUP) {
		scheduleAt(simTime() + DRIFTED_TIME(genk_dblrand(1)*frameTime), 
		    new MAC_ControlMessage("waiting for a SYNC msg", MAC_SELF_SYNC_CREATE));
	    }
	    break;		    
	}
	
	case MAC_SELF_SYNC_CREATE: {
	    if (macState == MAC_STATE_SETUP) createPrimarySchedule();
	    break;
	}
	
	case MAC_SELF_SYNC_RENEW: {
	    needResync = 1;
	    scheduleAt(simTime() + DRIFTED_TIME(resyncTime),
	        new MAC_ControlMessage("initiate resync procedure", MAC_SELF_SYNC_RENEW));    
	    break;
	}
	
	/*
	 *	These messages are timeouts
	 */
		
	case MAC_TIMER_NAV: {
	    navTimeoutMsg = NULL;
	    resetDefaultState();
	    break;
	}
	
	case MAC_TIMER_WFCTS: {
	    if (getTXBufferSize() > 1) pushBuffer(popTxBuffer());
	    resetDefaultState();
	    if(msg == waitForCtsTimeout) waitForCtsTimeout = NULL;
	    break;
	}
	
	case MAC_TIMER_WFDATA: {
	    resetDefaultState();
	    if(msg == waitForDataTimeout) waitForDataTimeout = NULL;
	    break;
	}
	
	case MAC_TIMER_WFACK: {
	    if (getTXBufferSize() > 1) pushBuffer(popTxBuffer());
	    resetDefaultState();
	    if(msg == waitForAckTimeout) waitForAckTimeout = NULL;
	    break;
	}
	
	default: {
	    CASTALIA_DEBUG << "WARNING: received packet of unknown type";
	    break;
	}

    }//end_of_switch

    delete msg;
    msg = NULL;		// safeguard		
}


void TMacModule::finish() {
    MAC_GenericFrame *macMsg;
    while(!BUFFER_IS_EMPTY) {
	macMsg = popTxBuffer();
	cancelAndDelete(macMsg);
  	macMsg = NULL;
    }
    delete[] schedTXBuffer;
	
    if(printPotentiallyDropped)
    	CASTALIA_DEBUG <<"Dropped Packets at MAC module while receiving:\t " <<  potentiallyDroppedDataFrames << " data frames";
	//recordScalar("Max Mac-buffer size recorded", maxSchedTXBufferSizeRecorded);
    char nodo[100];
    sprintf(nodo,"[MAC] Node %d has sent SYNC messages # %d and received SYNC #",self,syncSN);
    recordScalar(nodo, syncReceived);
}

void TMacModule::readIniFileParameters(void) {
    printPotentiallyDropped = par("printPotentiallyDroppedPacketsStatistics");
    printDebugInfo = par("printDebugInfo");
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
    contentionPeriod = ((double)par("contentionPeriod"))/1000.0;		// just convert msecs to secs 
    listenTimeout = ((double)par("listenTimeout"))/1000.0;		// TA: just convert msecs to secs (15ms default);
    useFRTS = par("useFrts");
    useRtsCts = par("useRtsCts");
}


void TMacModule::setRadioState(MAC_ControlMessageType typeID, double delay) {
    if( (typeID != MAC_2_RADIO_ENTER_SLEEP) && (typeID != MAC_2_RADIO_ENTER_LISTEN) && (typeID != MAC_2_RADIO_ENTER_TX) )
	opp_error("MAC attempt to set Radio into an unknown state. ERROR commandID");
    MAC_ControlMessage * ctrlMsg = new MAC_ControlMessage("state command strobe MAC->radio", typeID);
    sendDelayed(ctrlMsg, delay, "toRadioModule");
}


void TMacModule::setRadioTxMode(Radio_TxMode txTypeID, double delay) {
    if( (txTypeID != CARRIER_SENSE_NONE)&&(txTypeID != CARRIER_SENSE_ONCE_CHECK)&&(txTypeID != CARRIER_SENSE_PERSISTENT) )
	opp_error("MAC attempt to set Radio CarrierSense into an unknown type. ERROR commandID");

    MAC_ControlMessage * ctrlMsg = new MAC_ControlMessage("Change Radio TX mode command strobe MAC->radio", MAC_2_RADIO_CHANGE_TX_MODE);
    ctrlMsg->setRadioTxMode(txTypeID);
    sendDelayed(ctrlMsg, delay, "toRadioModule");
}

void TMacModule::setRadioPowerLevel(int powLevel, double delay) {
    if( (powLevel >= 0) && (powLevel < radioModule->getTotalTxPowerLevels()) ) {
	MAC_ControlMessage * ctrlMsg = new MAC_ControlMessage("Set power level command strobe MAC->radio", MAC_2_RADIO_CHANGE_POWER_LEVEL);
	ctrlMsg->setPowerLevel(powLevel);
	sendDelayed(ctrlMsg, delay, "toRadioModule");
    } else
	CASTALIA_DEBUG << "WARNING: in function setRadioPowerLevel() of Mac module, parameter powLevel has invalid value.\n";
}

void TMacModule::resetDefaultState()  {
    changeState(primaryWakeup ? MAC_STATE_ACTIVE : MAC_STATE_PASSIVE);
    //delete previous sleep message (if any)
    if (currentSleepMsg != NULL && currentSleepMsg->isScheduled()) {
	cancelAndDelete(currentSleepMsg);
	currentSleepMsg = NULL;
    }

    if (primaryWakeup) {
	//if we have something to send (RTS or SYNC), 
	//then create a message to initiate transimission
	//otherwise create a new sleep message if it makes sense to go to sleep
	if (needResync) {
	    scheduleSyncFrame(genk_dblrand(1)*contentionPeriod);
	    return;
	} else if (!BUFFER_IS_EMPTY) {
	    changeState(useRtsCts ? MAC_CARRIER_SENSE_FOR_TX_RTS : MAC_CARRIER_SENSE_FOR_TX_DATA);
	    if (carrierSenseMsg != NULL && carrierSenseMsg->isScheduled()) cancelAndDelete(carrierSenseMsg);
	    carrierSenseMsg = new MAC_ControlMessage("Enter CARRIER SENSE after waiting", MAC_SELF_PERFORM_CARRIER_SENSE);
	    scheduleAt(simTime() + epsilon + DRIFTED_TIME(genk_dblrand(1)*contentionPeriod), carrierSenseMsg);
	    return;
	} 
    }
    
    if (simTime() + DRIFTED_TIME(listenTimeout) < nextWakeup()) {
	currentSleepMsg =  new MAC_ControlMessage("put_radio_to_sleep", MAC_SELF_SET_RADIO_SLEEP);		
	scheduleAt(simTime() + DRIFTED_TIME(listenTimeout), currentSleepMsg);
    } 
}

int TMacModule::pushBuffer(MAC_GenericFrame *theFrame) {
    if(theFrame == NULL) {
	CASTALIA_DEBUG <<"WARNING: Trying to push NULL MAC_GenericFrame to the MAC_Buffer!";
	return 0;
    }

    tailTxBuffer = (++tailTxBuffer)%(MAC_BUFFER_ARRAY_SIZE); //increment the tailTxBuffer pointer. If reached the end of array, then start from position [0] of the array
    if (tailTxBuffer == headTxBuffer) {
	// reset tail pointer
	if(tailTxBuffer == 0) tailTxBuffer = MAC_BUFFER_ARRAY_SIZE-1;
	else tailTxBuffer--;
	CASTALIA_DEBUG << "WARNING: SchedTxBuffer FULL!!! value to be Tx not added to buffer";
	return 0;
    }
    
    theFrame->setKind(MAC_FRAME);
    if (tailTxBuffer==0) schedTXBuffer[MAC_BUFFER_ARRAY_SIZE-1] = theFrame;
    else schedTXBuffer[tailTxBuffer-1] = theFrame;

    int currLen = getTXBufferSize();
    if (currLen > maxSchedTXBufferSizeRecorded)
    	maxSchedTXBufferSizeRecorded = currLen;

    return 1;
}

MAC_GenericFrame* TMacModule::popTxBuffer() {
    if (tailTxBuffer == headTxBuffer) {
        ev << "\nTrying to pop EMPTY TxBuffer!!";
        tailTxBuffer--;  // reset tail pointer
        return NULL;
    }

    MAC_GenericFrame *pop_message = schedTXBuffer[headTxBuffer];
    headTxBuffer = (++headTxBuffer)%(MAC_BUFFER_ARRAY_SIZE);
    return pop_message;
}

MAC_GenericFrame* TMacModule::peekTxBuffer() {
    if (tailTxBuffer == headTxBuffer) {
	ev << "\nTrying to peek EMPTY TxBuffer!";
	return NULL;
    }
    
    MAC_GenericFrame *peek_message = schedTXBuffer[headTxBuffer];
    return peek_message;
}

int TMacModule::getTXBufferSize(void) {
    int size = tailTxBuffer - headTxBuffer;
    if ( size < 0 ) size += MAC_BUFFER_ARRAY_SIZE;
    return size;
}

void TMacModule::updateNav(double t) {
    if(navTimeoutMsg == NULL || simTime() + t > navTimeoutMsg->arrivalTime()) { 
	if (navTimeoutMsg) cancelAndDelete(navTimeoutMsg);
	navTimeoutMsg = new MAC_ControlMessage("NAV timeout expired", MAC_TIMER_NAV);
        scheduleAt(simTime() + DRIFTED_TIME(t), navTimeoutMsg);
    }
}

int TMacModule::encapsulateNetworkFrame(Network_GenericFrame *networkFrame, MAC_GenericFrame *retFrame) {
    int totalMsgLen = networkFrame->byteLength() + macFrameOverhead;
    if(totalMsgLen > maxMACFrameSize) return 0;
    retFrame->setByteLength(macFrameOverhead); //networkFrame->byteLength() extra bytes will be added after the encapsulation
    retFrame->getHeader().srcID = self;
    retFrame->getHeader().destID = deliver2NextHop(networkFrame->getHeader().nextHop.c_str());
    retFrame->getHeader().frameType = MAC_PROTO_DATA_FRAME;
    Network_GenericFrame *dupNetworkFrame = check_and_cast<Network_GenericFrame *>(networkFrame->dup());
    retFrame->encapsulate(dupNetworkFrame);
    return 1;
}

// Resolve Network-layer IDs to MAC-layer IDs
int TMacModule::deliver2NextHop(const char *nextHop) {
    string strNextHop(nextHop);	
    return (strNextHop.compare(BROADCAST) == 0 ? BROADCAST_ADDR : atoi(strNextHop.c_str()));
}

void TMacModule::createPrimarySchedule() {
    updateScheduleTable(frameTime,self,0);
    scheduleAt(simTime() + DRIFTED_TIME(resyncTime),
	new MAC_ControlMessage("initiate resync procedure", MAC_SELF_SYNC_RENEW));
}

void TMacModule::changeState(int newState) {
    if (macState == newState) return;
    if (printStateTransitions) {
        CASTALIA_DEBUG << "state changed to "<<newState;
    }
    macState = newState;
}

void TMacModule::scheduleSyncFrame(double delay) {
    if (scheduleTable.size() < 1) {
	CASTALIA_DEBUG << "WARNING: unable to schedule Sync frame without a schedule!";
	return;
    }
    TMacSchedule sch = scheduleTable[0];
    if (syncFrame) delete syncFrame; 
    double nextFrameStart = currentFrameStart + frameTime;
    syncFrame = new MAC_GenericFrame("SYNC message", MAC_FRAME);
    syncFrame->setKind(MAC_FRAME) ;
    syncFrame->getHeader().srcID = self;
    syncFrame->getHeader().destID = BROADCAST_ADDR;
    syncFrame->getHeader().SYNC = nextFrameStart > delay ? 
	nextFrameStart - delay : frameTime + nextFrameStart - delay;
    syncFrame->getHeader().SYNC_ID = sch.ID;
    syncFrame->getHeader().SYNC_SN = sch.SN;
    syncFrame->getHeader().frameType = MAC_PROTO_SYNC_FRAME;
    syncFrame->setByteLength(syncFrameSize);
    changeState(MAC_CARRIER_SENSE_FOR_TX_SYNC);
    scheduleAt(simTime()+ DRIFTED_TIME(delay), 
	new MAC_ControlMessage("Enter CARRIER SENSE after waiting", MAC_SELF_PERFORM_CARRIER_SENSE));
}

void TMacModule::updateScheduleTable(double wakeup, int ID, int SN) {
    for (int i = 0; i < scheduleTable.size(); i++) {
        TMacSchedule sch = scheduleTable[i];
        if (sch.ID == ID) {
    	    if (sch.SN < SN) {	//update
    		double new_offset = simTime() - currentFrameStart + wakeup;
    		if (new_offset > frameTime) new_offset -= frameTime;
    		CASTALIA_DEBUG << "Resync successful for ID:"<<ID<<" old offset:"<<sch.offset<<" new offset:"<<new_offset;
    		sch.offset = new_offset;
    		sch.SN = SN;
    		if (i == 0) needResync = 1;
	    } else {
		//+++ need to send non-broadcast sync...
	    }
	    return;
	}
    }

    TMacSchedule newSch;
    if (currentFrameStart == -1) {
	currentFrameStart = simTime();
	newSch.offset = 0;
	nextFrameMsg = new MAC_ControlMessage("First frame start", MAC_SELF_FRAME_START);
	scheduleAt(currentFrameStart + DRIFTED_TIME(frameTime), nextFrameMsg);
    } else {
	newSch.offset = simTime() - currentFrameStart + wakeup;
	if (newSch.offset > frameTime) newSch.offset -= frameTime;
    }
    newSch.ID = ID;
    newSch.SN = SN;
    scheduleTable.push_back(newSch);
    if (scheduleTable.size() == 1) {
	needResync = 1;
	resetDefaultState();
    }
}

double TMacModule::nextWakeup() {
    if (currentFrameStart == -1) {
	CASTALIA_DEBUG << "Warning nextWakeup called without currentFrameStart";
	return -1;
    }
    double result = frameTime;
    double currentOffset = simTime() - currentFrameStart;
    for (int i = 0; i < scheduleTable.size(); i++) {
	TMacSchedule sch = scheduleTable[i];
	double when = sch.offset - currentOffset;
	if (when < 0) when += frameTime;
	if (when < result) result = when;
    }
    return result;
}

void TMacModule::processMacFrame(MAC_GenericFrame *rcvFrame) {

    switch (rcvFrame->getHeader().frameType) {
    	
	case MAC_PROTO_RTS_FRAME: {
	    if (macState == MAC_STATE_SILENT) break;
	    if (rcvFrame->getHeader().destID == self) {
		changeState(MAC_CARRIER_SENSE_FOR_TX_CTS);
		ctsFrame = new MAC_GenericFrame("CTS message", MAC_FRAME);
		ctsFrame->setKind(MAC_FRAME) ;
		ctsFrame->getHeader().srcID = self;
		ctsFrame->getHeader().destID = rcvFrame->getHeader().srcID;
		ctsFrame->getHeader().NAV = rcvFrame->getHeader().NAV - rtsTxTime;
		ctsFrame->getHeader().frameType = MAC_PROTO_CTS_FRAME;
		ctsFrame->setByteLength(ctsFrameSize);
		scheduleAt(simTime(), new MAC_ControlMessage("Enter CARRIER SENSE", MAC_SELF_PERFORM_CARRIER_SENSE));
	    } else {
		updateNav(rcvFrame->getHeader().NAV);
		changeState(MAC_STATE_SILENT);
	    }
	    break;
	}
	
	case MAC_PROTO_CTS_FRAME: {
	    if(macState == MAC_STATE_WAIT_FOR_CTS && rcvFrame->getHeader().destID == self) {
		if(BUFFER_IS_EMPTY) {
		    CASTALIA_DEBUG << "WARNING: invalid MAC_STATE_WAIT_FOR_CTS while buffer is empty";
		    resetDefaultState();
		    break;
		}
		MAC_GenericFrame *dataFrame = peekTxBuffer();
		if(rcvFrame->getHeader().srcID == currentAdr) { 
		    if(waitForCtsTimeout != NULL && waitForCtsTimeout->isScheduled()) {
			cancelAndDelete(waitForCtsTimeout);
			waitForCtsTimeout = NULL;
	            }
	            changeState(MAC_CARRIER_SENSE_FOR_TX_DATA);
	            scheduleAt(simTime(), new MAC_ControlMessage("Enter CARRIER SENSE", MAC_SELF_PERFORM_CARRIER_SENSE));
		} else {
		    CASTALIA_DEBUG << "WARNING: recieved unexpected CTS from "<<rcvFrame->getHeader().srcID;
		}
		break;
	    } else if (macState == MAC_CARRIER_SENSE_FOR_TX_RTS) {
		CASTALIA_DEBUG << "FRTS required here";
	        //+++ implement FRTS
	    }
	    //i'm overhearing CTS, need to update NAV
	    updateNav(rcvFrame->getHeader().NAV);
	    changeState(MAC_STATE_SILENT);
	    break;
	}
	
	case MAC_PROTO_DATA_FRAME: {
	    int destAddr = rcvFrame->getHeader().destID;
	    if (destAddr != BROADCAST_ADDR && destAddr != self) break;
	    Network_GenericFrame *netDataFrame;
	    netDataFrame = check_and_cast<Network_GenericFrame *>(rcvFrame->decapsulate());
	    netDataFrame->setRssi(rcvFrame->getRssi());
	    send(netDataFrame, "toNetworkModule");
	    if (destAddr == BROADCAST_ADDR) break;
	    if (!useRtsCts || macState == MAC_STATE_WAIT_FOR_DATA) {
	        if (waitForDataTimeout != NULL && waitForDataTimeout->isScheduled()) {
	    	    cancelAndDelete(waitForDataTimeout);
		    waitForDataTimeout = NULL;
		}
		ackFrame = new MAC_GenericFrame("ACK message", MAC_FRAME);  
		ackFrame->setKind(MAC_FRAME) ;
		ackFrame->getHeader().srcID = self;
		ackFrame->getHeader().destID = rcvFrame->getHeader().srcID;
		ackFrame->getHeader().frameType = MAC_PROTO_ACK_FRAME;
		ackFrame->setByteLength(ackFrameSize);
		changeState(MAC_CARRIER_SENSE_FOR_TX_ACK);
		scheduleAt(simTime(), new MAC_ControlMessage("Enter CARRIER SENSE", MAC_SELF_PERFORM_CARRIER_SENSE));
	    } else {
	        CASTALIA_DEBUG << "WARNING: received DATA frame not in MAC_STATE_WAIT_FOR_DATA";
	    }
	    break;
	}
	
	case MAC_PROTO_ACK_FRAME: {
	    if (macState == MAC_STATE_WAIT_FOR_ACK && rcvFrame->getHeader().destID == self) {
		MAC_GenericFrame *dataFrame = peekTxBuffer();
		if (rcvFrame->getHeader().srcID == dataFrame->getHeader().destID) {
		    if(waitForAckTimeout != NULL && waitForAckTimeout->isScheduled()) {
		        cancelAndDelete(waitForAckTimeout);
		        waitForAckTimeout = NULL;
	    	    }
		    popTxBuffer();
		    CASTALIA_DEBUG << "Transmission succesful to " << dataFrame->getHeader().destID;
		    resetDefaultState();
		} else {
		    CASTALIA_DEBUG << "WARNING: recieved unexpected ACK from "<<rcvFrame->getHeader().srcID;
		}
	    }
	    break;
	}
	
	case MAC_PROTO_SYNC_FRAME: {
	    int destAddr = rcvFrame->getHeader().destID;
	    if (destAddr != BROADCAST_ADDR && destAddr != self) break;
	    updateScheduleTable(rcvFrame->getHeader().SYNC, rcvFrame->getHeader().SYNC_ID, rcvFrame->getHeader().SYNC_SN);
	    break;				
	}
	
	default: {
	    CASTALIA_DEBUG << "WARNING: unknown frame type received " << rcvFrame->getHeader().frameType;
	}
    }	
}

void TMacModule::carrierIsClear() {
    switch(macState) {
	case MAC_CARRIER_SENSE_FOR_TX_RTS: {
	    if (BUFFER_IS_EMPTY) {
		CASTALIA_DEBUG << "WARNING! BUFFER_IS_EMPTY in MAC_CARRIER_SENSE_FOR_TX_RTS, will reset state";
		resetDefaultState();
		break;
	    }	
	    MAC_GenericFrame *dataFrame = peekTxBuffer();
	    rtsFrame = new MAC_GenericFrame("RTS message", MAC_FRAME);
    	    rtsFrame->setKind(MAC_FRAME) ;
	    rtsFrame->getHeader().srcID = self;
	    rtsFrame->getHeader().destID = dataFrame->getHeader().destID;
	    rtsFrame->getHeader().NAV = ctsTxTime + NAV_EXTENSION; //+++
 	    rtsFrame->getHeader().frameType = MAC_PROTO_RTS_FRAME;
	    rtsFrame->setByteLength(rtsFrameSize);
	    send(rtsFrame, "toRadioModule");
	    setRadioState(MAC_2_RADIO_ENTER_TX);
	    changeState(MAC_STATE_WAIT_FOR_CTS);
	    waitForCtsTimeout = new MAC_ControlMessage("Wait for CTS (timeout expired)", MAC_TIMER_WFCTS);
		scheduleAt(simTime() + DRIFTED_TIME(ctsTxTime + NAV_EXTENSION), waitForCtsTimeout);
	    rtsSN++;
	    rtsFrame = NULL;
	    break;
	}
	
	case MAC_CARRIER_SENSE_FOR_TX_SYNC: {
	    if (syncFrame != NULL) {
		send(syncFrame, "toRadioModule");
		setRadioState(MAC_2_RADIO_ENTER_TX);
		if (syncID == self) syncSN++; 
		syncFrame = NULL;
		needResync = 0;
	    } else {
		CASTALIA_DEBUG << "WARNING: Invalid MAC_CARRIER_SENSE_FOR_TX_SYNC while syncFrame undefined";
	    } 
	    resetDefaultState();
	    break;
	}
	
	case MAC_CARRIER_SENSE_FOR_TX_CTS: {
	    if (ctsFrame != NULL) {
		changeState(MAC_STATE_WAIT_FOR_DATA);
		waitForDataTimeout = new MAC_ControlMessage("Wait for DATA (timeout expired)", MAC_TIMER_WFDATA);
		    scheduleAt(simTime() + DRIFTED_TIME(ctsFrame->getHeader().NAV), waitForDataTimeout);
		send(ctsFrame, "toRadioModule");
		setRadioState(MAC_2_RADIO_ENTER_TX);
		ctsFrame = NULL;
	    } else {
		CASTALIA_DEBUG << "WARNING: Invalid MAC_CARRIER_SENSE_FOR_TX_CTS while ctsFrame undefined";
		resetDefaultState();
	    } 
	    break;
	}
					
	case MAC_CARRIER_SENSE_FOR_TX_DATA: {
	    if (BUFFER_IS_EMPTY) {
		CASTALIA_DEBUG << "WARNING: Invalid MAC_CARRIER_SENSE_FOR_TX_DATA while TX buffer is empty";
		resetDefaultState();
		break;
	    }
	    changeState(MAC_STATE_WAIT_FOR_ACK);
	    waitForAckTimeout = new MAC_ControlMessage("Wait for ACK (timeout expired)", MAC_TIMER_WFACK);
	    scheduleAt(simTime() + DRIFTED_TIME(FLENGTH(peekTxBuffer()->getHeader().NAV)), waitForAckTimeout);
	    send(peekTxBuffer(),"toRadioModule"); 
	    setRadioState(MAC_2_RADIO_ENTER_TX, epsilon);
	    break;
	}

	case MAC_CARRIER_SENSE_FOR_TX_ACK: {
	    if(ackFrame != NULL) {
		send(ackFrame, "toRadioModule");
		setRadioState(MAC_2_RADIO_ENTER_TX);
		ackFrame = NULL;
	    } else {
	    	CASTALIA_DEBUG << "WARNING: Invalid MAC_STATE_WAIT_FOR_ACK while ackFrame undefined";
	    }
	    resetDefaultState();
	    break;
	}
    }
}
