/***************************************************************************
 *  Copyright: National ICT Australia,  2009
 *  Developed at the ATP lab, Networked Systems theme
 *  Author(s): Yuri Tselishchev
 *  This file is distributed under the terms in the attached LICENSE file.
 *  If you do not find this file, copies can be found by writing to:
 *
 *      NICTA, Locked Bag 9013, Alexandria, NSW 1435, Australia
 *      Attention:  License Inquiry.
 ***************************************************************************/

#include "VirtualMobilityModule.h"

Define_Module(VirtualMobilityModule);

void VirtualMobilityModule::initializeLocation() 
{
    wchannel = parentModule()->parentModule()->submodule("wirelessChannel");
    if (!wchannel) opp_error("Unable to obtain wchannel pointer");
                
    nodeLocation.x = parentModule()->par("xCoor");
    nodeLocation.y = parentModule()->par("yCoor");
    nodeLocation.z = parentModule()->par("zCoor");
    nodeLocation.phi = parentModule()->par("phi");
    nodeLocation.theta = parentModule()->par("theta");
}


void VirtualMobilityModule::setLocation(double x, double y, double z, double phi, double theta) 
{
    nodeLocation.x = x;
    nodeLocation.y = y;
    nodeLocation.z = z;
    nodeLocation.phi = phi;
    nodeLocation.theta = theta;
    notifyWirelessChannel();
}

void VirtualMobilityModule::setLocation(NodeLocation_type newLocation) 
{
    nodeLocation = newLocation;
    notifyWirelessChannel();
}

void VirtualMobilityModule::notifyWirelessChannel() 
{
    positionUpdateMsg = new WChannel_NodeMoveMessage("location update message",WC_NODE_MOVEMENT);
    positionUpdateMsg->setX(nodeLocation.x);
    positionUpdateMsg->setY(nodeLocation.y);
    positionUpdateMsg->setZ(nodeLocation.z);
    positionUpdateMsg->setPhi(nodeLocation.phi);
    positionUpdateMsg->setTheta(nodeLocation.theta);
    positionUpdateMsg->setSrcAddress(parentModule()->index());
    sendDirect(positionUpdateMsg,0,wchannel,"fromMobilityModule");
}

NodeLocation_type VirtualMobilityModule::getLocation()
{
    return nodeLocation;
}
