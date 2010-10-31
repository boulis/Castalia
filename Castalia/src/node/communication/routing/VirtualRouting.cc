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
 
#include "VirtualRouting.h"
void VirtualRouting::initialize()
{
	maxNetFrameSize = par("maxNetFrameSize");
	netDataFrameOverhead = par("netDataFrameOverhead");
	netBufferSize = par("netBufferSize");

	self = getParentModule()->getParentModule()->getIndex();

	//get a valid reference to the object of the Radio module so that we can make direct 
	//calls to its public methods instead of using extra messages & message types 
	//for tighlty couplped operations.
	radioModule = check_and_cast <Radio*>(gate("toMacModule")->getNextGate()->getOwnerModule()->gate("toRadioModule")->getNextGate()->getOwnerModule());

	//get a valid reference to the object of the Resources Manager module so that
	//we can make direct calls to its public methods instead of using extra messages 
	//& message types for tighlty couplped operations.
	cModule *parentParent = getParentModule()->getParentModule();
	if (parentParent->findSubmodule("ResourceManager") != -1) {
		resMgrModule = check_and_cast <ResourceManager*>(parentParent->getSubmodule("ResourceManager"));
	} else {
		opp_error("\n[Network]:\n Error in geting a valid reference to ResourceManager for direct method calls.");
	}
	cpuClockDrift = resMgrModule->getCPUClockDrift();
	setTimerDrift(cpuClockDrift);
	pktHistory.clear();
	
	disabled = 1;
	currentSequenceNumber = 0;
	stringstream out;
	out << self;
	selfAddress = out.str();
	declareOutput("Buffer overflow");
}

void VirtualRouting::toMacLayer(cMessage * msg)
{
	if (msg->getKind() == NETWORK_LAYER_PACKET)
		opp_error("toMacLayer() function used incorrectly to send NETWORK_LAYER_PACKET without destination MAC address");
	send(msg, "toMacModule");
}

void VirtualRouting::toMacLayer(cPacket * pkt, int destination)
{
	RoutingPacket *netPacket = check_and_cast <RoutingPacket*>(pkt);
	netPacket->getRoutingInteractionControl().nextHop = destination;
	send(netPacket, "toMacModule");
}

void VirtualRouting::toApplicationLayer(cMessage * msg)
{
	send(msg, "toCommunicationModule");
}

void VirtualRouting::encapsulatePacket(cPacket * pkt, cPacket * appPkt)
{
	RoutingPacket *netPkt = check_and_cast <RoutingPacket*>(pkt);
	netPkt->setByteLength(netDataFrameOverhead);
	netPkt->setKind(NETWORK_LAYER_PACKET);
	netPkt->getRoutingInteractionControl().source = SELF_NETWORK_ADDRESS;
	netPkt->getRoutingInteractionControl().sequenceNumber = currentSequenceNumber++;
	netPkt->encapsulate(appPkt);
}

cPacket *VirtualRouting::decapsulatePacket(cPacket * pkt)
{
	RoutingPacket *netPkt = check_and_cast <RoutingPacket*>(pkt);
	ApplicationGenericDataPacket *appPkt = check_and_cast <ApplicationGenericDataPacket*>(netPkt->decapsulate());

	appPkt->getApplicationInteractionControl().RSSI = netPkt->getRoutingInteractionControl().RSSI;
	appPkt->getApplicationInteractionControl().LQI = netPkt->getRoutingInteractionControl().LQI;
	appPkt->getApplicationInteractionControl().source = netPkt->getRoutingInteractionControl().source;
	return appPkt;
}

void VirtualRouting::handleMessage(cMessage * msg)
{
	int msgKind = msg->getKind();
	if (disabled && msgKind != NODE_STARTUP) {
		delete msg;
		msg = NULL;	// safeguard
		return;
	}

	switch (msgKind) {

	/*--------------------------------------------------------------------------------------------------------------
	 * Sent by the Application submodule in order to start/switch-on the Network submodule.
	 *--------------------------------------------------------------------------------------------------------------*/
		case NODE_STARTUP:{
			disabled = 0;
			send(new cMessage("Network --> Mac startup message", NODE_STARTUP), "toMacModule");
			startup();
			break;
		}

	/*--------------------------------------------------------------------------------------------------------------
	 * Sent by the Application submodule. We need to push it into the buffer and schedule 
	 * its forwarding to the MAC buffer for TX.
	 *--------------------------------------------------------------------------------------------------------------*/
		case APPLICATION_PACKET:{
			ApplicationGenericDataPacket *appPacket = check_and_cast <ApplicationGenericDataPacket*>(msg);
			if (maxNetFrameSize > 0 && maxNetFrameSize < appPacket->getByteLength() + netDataFrameOverhead) {
				trace() << "Oversized packet dropped. Size:" << appPacket->getByteLength() <<
				    ", Network layer overhead:" << netDataFrameOverhead << 
				    ", max Network packet size:" << maxNetFrameSize;
				break;
			}
			trace() << "Received [" << appPacket->getName() << "] from application layer";
			fromApplicationLayer(appPacket, appPacket->getApplicationInteractionControl().destination.c_str());	
			//fromApplicationLayer() now has control of the packet, we use return here to avoid deleting
			//it. This is done since the packet will most likely be encapsulated and forwarded to MAC layer.
			//An alternative way would be to use dup() here, but this will not change things for
			//fromApplicationLayer() function (i.e. it will need to delete the packet if its not forwarded)
			//but will create an unneeded packet duplication call here
			return;
		}

	/*--------------------------------------------------------------------------------------------------------------
	 * Data Frame Received from the MAC layer
	 *--------------------------------------------------------------------------------------------------------------*/
		case NETWORK_LAYER_PACKET:{
			RoutingPacket *netPacket = check_and_cast <RoutingPacket*>(msg);
			trace() << "Received [" << netPacket->getName() << "] from MAC layer";
			RoutingInteractionControl_type control = netPacket->getRoutingInteractionControl();
			fromMacLayer(netPacket, control.lastHop, control.RSSI, control.LQI);
			//although we passed the message control to function fromMacLayer(), we still use break here 
			//instead of return. This is to allow fromMacLayer function to worry only about encapsulated
			//packet, making sure that the network layer packet gets deleted anyway
			break;
		}

		case TIMER_SERVICE:{
			handleTimerMessage(msg);
			break;
		}

		case MAC_CONTROL_MESSAGE:{
			handleMacControlMessage(msg);
			return;
		}

		case RADIO_CONTROL_MESSAGE:{
			handleRadioControlMessage(msg);
			return;
		}

		case MAC_CONTROL_COMMAND:{
			toMacLayer(msg);
			return;
		}

		case RADIO_CONTROL_COMMAND:{
			toMacLayer(msg);
			return;
		}

		case NETWORK_CONTROL_COMMAND:{
			handleNetworkControlCommand(msg);
			break;
		}

	/*--------------------------------------------------------------------------------------------------------------
	 * Message sent by the Resource Manager when battery is out of energy.
	 * Node has to shut down.
	 *--------------------------------------------------------------------------------------------------------------*/
		case OUT_OF_ENERGY:{
			disabled = 1;
			break;
		}

	/*--------------------------------------------------------------------------------------------------------------
	 * Message received by the ResourceManager module. It commands the module to stop its operation.
	 *--------------------------------------------------------------------------------------------------------------*/
		case DESTROY_NODE:{
			disabled = 1;
			break;
		}

		default:{
			opp_error("Network module recieved unexpected message: [%s]", msg->getName());
		}
	}

	delete msg;
}

// handleMacControlMessage needs to either process and DELETE the message OR forward it
void VirtualRouting::handleMacControlMessage(cMessage * msg)
{
	toApplicationLayer(msg);
}

// handleRadioControlMessage needs to either process and DELETE the message OR forward it
void VirtualRouting::handleRadioControlMessage(cMessage * msg)
{
	toApplicationLayer(msg);
}

void VirtualRouting::finish()
{
	CastaliaModule::finish();
	cPacket *pkt;
	while (!TXBuffer.empty()) {
		pkt = TXBuffer.front();
		TXBuffer.pop();
		cancelAndDelete(pkt);
	}
}

int VirtualRouting::bufferPacket(cPacket * rcvFrame)
{
	if ((int)TXBuffer.size() >= netBufferSize) {
		collectOutput("Buffer overflow");
		cancelAndDelete(rcvFrame);
		return 0;
	} else {
		TXBuffer.push(rcvFrame);
		trace() << "Packet buffered from application layer, buffer state: " <<
		    TXBuffer.size() << "/" << netBufferSize;
		return 1;
	}
}

int VirtualRouting::resolveNetworkAddress(const char *netAddr)
{
	if (!netAddr[0] || netAddr[0] < '0' || netAddr[0] > '9')
		return BROADCAST_MAC_ADDRESS;
	return atoi(netAddr);
}

bool VirtualRouting::isNotDuplicatePacket(cPacket * pkt) 
{
	//extract source address and sequence number from the packet
	RoutingPacket *netPkt = check_and_cast <RoutingPacket*>(pkt);
	int src = resolveNetworkAddress(netPkt->getRoutingInteractionControl().source.c_str());
	int sn = netPkt->getRoutingInteractionControl().sequenceNumber;

	//resize packet history vector if necessary
	if (src >= (int)pktHistory.size())
		pktHistory.resize(src+1);
		
	//search for this sequence number in the list, corresponding to address 'src'
	list<int>::iterator it1;
	for (it1 = pktHistory[src].begin(); it1 != pktHistory[src].end(); it1++) {
		//if such sequence number is found, packet is duplicate
		if (*it1 == sn) return false;
	}
	
	//no such sequence number found, this is new packet
	pktHistory[src].push_front(sn);
	if (pktHistory[src].size() > PACKET_HISTORY_SIZE)
		pktHistory[src].pop_back();
	return true;
}
