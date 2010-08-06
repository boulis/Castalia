//********************************************************************************
//*  Copyright: National ICT Australia,  2007 - 2010                             *
//*  Developed at the Networks and Pervasive Computing program                   *
//*  Author(s): Athanassios Boulis, Dimosthenis Pediaditakis, Yuriy Tselishchev  *
//*  This file is distributed under the terms in the attached LICENSE file.      *
//*  If you do not find this file, copies can be found by writing to:            *
//*                                                                              *
//*      NICTA, Locked Bag 9013, Alexandria, NSW 1435, Australia                 *
//*      Attention:  License Inquiry.                                            *
//*                                                                              *
//*******************************************************************************/

package node.communication;

module CommunicationModule {
 parameters:
 	string MACProtocolName = default ("BypassMacModule");
	string RoutingProtocolName = default ("BypassRoutingModule");

 gates:
 	output toApplicationModule;
	output toNodeContainerModule;
	input fromApplicationModule;
	input fromNodeContainerModule;
	input fromResourceManager2Net;
	input fromResourceManager2Mac;
	input fromResourceManager2Radio;

 submodules:
 	Radio: node.communication.Radio.RadioModule;
	MAC: <MACProtocolName> like node.communication.MAC.MacGenericModule;
	Routing: <RoutingProtocolName> like node.communication.Network.NetworkGenericModule;

 connections:
 	fromApplicationModule --> Routing.fromCommunicationModule;
	Routing.toCommunicationModule --> toApplicationModule;
	Routing.toMacModule --> MAC.fromNetworkModule;
	MAC.toNetworkModule --> Routing.fromMacModule;
	MAC.toRadioModule --> Radio.fromMacModule;
	Radio.toMacModule --> MAC.fromRadioModule;
	
	fromNodeContainerModule --> Radio.fromCommunicationModule;
	Radio.toCommunicationModule --> toNodeContainerModule;

	fromResourceManager2Net --> Routing.fromCommModuleResourceMgr;
	fromResourceManager2Mac --> MAC.fromCommModuleResourceMgr;
	fromResourceManager2Radio --> Radio.fromCommModuleResourceMgr;
}

