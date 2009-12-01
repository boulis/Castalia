//***************************************************************************************
//*  Copyright: National ICT Australia,  2007, 2008, 2009				*
//*  Developed at the Networks and Pervasive Computing program				*
//*  Author(s): Athanassios Boulis, Dimosthenis Pediaditakis				*
//*  This file is distributed under the terms in the attached LICENSE file.		*
//*  If you do not find this file, copies can be found by writing to:			*
//*											*
//*      NICTA, Locked Bag 9013, Alexandria, NSW 1435, Australia			*
//*      Attention:  License Inquiry.							*
//*											*
//***************************************************************************************


package Node.Communication;

module CommunicationModule

{
    parameters:
        string macModuleName = default("BypassMacModule");
        string networkModuleName = default("BypassRoutingModule");

    gates:
        output toApplicationModule;
        output toNodeContainerModule;
        input fromApplicationModule;
        input fromNodeContainerModule;
        input fromResourceManager2Net;
        input fromResourceManager2Mac;
        input fromResourceManager2Radio;

    submodules:
        Radio: Node.Communication.Radio.RadioModule;
        MAC: <macModuleName> like Node.Communication.MAC.MacGenericModule;
        Network: <networkModuleName> like Node.Communication.Network.NetworkGenericModule;

    connections:
        fromApplicationModule --> Network.fromCommunicationModule;
        Network.toCommunicationModule --> toApplicationModule;

        Network.toMacModule --> MAC.fromNetworkModule;
        MAC.toNetworkModule --> Network.fromMacModule;

        MAC.toRadioModule --> Radio.fromMacModule;
        Radio.toMacModule --> MAC.fromRadioModule;

        fromNodeContainerModule --> Radio.fromCommunicationModule;
        Radio.toCommunicationModule --> toNodeContainerModule;

        fromResourceManager2Net --> Network.fromCommModuleResourceMgr;
        fromResourceManager2Mac --> MAC.fromCommModuleResourceMgr;
        fromResourceManager2Radio --> Radio.fromCommModuleResourceMgr;

}
