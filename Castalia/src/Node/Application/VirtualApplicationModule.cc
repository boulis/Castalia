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


#include "VirtualApplicationModule.h"

void VirtualApplicationModule::initialize() {

	//get a valid reference to the object of the Resources Manager module so that we can make direct calls to its public methods
	//instead of using extra messages & message types for tighlty couplped operations.
	cModule *parent = getParentModule();
	self = parent->getIndex();
	if(parent->findSubmodule("nodeResourceMgr") != -1) {
	    resMgrModule = check_and_cast<ResourceGenericManager*>(parent->getSubmodule("nodeResourceMgr"));
	} else {
	    opp_error("\n[Application]:\n Error in geting a valid reference to nodeResourceMgr for direct method calls.");
	}
	
	if(parent->findSubmodule("nodeMobilityModule") != -1) {
	    mobilityModule = check_and_cast<VirtualMobilityModule*>(parent->getSubmodule("nodeMobilityModule"));
	} else {
	    opp_error("\n[Application]:\n Error in geting a valid reference to nodeMobilityModule for direct method calls.");
	}
	
	cpuClockDrift = resMgrModule->getCPUClockDrift();
	setTimerDrift(cpuClockDrift);
	disabled = 1;

	stringstream out;
	out << self;
	selfAddress = out.str();
	broadcastAddress = "-1";
	sinkAddress = "0";
	
	applicationID = par("applicationID").stringValue();
	priority = par("priority");
	packetHeaderOverhead = par("packetHeaderOverhead");
	constantDataPayload = par("constantDataPayload");
	isSink = hasPar("isSink") ? par("isSink") : false;

	// Send the STARTUP message to MAC & to Sensor_Manager modules so that the node start to operate. (after a random delay, because we don't want the nodes to be synchronized)
	double random_startup_delay = genk_dblrand(0) * 0.05;
	sendDelayed(new cMessage("Application --> Sensor Dev Mgr [STARTUP]", NODE_STARTUP), simTime() + cpuClockDrift*random_startup_delay, "toSensorDeviceManager");
	sendDelayed(new cMessage("Application --> Network [STARTUP]", NODE_STARTUP), simTime() + cpuClockDrift*random_startup_delay, "toCommunicationModule");
	scheduleAt(simTime() + cpuClockDrift*random_startup_delay, new cMessage("Application --> Application (self)", NODE_STARTUP));
}

void VirtualApplicationModule::handleMessage(cMessage *msg) {
    int msgKind = msg->getKind();

    if(disabled && msgKind != NODE_STARTUP) {
	delete msg;
	msg = NULL;		// safeguard
	return;
    }

    switch (msgKind) {
	/*--------------------------------------------------------------------------------------------------------------
	 * Sent by ourself in order to start running the Application.
	 *--------------------------------------------------------------------------------------------------------------*/
	case NODE_STARTUP: {
	    disabled = 0;
	    startup();
	    break;
	}

	case APPLICATION_PACKET: {
	    ApplicationGenericDataPacket *rcvPacket;
	    rcvPacket = check_and_cast<ApplicationGenericDataPacket *>(msg);
	    ApplicationInteractionControl_type control = rcvPacket->getApplicationInteractionControl();
	    if (applicationID.compare(control.applicationID.c_str()) == 0) 
		fromNetworkLayer(rcvPacket,control.source.c_str(),control.pathFromSource.c_str(),control.RSSI,control.LQI);
	    break;
	}

	case TIMER_SERVICE: {
	    handleTimerMessage(msg);
	    break;
	}

	case SENSOR_READING_MESSAGE: {
	    SensorReadingGenericMessage *sensMsg = check_and_cast<SensorReadingGenericMessage*>(msg);
	    handleSensorReading(sensMsg);
	    break;
	}

	case OUT_OF_ENERGY: {
	    disabled = 1;
	    break;
	}
	
	case DESTROY_NODE: {
	    disabled = 1;
	    break;
	}
	
	case NETWORK_CONTROL_MESSAGE: {
	    handleNetworkControlMessage(msg);
	    break;
	}
	
	case MAC_CONTROL_MESSAGE: {
	    handleMacControlMessage(msg);
	    break;
	}
	
	case RADIO_CONTROL_MESSAGE: {
	    handleRadioControlMessage(msg);
	    break;
	}
	
	default: {
	    opp_error("Application module recieved unexpected message");
	}
    }

    delete msg;
    msg = NULL;		// safeguard
}

void VirtualApplicationModule::finish() {
    finishSpecific();
    VirtualCastaliaModule::finish();
    DebugInfoWriter::closeStream();
}

ApplicationGenericDataPacket *VirtualApplicationModule::createGenericDataPacket(double data, int seqNum, int size) {
    ApplicationGenericDataPacket *newPacket = new ApplicationGenericDataPacket("Application generic packet", APPLICATION_PACKET);
    newPacket->setData(data);
    newPacket->setSequenceNumber(seqNum);
    if (size < 0) size = constantDataPayload;
    if (size < 0) opp_error("cannot create generic data packet of negative size");
    size += packetHeaderOverhead;
    newPacket->setByteLength(size);
    return newPacket;
}

void VirtualApplicationModule::requestSensorReading(int index) {
    // send the request message to the Sensor Device Manager
    SensorReadingGenericMessage *reqMsg;
    reqMsg = new SensorReadingGenericMessage("app 2 sensor dev manager (Sample request)", SENSOR_READING_MESSAGE);
    reqMsg->setSensorIndex(index); //we need the index of the vector in the sensorTypes vector to distinguish the self messages for each sensor
    send(reqMsg, "toSensorDeviceManager");

}

void VirtualApplicationModule::toNetworkLayer(cMessage *msg) {
    if (msg->getKind() == APPLICATION_PACKET) opp_error("toNetworkLayer() function used incorrectly to send APPLICATION_PACKET without destination Network address");
    send(msg, "toCommunicationModule");
}

void VirtualApplicationModule::toNetworkLayer(cPacket *pkt, const char * dst) {
    ApplicationGenericDataPacket *appPkt = check_and_cast<ApplicationGenericDataPacket*>(pkt);
    appPkt->getApplicationInteractionControl().destination = string(dst);
    appPkt->getApplicationInteractionControl().applicationID = applicationID;
    send(appPkt, "toCommunicationModule");
}
