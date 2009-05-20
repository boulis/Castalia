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




module CommunicationModule

    parameters:
	macModuleName:	string,
	networkModuleName: string;


    gates:
        out:	toApplicationModule, toNodeContainerModule;
        in: 	fromApplicationModule, fromNodeContainerModule, fromResourceManager2Net, fromResourceManager2Mac, fromResourceManager2Radio;


    submodules:
	Radio:	 RadioModule;
	MAC:	 macModuleName 	like MacGenericModule;
	Network: networkModuleName like NetworkGenericModule


    connections nocheck:
	fromApplicationModule	--> Network.fromCommunicationModule;
	Network.toCommunicationModule	--> toApplicationModule;

	Network.toMacModule	--> MAC.fromNetworkModule;
	MAC.toNetworkModule	--> Network.fromMacModule;

	MAC.toRadioModule 	  --> Radio.fromMacModule;
	Radio.toMacModule 	  --> MAC.fromRadioModule;

	fromNodeContainerModule	--> Radio.fromCommunicationModule;
	Radio.toCommunicationModule --> toNodeContainerModule;

	fromResourceManager2Net	 -->  Network.fromCommModuleResourceMgr;
	fromResourceManager2Mac  -->  MAC.fromCommModuleResourceMgr;
	fromResourceManager2Radio -->  Radio.fromCommModuleResourceMgr;
	
endmodule
