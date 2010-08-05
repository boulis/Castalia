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

package node;

import node.mobilityManager;

// The sensor node module. Connects to the wireless channel in order to communicate 
// with other nodes. Connects to psysical processes so it can sample them.

module Node {
 parameters:
	double xCoor = default (0);
	double yCoor = default (0);
	double zCoor = default (0);
	double phi = default (0);
	double theta = default (0);

	string appModuleName;	//the name of the implemented Application Module
	string mobilityModuleName = default ("NoMobilityManager");	//the name of the implemented Mobility Module

 gates:
	output toWirelessChannel;
	output toPhysicalProcess[];
	input fromWirelessChannel;
	input fromPhysicalProcess[];

 submodules:
 	nodeMobilityModule:<mobilityModuleName> like node.mobilityManager.iMobilityManager;
	nodeResourceMgr:node.Resource_Manager.ResourceGenericManager;
	nodeSensorDevMgr:node.Sensor_Device_Manager.SensorDevMgrModule {
	 gates:
		fromNodeContainerModule[sizeof(toPhysicalProcess)];
		toNodeContainerModule[sizeof(toPhysicalProcess)];
	} 
	networkInterface:node.Communication.CommunicationModule;
	nodeApplication:<appModuleName> like node.application.iApplication;

 connections:
	networkInterface.toNodeContainerModule --> toWirelessChannel;
	fromWirelessChannel --> networkInterface.fromNodeContainerModule;
	nodeApplication.toCommunicationModule --> networkInterface.fromApplicationModule;
	nodeApplication.toSensorDeviceManager --> nodeSensorDevMgr.fromApplicationModule;
	networkInterface.toApplicationModule --> nodeApplication.fromCommunicationModule;
	nodeSensorDevMgr.toApplicationModule --> nodeApplication.fromSensorDeviceManager;

	for i = 0..sizeof(toPhysicalProcess) - 1 {
		fromPhysicalProcess[i] --> nodeSensorDevMgr.fromNodeContainerModule[i];
		nodeSensorDevMgr.toNodeContainerModule[i] --> toPhysicalProcess[i];
	}

	nodeResourceMgr.toSensorDevManager --> nodeSensorDevMgr.fromResourceManager;
	nodeResourceMgr.toApplication --> nodeApplication.fromResourceManager;
	nodeResourceMgr.toNetwork --> networkInterface.fromResourceManager2Net;
	nodeResourceMgr.toMac --> networkInterface.fromResourceManager2Mac;
	nodeResourceMgr.toRadio --> networkInterface.fromResourceManager2Radio;
}

