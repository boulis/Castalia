/****************************************************************************
 *  Copyright: National ICT Australia,  2007 - 2010                         *
 *  Developed at the Networks and Pervasive Computing program               *
 *  Author(s): Yuriy Tselishchev                                            *
 *  This file is distributed under the terms in the attached LICENSE file.  *
 *  If you do not find this file, copies can be found by writing to:        *
 *                                                                          *
 *      NICTA, Locked Bag 9013, Alexandria, NSW 1435, Australia             *
 *      Attention:  License Inquiry.                                        *
 *                                                                          *  
 ****************************************************************************/

#include "BypassMAC.h"

Define_Module(BypassMAC);

/* We handle packet received from upper (network) layer. 
 * Here we need to create a packet, specific for this particular MAC (BypassMacPacket used)
 * and encapsulate the received network packet before forwarding it to RadioLayer
 */
void BypassMAC::fromNetworkLayer(cPacket * pkt, int destination)
{
	//First step is creating a wrapper packet. Note that MAC_DATA_FRAME is used as message kind. This is generic
	//constant defined in CastaliaMessages.h to ensure that all Castalia modules are working with a single set
	//of unique message kinds
	BypassMacPacket *macFrame =
	    new BypassMacPacket("BypassRouting packet", MAC_LAYER_PACKET);
	encapsulatePacket(macFrame, pkt);
	macFrame->setSource(SELF_MAC_ADDRESS);
	macFrame->setDestination(destination);
	toRadioLayer(macFrame);
	toRadioLayer(createRadioCommand(SET_STATE, TX));
}

/* We handle packet received from lower (radio) layer. Values of RSSI and LQI are ignored.
 * Since we only sent BypassMACSimpleFrame packets to the radio, we can assume that we will only 
 * receive the same type of packet back. However for various reasons (e.g. if someone decides to use 
 * two or more different MAC protocols in a single simulation we will use dynamic_cast here and 
 * only decapsulate and forward packets of type BypassMACSimpleFrame.
 */
void BypassMAC::fromRadioLayer(cPacket * pkt, double rssi, double lqi)
{
	BypassMacPacket *macPkt = dynamic_cast <BypassMacPacket*>(pkt);
	if (macPkt == NULL)
		return;
	if (macPkt->getDestination() == SELF_MAC_ADDRESS ||
	    macPkt->getDestination() == BROADCAST_MAC_ADDRESS) {
		toNetworkLayer(decapsulatePacket(macPkt));
	}
}

