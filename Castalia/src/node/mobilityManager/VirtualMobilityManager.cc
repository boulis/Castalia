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

#include "VirtualMobilityManager.h"

Define_Module(VirtualMobilityManager);

void VirtualMobilityManager::initialize()
{
	cModule *node = getParentModule();
	cModule *network = node->getParentModule();
	wchannel = network->getSubmodule("wirelessChannel");
	if (!wchannel)
		opp_error("Unable to obtain wchannel pointer");

	int deploymentType = network->par("deploymentType");
	int index = node->getIndex();

	int field_x = network->par("field_x");
	int field_y = network->par("field_y");
	int field_z = network->par("field_z");
	int xGridSize = network->par("xGridSize");
	int yGridSize = network->par("yGridSize");
	int zGridSize = network->par("zGridSize");

	nodeLocation.phi = node->par("phi");
	nodeLocation.theta = node->par("theta");

	if (deploymentType == 0) {
		nodeLocation.x = uniform(0, field_x);
		nodeLocation.y = uniform(0, field_y);
		nodeLocation.z = uniform(0, field_z);
	} else if (deploymentType == 1) {
		nodeLocation.x = (index % xGridSize) * (field_x / (xGridSize - 1));
		nodeLocation.y = ((int)floor(index / xGridSize) % yGridSize) * 
				(field_y / (yGridSize - 1));
		if (zGridSize < 2 || field_z < 2) {
			nodeLocation.z = 1;
		} else {
			nodeLocation.z = ((int)floor(index / (xGridSize * yGridSize)) % zGridSize) * 
					(field_z / (zGridSize - 1));
		}
	} else if (deploymentType == 2) {
		double randomFactor = network->par("randomFactor");
		nodeLocation.x = (index % xGridSize) * (field_x / (xGridSize - 1)) +
				normal(0, (field_x / (xGridSize - 1)) * randomFactor);
		nodeLocation.y = ((int)floor(index / xGridSize) % yGridSize) * (field_y / 
				(yGridSize - 1)) + normal(0, (field_y / (yGridSize - 1)) * randomFactor);
		if (zGridSize < 2 || field_z < 2) {
			nodeLocation.z = 1;
		} else {
			nodeLocation.z = ((int)floor(index / (xGridSize * yGridSize)) % zGridSize) * 
					(field_z / (zGridSize - 1)) + normal(0, (field_z / (zGridSize - 1)) * randomFactor);
		}
	} else {
		nodeLocation.x = node->par("xCoor");
		nodeLocation.y = node->par("yCoor");
		nodeLocation.z = node->par("zCoor");
	}

	trace() << "initial location(x:y:z) is " << nodeLocation.x << ":" << 
			nodeLocation.y << ":" << nodeLocation.z;
}

void VirtualMobilityManager::setLocation(double x, double y, double z, double phi, double theta)
{
	nodeLocation.x = x;
	nodeLocation.y = y;
	nodeLocation.z = z;
	nodeLocation.phi = phi;
	nodeLocation.theta = theta;
	notifyWirelessChannel();
}

void VirtualMobilityManager::setLocation(NodeLocation_type newLocation)
{
	nodeLocation = newLocation;
	notifyWirelessChannel();
}

void VirtualMobilityManager::notifyWirelessChannel()
{
	positionUpdateMsg =
	    new WirelessChannelNodeMoveMessage("location update message", WC_NODE_MOVEMENT);
	positionUpdateMsg->setX(nodeLocation.x);
	positionUpdateMsg->setY(nodeLocation.y);
	positionUpdateMsg->setZ(nodeLocation.z);
	positionUpdateMsg->setPhi(nodeLocation.phi);
	positionUpdateMsg->setTheta(nodeLocation.theta);
	positionUpdateMsg->setNodeID(getParentModule()->getIndex());
	sendDirect(positionUpdateMsg, wchannel, "fromMobilityModule");
}

NodeLocation_type VirtualMobilityManager::getLocation()
{
	return nodeLocation;
}

