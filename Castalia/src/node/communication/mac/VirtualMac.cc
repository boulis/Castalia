/****************************************************************************
 *  Copyright: National ICT Australia,  2007 - 2010                         *
 *  Developed at the ATP lab, Networked Systems research theme              *
 *  Author(s): Yuriy Tselishchev                                            *
 *  This file is distributed under the terms in the attached LICENSE file.  *
 *  If you do not find this file, copies can be found by writing to:        *
 *                                                                          *
 *      NICTA, Locked Bag 9013, Alexandria, NSW 1435, Australia             *
 *      Attention:  License Inquiry.                                        *
 *                                                                          *  
 ****************************************************************************/

#include "VirtualMac.h"

void VirtualMac::initialize()
{
	macBufferSize = par("macBufferSize");
	macFrameOverhead = par("macPacketOverhead");
	macMaxFrameSize = par("macMaxPacketSize");
	self = getParentModule()->getParentModule()->getIndex();

	//get a valid reference to the object of the Radio module so that we can make 
	//direct calls to its public methods instead of using extra messages & message 
	//types for tighlty couplped operations.
	radioModule = check_and_cast <Radio*>(gate("toRadioModule")->getNextGate()->getOwnerModule());

	//get a valid reference to the object of the Resources Manager module so that 
	//we can make direct calls to its public methods instead of using extra 
	//messages & message types for tighlty couplped operations.
	if (getParentModule()->getParentModule()->findSubmodule("ResourceManager") != -1) {
		resMgrModule = check_and_cast <ResourceManager*>
			(getParentModule()->getParentModule()->getSubmodule("ResourceManager"));
	} else {
		opp_error("\n[Mac]:\n Error in geting a valid reference to ResourceManager for direct method calls.");
	}
	setTimerDrift(resMgrModule->getCPUClockDrift());
	disabled = 1;
	//declareOutput("Buffer overflow");
}

int VirtualMac::handleControlCommand(cMessage * msg)
{
	trace() << "WARNING: handleControlCommand not defined in this module";
	return 0;
}

int VirtualMac::handleRadioControlMessage(cMessage * msg)
{
	toNetworkLayer(msg);
	return 1;
}

int VirtualMac::bufferPacket(cPacket * rcvFrame)
{
	if ((int)TXBuffer.size() >= macBufferSize) {
		//collectOutput("Buffer overflow");  this is handled in specific MACs
		cancelAndDelete(rcvFrame);
		// send a control message to the upper layer
		MacControlMessage *fullBuffMsg =
		    new MacControlMessage("MAC buffer full", MAC_CONTROL_MESSAGE);
		fullBuffMsg->setMacControlMessageKind(MAC_BUFFER_FULL);
		send(fullBuffMsg, "toNetworkModule");
		return 0;
	} else {
		TXBuffer.push(rcvFrame);
		trace() << "Packet buffered from network layer, buffer state: "
		    << TXBuffer.size() << "/" << macBufferSize;
		return 1;
	}
}

void VirtualMac::handleMessage(cMessage * msg)
{

	int msgKind = (int)msg->getKind();

	if (disabled && msgKind != NODE_STARTUP) {
		delete msg;
		msg = NULL;	// safeguard
		return;
	}

	switch (msgKind) {

		case NODE_STARTUP:{
			disabled = 0;
			startup();
			break;
		}

		case NETWORK_LAYER_PACKET:{
			RoutingPacket *pkt = check_and_cast <RoutingPacket*>(msg);
			if (macMaxFrameSize > 0 && macMaxFrameSize < pkt->getByteLength() + macFrameOverhead) {
				trace() << "Oversized packet dropped. Size:" << pkt->getByteLength() << 
						", MAC layer overhead:" << macFrameOverhead << 
						", max MAC frame size:" << macMaxFrameSize;
				break;
			}
			// trace() << "Received [" << pkt->getName() << "] from Network layer";
			fromNetworkLayer(pkt, pkt->getRoutingInteractionControl().nextHop);
			return;
		}

		case TIMER_SERVICE:{
			handleTimerMessage(msg);
			break;
		}

		case MAC_LAYER_PACKET:{
			MacPacket *pkt = check_and_cast <MacPacket*>(msg);
			// trace() << "Received [" << pkt->getName() << "] from Radio layer";
			fromRadioLayer(pkt, pkt->getMacInteractionControl().RSSI,
								pkt->getMacInteractionControl().LQI);
			break;
		}

		case OUT_OF_ENERGY:{
			disabled = 1;
			break;
		}

		case MAC_CONTROL_COMMAND:{
			if (handleControlCommand(msg))
				return;
			break;
		}

		case RADIO_CONTROL_COMMAND:{
			toRadioLayer(msg);
			return;
		}

		case RADIO_CONTROL_MESSAGE:{
			if (handleRadioControlMessage(msg))
				return;
			break;
		}

		default:{
			opp_error("MAC module received message of unknown kind %i", msgKind);
		}
	}

	delete msg;
	msg = NULL;		// safeguard
}

void VirtualMac::finish()
{
	CastaliaModule::finish();
	while (!TXBuffer.empty()) {
		cancelAndDelete(TXBuffer.front());
		TXBuffer.pop();
	}
}

void VirtualMac::toNetworkLayer(cMessage * macMsg)
{
	trace() << "Delivering [" << macMsg->getName() << "] to Network layer";
	send(macMsg, "toNetworkModule");
}

void VirtualMac::toRadioLayer(cMessage * macMsg)
{
	send(macMsg, "toRadioModule");
}

void VirtualMac::encapsulatePacket(cPacket * macPkt, cPacket * netPkt)
{
	macPkt->setByteLength(macFrameOverhead);
	macPkt->setKind(MAC_LAYER_PACKET);
	macPkt->encapsulate(netPkt);
}

cPacket *VirtualMac::decapsulatePacket(cPacket * pkt)
{
	MacPacket *macPkt = check_and_cast <MacPacket*>(pkt);
	RoutingPacket *netPkt = check_and_cast <RoutingPacket*>(macPkt->decapsulate());
	netPkt->getRoutingInteractionControl().RSSI =
	    	macPkt->getMacInteractionControl().RSSI;
	netPkt->getRoutingInteractionControl().LQI =
	    	macPkt->getMacInteractionControl().LQI;
	netPkt->getRoutingInteractionControl().lastHop = SELF_MAC_ADDRESS;
	return netPkt;
}

