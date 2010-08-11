/*******************************************************************************
 *  Copyright: National ICT Australia,  2007 - 2010                            *
 *  Developed at the ATP lab, Networked Systems research theme                 *
 *  Author(s): Athanassios Boulis, Dimosthenis Pediaditakis, Yuriy Tselishchev *
 *  This file is distributed under the terms in the attached LICENSE file.     *
 *  If you do not find this file, copies can be found by writing to:           *
 *                                                                             *
 *      NICTA, Locked Bag 9013, Alexandria, NSW 1435, Australia                *
 *      Attention:  License Inquiry.                                           *
 *                                                                             *  
 *******************************************************************************/

#include "BypassRouting.h"

Define_Module(BypassRouting);

//Application layer sends a packet together with a network layer address 'destination'
//Network layer is responsible to route that packet by selecting an appropriate MAC address
//to forward that packet to. We will encapsulate the received packet into a network packet
//and use BROADCAST_MAC_ADDRESS for all transmissions since this module does no routing
//NOTE: function toMacLayer requires second argument (a MAC address) if first argument is a packet
void BypassRouting::fromApplicationLayer(cPacket * pkt, const char *destination)
{
	BypassRoutingPacket *netPacket = new BypassRoutingPacket("BypassRouting packet", NETWORK_LAYER_PACKET);
	//NETWORK_LAYER_PACKET is a generic Castalia message kind to identify network layer packets

	netPacket->setSource(SELF_NETWORK_ADDRESS);
	netPacket->setDestination(destination);
	encapsulatePacket(netPacket, pkt);
	toMacLayer(netPacket, resolveNetworkAddress(destination));
}

//MAC layer sends a packet up to network also providing source MAC address 
//which will allow network module to create a routing table.
//Here we dont check what source MAC address is, just filter incoming packets 
//by network addresses
//NOTE: use of dynamic_cast is similar to that in BypassMacModule
void BypassRouting::fromMacLayer(cPacket * pkt, int srcMacAddress, double rssi, double lqi)
{
	BypassRoutingPacket *netPacket = dynamic_cast <BypassRoutingPacket*>(pkt);
	if (netPacket) {
		string destination(netPacket->getDestination());
		if (destination.compare(SELF_NETWORK_ADDRESS) == 0 ||
		    destination.compare(BROADCAST_NETWORK_ADDRESS) == 0)
			toApplicationLayer(decapsulatePacket(pkt));
	}
}

