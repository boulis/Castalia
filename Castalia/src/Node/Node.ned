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





// The sensor node module. Connects to the wireless channel in order to communicate 
// with other nodes. Connects to psysical processes so it can sample them.

module Node

    parameters:
		xCoor:			const,	// location of the node, x coordinate (set by the Network)
		yCoor: 			const,	// location of the node, y coordinate (set by the Network)
		zCoor: 			const,
		phi:			const,
		theta:			const,
		appModuleName:		string,	//the name of the implemented Application Module
		mobilityModuleName: 	string;	//the name of the implemented Mobility Module


    gates:
        out:	toWirelessChannel, toPhysicalProcess[];
        in:	fromWirelessChannel, fromPhysicalProcess[];


    submodules:
		nodeMobilityModule	: mobilityModuleName like GenericMobilityModule
		nodeResourceMgr		: ResourceGenericManager
		nodeSensorDevMgr	: SensorDevMgrModule
			gatesizes:  
				fromNodeContainerModule[sizeof(toPhysicalProcess)],
				toNodeContainerModule[sizeof(toPhysicalProcess)];
		networkInterface	: CommunicationModule;
		nodeApplication	 	: appModuleName  	 like  ApplicationGenericModule;
	

    connections nocheck:
		networkInterface.toNodeContainerModule  -->  toWirelessChannel;
		fromWirelessChannel  -->  networkInterface.fromNodeContainerModule;
	
		nodeApplication.toCommunicationModule  -->  networkInterface.fromApplicationModule;
		nodeApplication.toSensorDeviceManager  -->  nodeSensorDevMgr.fromApplicationModule;
		networkInterface.toApplicationModule  -->  nodeApplication.fromCommunicationModule;

		nodeSensorDevMgr.toApplicationModule  -->  nodeApplication.fromSensorDeviceManager;
		for i=0..sizeof(toPhysicalProcess)-1 do
			fromPhysicalProcess[i] --> nodeSensorDevMgr.fromNodeContainerModule[i];
			nodeSensorDevMgr.toNodeContainerModule[i] --> toPhysicalProcess[i];
	        endfor;

		nodeResourceMgr.toSensorDevManager  -->  nodeSensorDevMgr.fromResourceManager;
		nodeResourceMgr.toApplication -->  nodeApplication.fromResourceManager;
		nodeResourceMgr.toNetwork --> networkInterface.fromResourceManager2Net;
		nodeResourceMgr.toMac  -->  networkInterface.fromResourceManager2Mac;
		nodeResourceMgr.toRadio  -->  networkInterface.fromResourceManager2Radio;
endmodule
