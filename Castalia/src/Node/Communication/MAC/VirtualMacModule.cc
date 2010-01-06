//***************************************************************************************
//*  Copyright: National ICT Australia,  2009, 2010					*
//*  Developed at the Networks and Pervasive Computing program				*
//*  Author(s): Yuriy Tselishchev							*
//*  This file is distributed under the terms in the attached LICENSE file.		*
//*  If you do not find this file, copies can be found by writing to:			*
//*											*
//*      NICTA, Locked Bag 9013, Alexandria, NSW 1435, Australia			*
//*      Attention:  License Inquiry.							*
//*											*
//***************************************************************************************



#include "VirtualMacModule.h"

void VirtualMacModule::initialize() {

    self = getParentModule()->getParentModule()->getIndex();

    //get a valid reference to the object of the Radio module so that we can make direct calls to its public methods
    //instead of using extra messages & message types for tighlty couplped operations.
    radioModule = check_and_cast<RadioModule*>(gate("toRadioModule")->getNextGate()->getOwnerModule());

    //get a valid reference to the object of the Resources Manager module so that we can make direct calls to its public methods
    //instead of using extra messages & message types for tighlty couplped operations.
    if(getParentModule()->getParentModule()->findSubmodule("nodeResourceMgr") != -1) {
	resMgrModule = check_and_cast<ResourceGenericManager*>(getParentModule()->getParentModule()->getSubmodule("nodeResourceMgr"));
    } else {
	opp_error("\n[Mac]:\n Error in geting a valid reference to nodeResourceMgr for direct method calls.");
    }
    cpuClockDrift = resMgrModule->getCPUClockDrift();
    
    selfCarrierSenseMsg = NULL;
    disabled = 1;
}

void VirtualMacModule::startup() {}
void VirtualMacModule::radioBufferFullCallback() {}

void VirtualMacModule::carrierSenseCallback(int returnCode) {}

void VirtualMacModule::carrierSense(simtime_t time) {
    
    if (selfCarrierSenseMsg) { cancelAndDelete(selfCarrierSenseMsg); selfCarrierSenseMsg = NULL; }

    if (time > 0) {
	selfCarrierSenseMsg = new MAC_ControlMessage("Delayed carrier sense message", MAC_SELF_CARRIER_SENSE);
	scheduleAt(simTime() + cpuClockDrift * time, selfCarrierSenseMsg);
	return;                
    }

    //during this procedure we check if the carrier sense indication of the Radio is valid.
    int isCarrierSenseValid_ReturnCode = radioModule->isCarrierSenseValid();

    if(isCarrierSenseValid_ReturnCode == 1) {
        send(new MAC_ControlMessage("carrier sense strobe MAC->radio", MAC_2_RADIO_SENSE_CARRIER_INSTANTANEOUS), "toRadioModule");
    } else {
	carrierSenseCallback(isCarrierSenseValid_ReturnCode);
    }
}

void VirtualMacModule::timerFiredCallback(int timerIndex) {}

void VirtualMacModule::cancelTimer(int timerIndex) {
    map<int,MAC_ControlMessage *>::iterator iter = timerMessages.find(timerIndex);
    if(iter != timerMessages.end()) {
        if (timerMessages[timerIndex] != NULL && timerMessages[timerIndex]->isScheduled()) {
            cancelAndDelete(timerMessages[timerIndex]);
	}
	timerMessages.erase(iter);
    }

}

void VirtualMacModule::setTimer(int timerIndex, simtime_t time) {
    cancelTimer(timerIndex);
    timerMessages[timerIndex] = new MAC_ControlMessage("MAC timer", MAC_SELF_TIMER);
    timerMessages[timerIndex]->setTimerIndex(timerIndex);
    scheduleAt(simTime() + cpuClockDrift * time, timerMessages[timerIndex]);
}

void VirtualMacModule::processOverheardFrame(MAC_GenericFrame* rcvFrame) {}

int VirtualMacModule::bufferFrame(MAC_GenericFrame* rcvFrame) {
    if ((int)TXBuffer.size() >= (int)par("macBufferSize")) {
	send(new MAC_ControlMessage("MAC buffer is full Radio->Mac", MAC_2_NETWORK_FULL_BUFFER), "toNetworkModule");
	trace() << "TXBuffer is FULL! Network layer packet is dropped";
	return 0;
    } else {
    	TXBuffer.push(rcvFrame);
	trace() << "Packet accepted from network layer, buffer state: " <<
		    TXBuffer.size() << "/" << (int)par("macBufferSize");
	return 1; 
    }
}	
                
void VirtualMacModule::handleMessage(cMessage *msg) {
    
    int msgKind = (int)msg->getKind();

    if(disabled && msgKind != APP_NODE_STARTUP) {
	delete msg;
	msg = NULL;		// safeguard
	return;
    }

    switch (msgKind) {

	case APP_NODE_STARTUP: {

	    disabled = 0;
	    setRadioState(MAC_2_RADIO_ENTER_LISTEN);
	    MAC_ControlMessage *csMsg = new MAC_ControlMessage("carrier sense strobe MAC->radio", MAC_2_RADIO_SENSE_CARRIER);
	    csMsg->setSense_carrier_interval(strtod(ev.getConfig()->getConfigValue("sim-time-limit"),NULL));
	    sendDelayed(csMsg,((double)radioModule->par("delaySleep2Listen")+(double)radioModule->par("delayCSValid"))/1000.0,"toRadioModule");

	    startup();
	    break;
	}

	case NETWORK_FRAME: {
	    Network_GenericFrame *dataFrame = check_and_cast<Network_GenericFrame*>(msg->dup());
	    if ((int)par("macMaxFrameSize") > 0 && (int)par("macMaxFrameSize") < 
		    dataFrame->getByteLength() + (int)par("macFrameOverhead")) {
		trace() << "Oversized packet dropped. Size:" << dataFrame->getByteLength() << 
			    ", MAC layer overhead:" << (int)par("macFrameOverhead") << 
			    ", max MAC frame size:" << (int)par("macMaxFrameSize");
		cancelAndDelete(dataFrame);
		break;
	    }
	    MAC_GenericFrame *macFrame = new MAC_GenericFrame("MAC data frame", MAC_FRAME);
	    encapsulateNetworkFrame(dataFrame,macFrame);
	    fromNetworkLayer(macFrame);
	    break;
	}
	
	case MAC_SELF_TIMER: {
	    MAC_ControlMessage *ctrlMsg = check_and_cast<MAC_ControlMessage*>(msg);
	    int timerIndex = ctrlMsg->getTimerIndex();
	    ctrlMsg = NULL;
	    map<int,MAC_ControlMessage *>::iterator iter = timerMessages.find(timerIndex);
	    if(iter != timerMessages.end()) {
		timerMessages.erase(iter);
		timerFiredCallback(timerIndex);
	    } else {
		trace() << "WARNING: Timer service malfunction, unknown timer id " << timerIndex;
	    }
	    break;
	}
	    
	case MAC_FRAME:	{
	    MAC_GenericFrame *rcvFrame = check_and_cast<MAC_GenericFrame*>(msg);
	    if (rcvFrame->getHeader().destID == self || rcvFrame->getHeader().destID == MAC_BROADCAST_ADDR) {
		fromRadioLayer(rcvFrame);
	    } else {
		processOverheardFrame(rcvFrame);
	    }
	    break;
	}

	case RADIO_2_MAC_SENSED_CARRIER: {
	    carrierSenseCallback(CARRIER_BUSY);
	    break;
	}

	case RADIO_2_MAC_NO_SENSED_CARRIER: {
	    carrierSenseCallback(CARRIER_CLEAR);
            break;
        }
                                
	case RADIO_2_MAC_FULL_BUFFER: {
	    radioBufferFullCallback();
	    break;
	}

	case RADIO_2_MAC_STARTED_TX: { break; }
	case RADIO_2_MAC_STOPPED_TX: { break; }               

	case RESOURCE_MGR_OUT_OF_ENERGY: {
	    disabled = 1;
	    break;
	}

	case RESOURCE_MGR_DESTROY_NODE: {
    	    disabled = 1;
	    break;
	}
		
	case APPLICATION_2_MAC_SETRADIOTXPOWER: {
    	    App_ControlMessage *ctrlFrame;
	    ctrlFrame = check_and_cast<App_ControlMessage*>(msg);
			
	    int msgTXPowerLevel = ctrlFrame->getRadioTXPowerLevel();
	    setRadioPowerLevel(msgTXPowerLevel);
	    
	    ctrlFrame = NULL;
	    break;
	}
		
	case APPLICATION_2_MAC_SETRADIOTXMODE: {
	    App_ControlMessage *ctrlFrame;
	    ctrlFrame = check_and_cast<App_ControlMessage*>(msg);
			
	    int msgTXMode = ctrlFrame->getRadioTXMode();

   	    switch (msgTXMode) {
		case 0: {
   		    setRadioTxMode(CARRIER_SENSE_NONE);
   		    break;
   		}
   		
   		case 1: {
   		    setRadioTxMode(CARRIER_SENSE_ONCE_CHECK);
   		    break;
   		}
   						
   		case 2: {
   		    setRadioTxMode(CARRIER_SENSE_PERSISTENT);
   		    break;
   		}
   						
   		default: {
   		    trace() << "Application module sent an invalid SETRADIOTXMODE command strobe.";
   		    break;
   		}
  	    }
	    
	    ctrlFrame = NULL;
	    break;
	}
		
	case APPLICATION_2_MAC_SETRADIOSLEEP: {
	    setRadioState(MAC_2_RADIO_ENTER_SLEEP);
    	    break;
	}
		
	case APPLICATION_2_MAC_SETRADIOLISTEN: {
	    setRadioState(MAC_2_RADIO_ENTER_LISTEN);
	    break;
	}
		
	default: {
	    trace() << "WARNING: received packet of unknown type" << msgKind;
    	    break;
	}

    }

    delete msg; 
    msg = NULL;		// safeguard
}

void VirtualMacModule::finish() {
    finishSpecific();
    while(!TXBuffer.empty()) {
	cancelAndDelete(TXBuffer.front());
        TXBuffer.pop();
    }
    VirtualCastaliaModule::finish();
}


void VirtualMacModule::setRadioState(MAC_ControlMessageType typeID, simtime_t delay) {
    if( (typeID != MAC_2_RADIO_ENTER_SLEEP) && (typeID != MAC_2_RADIO_ENTER_LISTEN) && (typeID != MAC_2_RADIO_ENTER_TX) )
	opp_error("MAC attempt to set Radio into an unknown state. ERROR commandID");

    MAC_ControlMessage * ctrlMsg = new MAC_ControlMessage("state command strobe MAC->radio", typeID);
    sendDelayed(ctrlMsg, delay, "toRadioModule");
}

void VirtualMacModule::setRadioTxMode(Radio_TxMode txTypeID, simtime_t delay) {
    if( (txTypeID != CARRIER_SENSE_NONE)&&(txTypeID != CARRIER_SENSE_ONCE_CHECK)&&(txTypeID != CARRIER_SENSE_PERSISTENT) )
    	opp_error("MAC attempt to set Radio CarrierSense into an unknown type. ERROR commandID");

    MAC_ControlMessage * ctrlMsg = new MAC_ControlMessage("Change Radio TX mode command strobe MAC->radio", MAC_2_RADIO_CHANGE_TX_MODE);
    ctrlMsg->setRadioTxMode(txTypeID);
    sendDelayed(ctrlMsg, delay, "toRadioModule");
}

void VirtualMacModule::setRadioPowerLevel(int powLevel, simtime_t delay) {
    if(powLevel >= 0 && powLevel < radioModule->getTotalTxPowerLevels()) {
	MAC_ControlMessage * ctrlMsg = new MAC_ControlMessage("Set power level command strobe MAC->radio", MAC_2_RADIO_CHANGE_POWER_LEVEL);
	ctrlMsg->setPowerLevel(powLevel);
	sendDelayed(ctrlMsg, delay, "toRadioModule");
    }
    else
	trace() <<"WARNING: in function setRadioPowerLevel() of Mac module, parameter powLevel has invalid value";
}

void VirtualMacModule::encapsulateNetworkFrame(Network_GenericFrame *networkFrame, MAC_GenericFrame *retFrame) {
    retFrame->setByteLength((int)par("macFrameOverhead")); //networkFrame->getByteLength() extra bytes will be added after the encapsulation
    retFrame->getHeader().srcID = self;
    retFrame->getHeader().destID = resolveNextHop(networkFrame->getHeader().nextHop.c_str());
    retFrame->getHeader().frameType = (unsigned short)MAC_PROTO_DATA_FRAME;
    retFrame->encapsulate(networkFrame);
}

int VirtualMacModule::resolveNextHop(const char *nextHop) {
    string strNextHop(nextHop);
    // Resolve Network-layer IDs to MAC-layer IDs
    if(strNextHop.compare(BROADCAST) == 0) {
	return MAC_BROADCAST_ADDR;
    } else {
	return atoi(strNextHop.c_str());
    }
}

void VirtualMacModule::toNetworkLayer(MAC_GenericFrame* macFrame) {
    Network_GenericFrame *netDataFrame;
    netDataFrame = check_and_cast<Network_GenericFrame *>(macFrame->decapsulate());
    netDataFrame->setRssi(macFrame->getRssi());
    send(netDataFrame, "toNetworkModule");                                                
}

void VirtualMacModule::toRadioLayer(MAC_GenericFrame* macFrame) {
    macFrame->setKind(MAC_FRAME);
    send(macFrame, "toRadioModule");
    setRadioState(MAC_2_RADIO_ENTER_TX);
}
