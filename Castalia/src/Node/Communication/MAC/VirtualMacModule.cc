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
    macBufferSize = par("macBufferSize");
    macFrameOverhead = par("macPacketOverhead");
    macMaxFrameSize = par("macMaxPacketSize");
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
    setTimerDrift(resMgrModule->getCPUClockDrift());
    disabled = 1;
    //declareOutput("Buffer overflow");
}

int VirtualMacModule::handleControlCommand(cMessage *msg) {
    trace() << "WARNING: handleControlCommand not defined in this module";
    return 0;
}

int VirtualMacModule::handleRadioControlMessage(cMessage *msg) {
    toNetworkLayer(msg);
    return 1;
}

int VirtualMacModule::bufferPacket(cPacket* rcvFrame) {
	if ((int)TXBuffer.size() >= macBufferSize) {
		//collectOutput("Buffer overflow");  this is handled in specific MACs
		cancelAndDelete(rcvFrame);
		// send a control message to the upper layer
		MacControlMessage *fullBuffMsg = new MacControlMessage("MAC buffer full", MAC_CONTROL_MESSAGE);
		fullBuffMsg->setMacControlMessageKind(MAC_BUFFER_FULL);
		send(fullBuffMsg, "toNetworkModule");
		return 0;
	} else {
		TXBuffer.push(rcvFrame);
		trace() << "Packet buffered from network layer, buffer state: " << TXBuffer.size() << "/" << macBufferSize;
		return 1;
	}
}

void VirtualMacModule::handleMessage(cMessage *msg) {

    int msgKind = (int)msg->getKind();

    if(disabled && msgKind != NODE_STARTUP) {
	delete msg;
	msg = NULL;		// safeguard
	return;
    }

    switch (msgKind) {

	case NODE_STARTUP: {
	    disabled = 0;
	    startup();
	    break;
	}

	case NETWORK_LAYER_PACKET: {
	    NetworkGenericPacket *pkt = check_and_cast<NetworkGenericPacket*>(msg);
	    if (macMaxFrameSize > 0 && macMaxFrameSize < pkt->getByteLength() + macFrameOverhead) {
		trace() << "Oversized packet dropped. Size:" << pkt->getByteLength() <<
			    ", MAC layer overhead:" << macFrameOverhead <<
			    ", max MAC frame size:" << macMaxFrameSize;
		break;
	    }
	    // trace() << "Received [" << pkt->getName() << "] from Network layer";
	    fromNetworkLayer(pkt, pkt->getNetworkInteractionControl().nextHop);
	    return;
	}

	case TIMER_SERVICE: {
	    handleTimerMessage(msg);
	    break;
	}

	case MAC_LAYER_PACKET: {
	    MacGenericPacket *pkt = check_and_cast<MacGenericPacket*>(msg);
	    // trace() << "Received [" << pkt->getName() << "] from Radio layer";
	    fromRadioLayer(pkt,pkt->getMacInteractionControl().RSSI,pkt->getMacInteractionControl().LQI);
	    break;
	}

	case OUT_OF_ENERGY: {
	    disabled = 1;
	    break;
	}

	case MAC_CONTROL_COMMAND: {
	    if (handleControlCommand(msg)) return;
	    break;
	}

	case RADIO_CONTROL_COMMAND: {
	    toRadioLayer(msg);
	    return;
	}

	case RADIO_CONTROL_MESSAGE: {
	    if (handleRadioControlMessage(msg)) return;
	    break;
	}

	default: {
	    opp_error("MAC module received message of unknown kind %i",msgKind);
	}
    }

    delete msg;
    msg = NULL;		// safeguard
}

void VirtualMacModule::finish() {
    VirtualCastaliaModule::finish();
    while(!TXBuffer.empty()) {
	cancelAndDelete(TXBuffer.front());
        TXBuffer.pop();
    }
}

void VirtualMacModule::toNetworkLayer(cMessage* macMsg) {
	trace() << "Delivering [" << macMsg->getName() << "] to Network layer";
    send(macMsg, "toNetworkModule");
}

void VirtualMacModule::toRadioLayer(cMessage* macMsg) {
    send(macMsg, "toRadioModule");
}

void VirtualMacModule::encapsulatePacket(cPacket *macPkt, cPacket *netPkt) {
    macPkt->setByteLength(macFrameOverhead);
    macPkt->setKind(MAC_LAYER_PACKET);
    macPkt->encapsulate(netPkt);
}

cPacket * VirtualMacModule::decapsulatePacket(cPacket *pkt) {
    MacGenericPacket *macPkt = check_and_cast<MacGenericPacket*>(pkt);
    NetworkGenericPacket *netPkt = check_and_cast<NetworkGenericPacket*>(macPkt->decapsulate());
    netPkt->getNetworkInteractionControl().RSSI = macPkt->getMacInteractionControl().RSSI;
    netPkt->getNetworkInteractionControl().LQI = macPkt->getMacInteractionControl().LQI;
    netPkt->getNetworkInteractionControl().lastHop = SELF_MAC_ADDRESS;
    return netPkt;
}
