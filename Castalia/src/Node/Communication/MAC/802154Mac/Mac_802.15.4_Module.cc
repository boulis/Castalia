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
#include "Mac_802.15.4_Module.h"

#define DRIFTED_TIME(time) ((time) * cpuClockDrift)
#define BROADCAST_ADDR -1
#define EV   ev.disabled() ? (ostream&)ev : ev
#define CASTALIA_DEBUG_OLD (!printDebugInfo)?(ostream&)DebugInfoWriter::getStream():DebugInfoWriter::getStream()
#define BASIC_INFO "\n[MAC-"<<self<<"-"<<simTime()<<"] "
#define CASTALIA_DEBUG CASTALIA_DEBUG_OLD << BASIC_INFO

#define ACK_PKT_SIZE 6
#define BASE_BEACON_PKT_SIZE 12
#define BITS_PER_SYMBOL 8

#define BEACON_GUARD 0.001
#define PROCESSING_DELAY 0.00001

#define TX_TIME(x)		(phyLayerOverhead + x)*1/(1000*radioDataRate/8.0)		//x are in BYTES

Define_Module(Mac802154Module);

void Mac802154Module::initialize() {
	readIniFileParameters();

	//Initialise state descriptions	used in debug output
	
	if (printStateTransitions) {
	    stateDescr[1000] = "MAC_STATE_SETUP";
	    stateDescr[1001] = "MAC_STATE_SLEEP";
	    stateDescr[1002] = "MAC_STATE_IDLE";
	    stateDescr[1003] = "MAC_STATE_CSMA_CA";
	    stateDescr[1004] = "MAC_STATE_CCA";
	    stateDescr[1005] = "MAC_STATE_IN_TX";
	    stateDescr[1006] = "MAC_STATE_WAIT_FOR_ASSOCIATE_RESPONSE";
	    stateDescr[1007] = "MAC_STATE_WAIT_FOR_DATA_ACK";
	    stateDescr[1008] = "MAC_STATE_WAIT_FOR_BEACON";
	}
	
	//initialization of the class member variables

	self = parentModule()->parentModule()->index();

	//get a valid reference to the object of the Radio module so that we can make direct calls to its public methods
	//instead of using extra messages & message types for tighlty coupled operations.
	radioModule = check_and_cast<RadioModule*>(gate("toRadioModule")->toGate()->ownerModule());
	radioDataRate = (double) radioModule->par("dataRate");
	radioDelayForSleep2Listen = ((double) radioModule->par("delaySleep2Listen"))/1000.0;	
	radioDelayForListen2Tx = ((double) radioModule->par("delayListen2Tx"))/1000.0;
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
    
	epsilon = 0.000001f; 
	disabled = 1;		
	
	/**************************************************************************************************
	 *			802.15.4 specific intialize
	 **************************************************************************************************/

	symbolLen = 1/(radioDataRate*1000/BITS_PER_SYMBOL);
	ackWaitDuration = symbolLen*unitBackoffPeriod + radioDelayForListen2Tx + TX_TIME(ACK_PKT_SIZE);

	beaconPacket = NULL;
	associateRequestPacket = NULL;
	nextPacket = NULL;
	nextPacketResponse = 0;
	nextPacketRetries = 0;
	nextMacState = 0;

	beaconTimeoutMsg = NULL;
	
	macState = MAC_STATE_SETUP;
	macBSN = rand()%255;
	associatedPAN = -1;
	currentFrameStart = 0;
	lostBeacons = 0;
}

void Mac802154Module::handleMessage(cMessage *msg) {
    int msgKind = msg->kind();
    if((disabled) && (msgKind != APP_NODE_STARTUP)) {
    	delete msg;
    	msg = NULL;		// safeguard
    	return;
    }

    switch (msgKind) {

	/* This message is sent by the Application submodule in order to start/switch-on 
	 * the MAC submodule.
	 */
	case APP_NODE_STARTUP: {
	    disabled = 0;
	    setRadioState(MAC_2_RADIO_ENTER_LISTEN);
	    MAC_ControlMessage *csMsg = new MAC_ControlMessage("carrier sense strobe MAC->radio", MAC_2_RADIO_SENSE_CARRIER);
	    csMsg->setSense_carrier_interval(totalSimTime);
	    sendDelayed(csMsg,radioDelayForSleep2Listen+radioDelayForValidCS+epsilon ,"toRadioModule");
		
	    if (isFFD && isPANCoordinator) {
		CASTALIA_DEBUG << "backoff slot length is " << symbolLen*unitBackoffPeriod;
		associatedPAN = self;
		scheduleAt(simTime(), new MAC_ControlMessage("Beacon broadcast message", MAC_SELF_FRAME_START));
	    }
	    break;
	}

	
	/* This message indicates that a packet is received from upper layer (Network)
	 * First we check that the MAC buffer is capable of holding the new packet, also check 
	 * that length does not exceed the valid mac frame size. Then store a copy of the packet
	 * is stored in transmission buffer. We dont need to encapsulate the message here - it will
	 * be done separately for each transmission attempt
	 */
	case NETWORK_FRAME: {
	    if (TXBuffer.size() >= macBufferSize) {
		send(new MAC_ControlMessage("MAC buffer is full Radio->Mac", MAC_2_NETWORK_FULL_BUFFER), "toNetworkModule");
		CASTALIA_DEBUG << "WARNING: SchedTxBuffer FULL! Application packet is dropped";
		break;
	    }
	    
	    Network_GenericFrame *dataFrame = check_and_cast<Network_GenericFrame*>(msg->dup());
	    if (dataFrame->byteLength() + macFrameOverhead > maxMACFrameSize) {
		CASTALIA_DEBUG << "WARNING: Oversied packet dropped. Packet size:" << dataFrame->byteLength() 
		    << ", MAC overhead:" << macFrameOverhead << ", max MAC frame size:" << maxMACFrameSize;
		cancelAndDelete(dataFrame);
	    } else {
		TXBuffer.push(dataFrame);
		CASTALIA_DEBUG << "Packet accepted from network layer, buffer state: "<<TXBuffer.size()<<"/"<<macBufferSize;
		if (macState == MAC_STATE_IDLE) attemptTransmission();
	    }
	    break;
	}
	

	// These messages from the radio are currently unused
	case RADIO_2_MAC_STARTED_TX: { break; }
	case RADIO_2_MAC_STOPPED_TX: { break; }


	/* This is the core self message of the MAC, it indicates that the new frame has started
	 */
	case MAC_SELF_FRAME_START: {
	    if (isPANCoordinator) {	// as a PAN coordinator, a node should create and broadcast beacon packet
		beaconPacket = new MAC_GenericFrame("PAN beacon message", MAC_FRAME);
		beaconPacket->setKind(MAC_FRAME) ;
		beaconPacket->getHeader().PANid = self;
		beaconPacket->getHeader().frameType = MAC_802154_BEACON_FRAME;
		beaconPacket->getHeader().beaconOrder = beaconOrder;
		beaconPacket->getHeader().frameOrder = frameOrder;
		if (macBSN > 254) { macBSN = 0; } else { macBSN++; }
		beaconPacket->getHeader().BSN = macBSN;
		beaconPacket->getHeader().CAPlength = CAPlength; 
		beaconPacket->setByteLength(BASE_BEACON_PKT_SIZE + GTSlist.size()*3);
		beaconPacket->setGTSlistArraySize(GTSlist.size());
		for (int i = 0; i < GTSlist.size(); i++) {
		    beaconPacket->setGTSlist(i,GTSlist[i]);		    
		}
		
		CAPend = CAPlength * baseSlotDuration * ( 1 << frameOrder) * symbolLen;
		send(beaconPacket,"toRadioModule");
		setMacState(MAC_STATE_IN_TX);
		setRadioState(MAC_2_RADIO_ENTER_TX);
		attemptTransmission(TX_TIME(beaconPacket->byteLength()));
		beaconPacket = NULL;
		
		scheduleAt(simTime() + DRIFTED_TIME(beaconInterval*symbolLen), 
		    new MAC_ControlMessage("Beacon broadcast message", MAC_SELF_FRAME_START));
		currentFrameStart = simTime() + radioDelayForListen2Tx;
	    } else {		// if not a PAN coordinator, then wait for beacon
		setRadioState(MAC_2_RADIO_ENTER_LISTEN);
		setMacState(MAC_STATE_WAIT_FOR_BEACON);
		beaconTimeoutMsg = new MAC_ControlMessage("Beacon timeout message", MAC_SELF_BEACON_TIMEOUT);
		scheduleAt(simTime() + DRIFTED_TIME(BEACON_GUARD*3), beaconTimeoutMsg);
		currentFrameStart = simTime() + BEACON_GUARD;
	    }
	    break;
	}
	
	case MAC_SELF_RESET_TX: {
	    // previous transmission is reset, attempt a new transmission
	    attemptTransmission();
	    break;
	}
	
	/* We recieved a MAC packet from the lower layer (Physical, or radio)
	 * A separate function had been created to handle the task of processing 
	 * these packets to allow for better and cleaner code structure
	 */
	case MAC_FRAME: {
	    processMacFrame(check_and_cast<MAC_GenericFrame*>(msg));
	    break;
	}

	// self message to preform carrier sense
	case MAC_SELF_PERFORM_CCA: {
	    if (macState != MAC_STATE_CSMA_CA) break;
	    setMacState(MAC_STATE_CCA);
	    performCarrierSense();
	    break;
	}
	
	
	// beacon timeout fired - indicates that beacon was missed by this node
	case MAC_SELF_BEACON_TIMEOUT: {
	    lostBeacons++;
	    if (beaconTimeoutMsg == msg) { beaconTimeoutMsg = NULL; }
	    if (lostBeacons >= maxLostBeacons) {
		CASTALIA_DEBUG << "Lost synchronisation with PAN " << associatedPAN;
		setMacState(MAC_STATE_SETUP);
		associatedPAN = -1;
	    } else {
		setMacState(MAC_STATE_SLEEP);
		setRadioState(MAC_2_RADIO_ENTER_SLEEP);
		scheduleAt(currentFrameStart + DRIFTED_TIME(beaconInterval*symbolLen),
		    new MAC_ControlMessage("Beacon broadcast message", MAC_SELF_FRAME_START));                	
	    }
	    break;
	}

	// self message to go to sleep
	case MAC_SELF_SET_RADIO_SLEEP: {
	    setMacState(MAC_STATE_SLEEP);
	    setRadioState(MAC_2_RADIO_ENTER_SLEEP);
	    break;
	}

	/* This message is a response from the radio to our request to preform carrier sense 
	 * A separate function had been created to handle the task of processing
	 * this event to allow for better and cleaner code structure
	 */
	case RADIO_2_MAC_NO_SENSED_CARRIER: {
	    carrierIsClear();
	    break;
	}

	// carrier sense message received from the radio
	case RADIO_2_MAC_SENSED_CARRIER: {
	    lastCarrierSense = simTime();
	    if (macState != MAC_STATE_CCA) break;
	    
	    //if MAC is preforming CSMA-CA, update corresponding parameters
	    CW = enableSlottedCSMA ? 2 : 1; NB++; BE++;
	    if (BE > macMaxBE) BE = macMaxBE;
	    if (NB > macMaxCSMABackoffs) {
		if (nextPacketRetries > 0) nextPacketRetries--;
		attemptTransmission();
	    } else {
	        setMacState(MAC_STATE_CSMA_CA);
		continueCSMACA();
	    }
	    break;
	}

	case RADIO_2_MAC_FULL_BUFFER: {
	    CASTALIA_DEBUG << "Mac module received RADIO_2_MAC_FULL_BUFFER because the Radio buffer is full.";
	    break;
	}

	
	// Energy is depleted, MAC can no longer operate
	case RESOURCE_MGR_OUT_OF_ENERGY: {
	    disabled = 1;
	    break;
	}	
		

	default: {
	    CASTALIA_DEBUG << "WARNING: received message of unknown type: " << msg;
	    break;
	}

    }//end_of_switch

    delete msg;
    msg = NULL;		// safeguard		
}


void Mac802154Module::finish() {
    while(!TXBuffer.empty()) {
	cancelAndDelete(TXBuffer.front());
	TXBuffer.pop();
    }
    if (nextPacket) cancelAndDelete(nextPacket);
}

void Mac802154Module::readIniFileParameters(void) {
    printDebugInfo = par("printDebugInfo");
    printStateTransitions = par("printStateTransitions");
    macBufferSize = par("macBufferSize");
    maxMACFrameSize = par("maxMACFrameSize");
    macFrameOverhead = par("macFrameOverhead");
    enableSlottedCSMA = par("enableSlottedCSMA");
        
    isPANCoordinator = hasPar("isPANCoordinator") ? par("isPANCoordinator") : false;
    isFFD = hasPar("isFFD") ? par("isFFD") : false;

    unitBackoffPeriod = hasPar("unitBackoffPeriod") ? par("unitBackoffPeriod") : 20;
    baseSlotDuration = hasPar("baseSlotDuration") ? par("baseSlotDuration") : 60;
    numSuperframeSlots = hasPar("numSuperframeSlots") ? par("numSuperframeSlots") : 16;
    maxLostBeacons = hasPar("maxLostBeacons") ? par("maxLostBeacons") : 4;
    macMinBE = hasPar("macMinBE") ? par("macMinBE") : 2;
    macMaxBE = hasPar("macMaxBE") ? par("macMaxBE") : 5;
    macMaxCSMABackoffs = hasPar("macMacCSMABackoffs") ? par("macMacCSMABackoffs") : 4;
    macMaxFrameRetries = hasPar("macMacFrameRetries") ? par("macMacFrameRetries") : 3;
    batteryLifeExtention = hasPar("batteryLifeExtention") ? par("batteryLifeExtention") : false;
    baseSuperframeDuration = baseSlotDuration * numSuperframeSlots;
    

    if (isPANCoordinator) {
	if (!isFFD) {
	    opp_error("Only full-function devices (isFFD=true) can be PAN coordinators");
	}
	
	frameOrder = par("frameOrder");
	beaconOrder = par("beaconOrder");
	if (frameOrder < 0 || beaconOrder < 0 || beaconOrder > 14 || frameOrder > 14 || beaconOrder < frameOrder) {
	    opp_error("Invalid combination of frameOrder and beaconOrder parameters. Must be 0 <= frameOrder <= beaconOrder <= 14");
	}

	beaconInterval = baseSuperframeDuration * ( 1 << beaconOrder);
	frameInterval = baseSuperframeDuration * ( 1 << frameOrder);
	CAPlength = numSuperframeSlots;
	
	if (beaconInterval <= 0 || frameInterval <= 0) {
	    opp_error("Invalid parameter combination of baseSlotDuration and numSuperframeSlots");
	}
    }
}


void Mac802154Module::setRadioState(MAC_ControlMessageType typeID, double delay) {
    if( (typeID != MAC_2_RADIO_ENTER_SLEEP) && (typeID != MAC_2_RADIO_ENTER_LISTEN) && (typeID != MAC_2_RADIO_ENTER_TX) )
	opp_error("MAC attempt to set Radio into an unknown state. ERROR commandID");
    MAC_ControlMessage * ctrlMsg = new MAC_ControlMessage("state command strobe MAC->radio", typeID);
    sendDelayed(ctrlMsg, delay, "toRadioModule");
}


void Mac802154Module::setRadioTxMode(Radio_TxMode txTypeID, double delay) {
    if( (txTypeID != CARRIER_SENSE_NONE)&&(txTypeID != CARRIER_SENSE_ONCE_CHECK)&&(txTypeID != CARRIER_SENSE_PERSISTENT) )
	opp_error("MAC attempt to set Radio CarrierSense into an unknown type. ERROR commandID");

    MAC_ControlMessage * ctrlMsg = new MAC_ControlMessage("Change Radio TX mode command strobe MAC->radio", MAC_2_RADIO_CHANGE_TX_MODE);
    ctrlMsg->setRadioTxMode(txTypeID);
    sendDelayed(ctrlMsg, delay, "toRadioModule");
}

void Mac802154Module::setRadioPowerLevel(int powLevel, double delay) {
    if( (powLevel >= 0) && (powLevel < radioModule->getTotalTxPowerLevels()) ) {
	MAC_ControlMessage * ctrlMsg = new MAC_ControlMessage("Set power level command strobe MAC->radio", MAC_2_RADIO_CHANGE_POWER_LEVEL);
	ctrlMsg->setPowerLevel(powLevel);
	sendDelayed(ctrlMsg, delay, "toRadioModule");
    } else
	CASTALIA_DEBUG << "WARNING: in function setRadioPowerLevel() of Mac module, parameter powLevel has invalid value.\n";
}


/* Helper function to change internal MAC state and print a debug statement if neccesary */
void Mac802154Module::setMacState(int newState) {
    if (macState == newState) return;
    if (printStateTransitions) {
	CASTALIA_DEBUG << "state changed from " << stateDescr[macState] << " to " << stateDescr[newState];
    }
    macState = newState;
}

/* This function will handle a MAC frame received from the lower layer (physical or radio) 
 */
void Mac802154Module::processMacFrame(MAC_GenericFrame *rcvFrame) {

    switch (rcvFrame->getHeader().frameType) {
    	
	/* received a BEACON frame */
	case MAC_802154_BEACON_FRAME: {

	    int PANaddr = rcvFrame->getHeader().PANid;
	    if (associatedPAN == -1) {	
		//if not associated - create request packet to do so
		if (nextPacket) { cancelAndDelete(nextPacket); }
		nextPacket = new MAC_GenericFrame("PAN associate request", MAC_FRAME);
		nextPacket->setKind(MAC_FRAME);
		nextPacket->getHeader().PANid = PANaddr;
		nextPacket->getHeader().frameType = MAC_802154_ASSOCIATE_FRAME;
		nextPacket->getHeader().srcID = self;
		nextPacket->setByteLength(6);
		initiateCSMACA(-1,MAC_STATE_WAIT_FOR_ASSOCIATE_RESPONSE,ackWaitDuration + TX_TIME(6));
	    } else if (associatedPAN != PANaddr) {
		// if associated to a different PAN - do nothing
		return;
	    }
	    
	    //update frame parameters
	    currentFrameStart = lastCarrierSense;
	    lostBeacons = 0;
	    frameOrder = rcvFrame->getHeader().frameOrder;
	    beaconOrder = rcvFrame->getHeader().beaconOrder;
	    beaconInterval = baseSuperframeDuration * ( 1 << beaconOrder);
            macBSN = rcvFrame->getHeader().BSN;
	    CAPlength = rcvFrame->getHeader().CAPlength;
	    CAPend = CAPlength * baseSlotDuration * ( 1 << frameOrder) * symbolLen;
	    
	    //cancel beacon timeout message (if present)
	    if(beaconTimeoutMsg) { 
		cancelAndDelete(beaconTimeoutMsg); 
		beaconTimeoutMsg = NULL; 
	    }
	    
	    //schedule sleep and wakeup
	    scheduleAt(simTime() + DRIFTED_TIME(CAPend), 
		new MAC_ControlMessage("CAP end message", MAC_SELF_SET_RADIO_SLEEP));
	    scheduleAt(simTime() + DRIFTED_TIME(baseSuperframeDuration * ( 1 << beaconOrder) * symbolLen - BEACON_GUARD),
	        new MAC_ControlMessage("Beacon broadcast message", MAC_SELF_FRAME_START));
	    
	    //if already associated to this pan - attempt transmission
	    if (associatedPAN == PANaddr) attemptTransmission();
	    
            break;
	}
	
	
	// request to associate
	case MAC_802154_ASSOCIATE_FRAME: {
	    // only PAN coordinators can accept association requests
	    // if multihop communication is to be allowed - then this has to be changed
	    // in particular, any FFD can become a coordinator and accept requests
	    if (!isPANCoordinator) break;
	    
	    // if PAN id is not the same as my ID - do nothing
	    if (rcvFrame->getHeader().PANid != self) break;
	    
	    // update associatedDevices and reply with an ACK (i.e. association is always allowed)
	    CASTALIA_DEBUG << "Received REQ frame from " << rcvFrame->getHeader().srcID;
	    associatedDevices[rcvFrame->getHeader().srcID] = true;
	    MAC_GenericFrame *ackFrame = new MAC_GenericFrame("PAN associate response", MAC_FRAME);
	    ackFrame->setKind(MAC_FRAME);
	    ackFrame->getHeader().PANid = self;
	    ackFrame->getHeader().frameType = MAC_802154_ACK_FRAME;
	    ackFrame->getHeader().destID = rcvFrame->getHeader().srcID;
	    ackFrame->setByteLength(ACK_PKT_SIZE);
	    send(ackFrame,"toRadioModule");
	    
	    setRadioState(MAC_2_RADIO_ENTER_TX,PROCESSING_DELAY);
	    setMacState(MAC_STATE_IN_TX);
	    attemptTransmission(TX_TIME(ackFrame->byteLength()));
	    break;
	}
	
	// ack frames are handled by a separate function
	case MAC_802154_ACK_FRAME: {
	    if (rcvFrame->getHeader().destID != self) break;
	    handleAckFrame(rcvFrame);
	    break;
	}
	
	// data frame
	case MAC_802154_DATA_FRAME: {
	    int dstAddr = rcvFrame->getHeader().destID;
	    if (dstAddr != self && dstAddr != BROADCAST_ADDR) break;
	    
	    Network_GenericFrame *netDataFrame;
            netDataFrame = check_and_cast<Network_GenericFrame *>(rcvFrame->decapsulate());
            netDataFrame->setRssi(rcvFrame->getRssi());
            send(netDataFrame, "toNetworkModule");
            
            // If the frame was sent to broadcast address, nothing else needs to be done
            if (dstAddr == BROADCAST_ADDR) break;
                        
	    MAC_GenericFrame *ackFrame = new MAC_GenericFrame("PAN associate response", MAC_FRAME);
	    ackFrame->setKind(MAC_FRAME);
	    ackFrame->getHeader().srcID = self;
	    ackFrame->getHeader().frameType = MAC_802154_ACK_FRAME;
	    ackFrame->getHeader().destID = rcvFrame->getHeader().srcID;
	    ackFrame->setByteLength(ACK_PKT_SIZE);
	    
	    send(ackFrame,"toRadioModule");
	    setRadioState(MAC_2_RADIO_ENTER_TX,PROCESSING_DELAY);
	    setMacState(MAC_STATE_IN_TX);
	    attemptTransmission(TX_TIME(ackFrame->byteLength()));                                                                                                                                                
	    
	    break;
	}
	
	default: {
	    CASTALIA_DEBUG << "WARNING: unknown frame type received " << rcvFrame->getHeader().frameType;
	}
    }	
}

void Mac802154Module::handleAckFrame(MAC_GenericFrame *rcvFrame) {
    switch (macState) {
    
	//received an ack while waiting for a response to association request
	case MAC_STATE_WAIT_FOR_ASSOCIATE_RESPONSE: {
	    associatedPAN = rcvFrame->getHeader().PANid;
	    CASTALIA_DEBUG << "associated with PAN:" << associatedPAN;
	    cancelAndDelete(nextPacket);
	    nextPacket = NULL;
	    attemptTransmission();
	    break;
	}
	
	//received an ack while waiting for a response to data packet
	case MAC_STATE_WAIT_FOR_DATA_ACK: {
	    if (isPANCoordinator || associatedPAN == rcvFrame->getHeader().srcID) {
		if (nextPacket) {
		    cancelAndDelete(nextPacket);
		    nextPacket = NULL;
		}
		attemptTransmission();
	    }
	    break;
	}
	
	default: {
	    CASTALIA_DEBUG << "Received ACK in " << stateDescr[macState];
	    break;
	}
    }
}

// This function will initiate a transmission (or retransmission) attempt after a given delay
void Mac802154Module::attemptTransmission(double delay) {
    // if current frame active time has expired - no transmissions can be initiated
    if (currentFrameStart + CAPend < simTime()) return;
    
    // if delay is positive, schedule a self message
    if (delay > 0) {
	scheduleAt(simTime() + DRIFTED_TIME(delay),
            new MAC_ControlMessage("Transmission reset message", MAC_SELF_RESET_TX));
        return;        
    }

    // if a packet already queued for transmission - check avaliable retries
    if (nextPacket) {	
	if (nextPacketRetries == 0) {
	    cancelAndDelete(nextPacket);
	    nextPacket = NULL;
	} else {
	    initiateCSMACA();
	    return;
	}
    }
    
    // if not associated to a PAN - cannot initiate transmissions other than association requests
    if (associatedPAN == -1) return;
    
    // extract a packet from transmission buffer
    if (TXBuffer.size() > 0) {
	Network_GenericFrame * tmpFrame = TXBuffer.front(); TXBuffer.pop();
	string strNextHop(tmpFrame->getHeader().nextHop.c_str());
	int txAddr = strNextHop.compare(BROADCAST) == 0 ? BROADCAST_ADDR : 
		(isPANCoordinator ? atoi(strNextHop.c_str()) : associatedPAN);
	nextPacket = new MAC_GenericFrame("MAC data frame", MAC_FRAME);
	nextPacket->setByteLength(macFrameOverhead); //extra bytes will be added after the encapsulation
	nextPacket->getHeader().srcID = self;
	nextPacket->getHeader().destID = txAddr;
	nextPacket->getHeader().frameType = MAC_802154_DATA_FRAME;
	nextPacket->encapsulate(check_and_cast<Network_GenericFrame *>(tmpFrame));
	if (txAddr == BROADCAST_ADDR) {
	    initiateCSMACA(0,MAC_STATE_IDLE,0);
	} else {
	    initiateCSMACA(macMaxFrameRetries,MAC_STATE_WAIT_FOR_DATA_ACK,ackWaitDuration + TX_TIME(nextPacket->byteLength()));
	}
	return;
    }
    
    setMacState(MAC_STATE_IDLE);
}

// initiate CSMA-CA algorithm, initialising retries, next state and response values
void Mac802154Module::initiateCSMACA(int retries, int nextState, double response) {
    nextPacketRetries = retries;
    nextMacState = nextState;
    nextPacketResponse = response;
    initiateCSMACA();
}

// initiate CSMA-CA algorithm
void Mac802154Module::initiateCSMACA() {
    if (macState == MAC_STATE_CSMA_CA) {
	CASTALIA_DEBUG << "WARNING: cannot initiate CSMA-CA algorithm while in MAC_STATE_CSMA_CA";
	return;
    }
    setMacState(MAC_STATE_CSMA_CA);
    NB = 0;
    CW = enableSlottedCSMA ? 2 : 1;
    BE = batteryLifeExtention ? (macMinBE < 2 ? macMinBE : 2) : macMinBE;
    continueCSMACA();
}

// continue CSMA-CA algorithm 
void Mac802154Module::continueCSMACA() {
    if (macState != MAC_STATE_CSMA_CA) {
	CASTALIA_DEBUG << "WARNING: continueCSMACA called not in MAC_STATE_CSMA_CA";
	return;
    }
    
    //generate a random relay, multiply it by backoff period length
    int rnd = genk_intrand(1,(1<<BE)-1);
    double CCAtime = rnd*(unitBackoffPeriod*symbolLen);
    
    //if using slotted CSMA - need to locate backoff period boundary
    if (enableSlottedCSMA) {
	double backoffBoundary = (ceil((simTime() - currentFrameStart)/(unitBackoffPeriod*symbolLen)) - 
			        	(simTime() - currentFrameStart)/(unitBackoffPeriod*symbolLen)) * 
			        	(unitBackoffPeriod*symbolLen);
	CCAtime += backoffBoundary;
    }
    
    //create a self message to preform carrier sense after calculated time
    scheduleAt(simTime() + DRIFTED_TIME(CCAtime),
        new MAC_ControlMessage("CSMA-CA trigger message", MAC_SELF_PERFORM_CCA));
}

// carrier is clear (for CSMA-CA algorithm)
void Mac802154Module::carrierIsClear() {
    if (macState != MAC_STATE_CCA) return;
    CW--;
    
    if (CW != 0) {
	// since carrier is clear, no need to generate another random delay
    	setMacState(MAC_STATE_CSMA_CA);
	scheduleAt(simTime() + DRIFTED_TIME(unitBackoffPeriod*symbolLen),
	    new MAC_ControlMessage("CSMA-CA trigger message", MAC_SELF_PERFORM_CCA));
	return;
    }
    
    // CSMA-CA successful (CW == 0), can transmit the queued packet
    if (!nextPacket) {
    	CASTALIA_DEBUG << "ERROR: CSMA_CA algorithm executed without any data to transmit";
    	attemptTransmission();
    	return;
    }
    
    if (nextPacketResponse > 0) {
	setMacState(nextMacState);
	attemptTransmission(nextPacketResponse);
    } else {
	setMacState(MAC_STATE_IN_TX);
	attemptTransmission(TX_TIME(nextPacket->byteLength()));
    }

    if (nextPacketRetries > 0) nextPacketRetries--;
    send(check_and_cast<MAC_GenericFrame *>(nextPacket->dup()),"toRadioModule");
    setRadioState(MAC_2_RADIO_ENTER_TX);
}

/* This function will create a request to RADIO module to perform an instantaneous 
 * carrier sense.
 */
void Mac802154Module::performCarrierSense() {
    /* Here we check for validiry of carrier sense of the radio module
     * (This can be seen as checking the pin which is the indicator of wether 
     * carrier sensed pin is valid or not)
     */
    int isCarrierSenseValid_ReturnCode = radioModule->isCarrierSenseValid();
    if(isCarrierSenseValid_ReturnCode == 1) {
	/* If carrier sense indication of Radio is Valid, then we can
	 * send a command to perform Carrier Sense to the radio (i.e. check the 
	 * actual carrier sense pin)
	 */
	send(new MAC_ControlMessage("carrier sense strobe MAC->radio", MAC_2_RADIO_SENSE_CARRIER_INSTANTANEOUS), "toRadioModule");
    } else {
        // carrier sense indication of Radio is NOT Valid and isCarrierSenseValid_ReturnCode 
        // holds the cause for the non valid carrier sense indication. 
        // It is only possible to treat this situation as carrier is busy (since 802.15.4 
        // only interested in instant carrier sense)
        
	CASTALIA_DEBUG << "WARNING: carrier sense while radio is not ready " << isCarrierSenseValid_ReturnCode;
    }
}

