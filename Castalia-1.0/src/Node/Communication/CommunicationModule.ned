// **************************************************************************************
// *  Copyright: National ICT Australia,  2006											*
// *  Developed at the Networks and Pervasive Computing program							*
// *  Author(s): Athanassios Boulis, Dimosthenis Pediaditakis							*
// *  This file is distributed under the terms in the attached LICENSE file.			*
// *  If you do not find this file, copies can be found by writing to:					*
// *																					*
// *      NICTA, Locked Bag 9013, Alexandria, NSW 1435, Australia						*
// *      Attention:  License Inquiry.													*
// *																					*
// **************************************************************************************/



module CommunicationModule

    parameters:
	macModuleName:	string;


    gates:
        out:	toApplicationModule, toNodeContainerModule;
        in: 	fromApplicationModule, fromNodeContainerModule, fromResourceManager2Mac, fromResourceManager2Radio;


    submodules:
	customRadio:	RadioModule;
	customMAC:	macModuleName 	like MacGenericModule;


    connections nocheck:
	fromApplicationModule 		  --> customMAC.fromCommunicationModule;
	customMAC.toCommunicationModule   --> toApplicationModule;

	fromNodeContainerModule 	  --> customRadio.fromCommunicationModule;
	customRadio.toCommunicationModule --> toNodeContainerModule;

	customMAC.toRadioModule 	  --> customRadio.fromMacModule;
	customRadio.toMacModule 	  --> customMAC.fromRadioModule;

	fromResourceManager2Mac  -->  customMAC.fromCommModuleResourceMgr;
	fromResourceManager2Radio -->  customRadio.fromCommModuleResourceMgr;

endmodule
