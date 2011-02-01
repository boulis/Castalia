/****************************************************************************
 *  Copyright: National ICT Australia,  2007 - 2010                         *
 *  Developed at the ATP lab, Networked Systems research theme              *
 *  Author(s): Athanassios Boulis, Yuriy Tselishchev                        *
 *  This file is distributed under the terms in the attached LICENSE file.  *
 *  If you do not find this file, copies can be found by writing to:        *
 *                                                                          *
 *      NICTA, Locked Bag 9013, Alexandria, NSW 1435, Australia             *
 *      Attention:  License Inquiry.                                        *
 *                                                                          *  
 ****************************************************************************/

#include "VirtualApplication.h"

void VirtualApplication::initialize()
{
	/* Get a valid references to the objects of the Resources Manager module
	 * the Mobility module and the radio module, so that we can make direct
	 * calls to their public methods
	 */
	cModule *parent = getParentModule();

	if (parent->findSubmodule("ResourceManager") != -1) {
		resMgrModule = check_and_cast <ResourceManager*>(parent->getSubmodule("ResourceManager"));
	} else {
		opp_error("\n[Application]:\n Error in geting a valid reference to ResourceManager for direct method calls.");
	}

	if (parent->findSubmodule("MobilityManager") != -1) {
		mobilityModule = check_and_cast <VirtualMobilityManager*>(parent->getSubmodule("MobilityManager"));
	} else {
		opp_error("\n[Application]:\n Error in geting a valid reference to MobilityManager for direct method calls.");
	}

	// we make no checks here
	radioModule = check_and_cast <Radio*>(parent->getSubmodule("Communication")->getSubmodule("Radio"));

	self = parent->getIndex();
	cpuClockDrift = resMgrModule->getCPUClockDrift();
	setTimerDrift(cpuClockDrift);
	disabled = 1;

	stringstream out;
	out << self;
	selfAddress = out.str();

	applicationID = par("applicationID").stringValue();
	priority = par("priority");
	packetHeaderOverhead = par("packetHeaderOverhead");
	constantDataPayload = par("constantDataPayload");
	isSink = hasPar("isSink") ? par("isSink") : false;

	double startup_delay = parent->par("startupOffset");
	// Randomize the delay if the startupRandomization is non-zero
	startup_delay += genk_dblrand(0) * (double)parent->par("startupRandomization");

        /* Send the STARTUP message to 1)Sensor_Manager, 2)Commmunication module,
         * 3) Resource Manager, and $)APP (self message) so that the node starts
         * operation. Note that we send the message to the Resource Mgr through
	 * the unconnected gate "powerConsumption" using sendDirect()
         */
	sendDelayed(new cMessage("Sensor Dev Mgr [STARTUP]", NODE_STARTUP),
		    simTime() +  startup_delay, "toSensorDeviceManager");
	sendDelayed(new cMessage("Communication [STARTUP]", NODE_STARTUP),
		    simTime() +  startup_delay, "toCommunicationModule");
	sendDirect(new cMessage("Resource Mgr [STARTUP]", NODE_STARTUP),
		    startup_delay, 0, resMgrModule, "powerConsumption");
	scheduleAt(simTime() + startup_delay, new cMessage("App [STARTUP]", NODE_STARTUP));

	/* Latency measurement is optional. An application can not define the 
	 * following two parameters. If they are not defined then the
	 * declareHistogram and collectHistogram statement are not called.
	 */
	latencyMax = hasPar("latencyHistogramMax") ? par("latencyHistogramMax") : 0;
	latencyBuckets = hasPar("latencyHistogramBuckets") ? par("latencyHistogramBuckets") : 0;
	if (latencyMax > 0 && latencyBuckets > 0)
		declareHistogram("Application level latency, in ms", 0, latencyMax, latencyBuckets);
}

void VirtualApplication::handleMessage(cMessage * msg)
{
	int msgKind = msg->getKind();

	if (disabled && msgKind != NODE_STARTUP) {
		delete msg;
		msg = NULL;	// safeguard
		return;
	}

	switch (msgKind) {

		case NODE_STARTUP:
		{
			disabled = 0;
			startup();
			break;
		}

		case APPLICATION_PACKET:
		{
			ApplicationGenericDataPacket *rcvPacket;
			rcvPacket = check_and_cast <ApplicationGenericDataPacket*>(msg);
			ApplicationInteractionControl_type control = rcvPacket->getApplicationInteractionControl();
			// We deliver only packets that have the correct appID. Unless 
			// the appID is the empty string, in that case it will get all packets
			if (applicationID.compare(control.applicationID.c_str()) == 0 || applicationID.compare("") == 0) {
				if (latencyMax > 0 && latencyBuckets > 0)
					collectHistogram("Application level latency, in ms",
					1000 * SIMTIME_DBL(simTime() - control.timestamp));
				fromNetworkLayer(rcvPacket, control.source.c_str(), control.RSSI, control.LQI);
			}
			break;
		}

		case TIMER_SERVICE:
		{
			handleTimerMessage(msg);
			break;
		}

		case SENSOR_READING_MESSAGE:
		{
			SensorReadingMessage *sensMsg = check_and_cast <SensorReadingMessage*>(msg);
			handleSensorReading(sensMsg);
			break;
		}

		case OUT_OF_ENERGY:
		{
			disabled = 1;
			break;
		}

		case DESTROY_NODE:
		{
			disabled = 1;
			break;
		}

		case NETWORK_CONTROL_MESSAGE:
		{
			handleNetworkControlMessage(msg);
			break;
		}

		case MAC_CONTROL_MESSAGE:
		{
			handleMacControlMessage(msg);
			break;
		}

		case RADIO_CONTROL_MESSAGE:
		{
			RadioControlMessage *radioMsg = check_and_cast <RadioControlMessage*>(msg);
			handleRadioControlMessage(radioMsg);
			break;
		}

		default:
		{
			opp_error("Application module received unexpected message");
		}
	}

	delete msg;
	msg = NULL;		// safeguard
}

void VirtualApplication::finish()
{
	CastaliaModule::finish();
	DebugInfoWriter::closeStream();
}

ApplicationGenericDataPacket *VirtualApplication::createGenericDataPacket(double data, int seqNum, int size)
{
	ApplicationGenericDataPacket *newPacket =
	    new ApplicationGenericDataPacket("Application generic packet", APPLICATION_PACKET);
	newPacket->setData(data);
	newPacket->setSequenceNumber(seqNum);
	if (size > 0) newPacket->setByteLength(size);
	return newPacket;
}

void VirtualApplication::requestSensorReading(int index)
{
	// send the request message to the Sensor Device Manager
	SensorReadingMessage *reqMsg =
		new SensorReadingMessage("app 2 sensor dev manager (Sample request)", SENSOR_READING_MESSAGE);

	//we need the index of the vector in the sensorTypes vector
	//to distinguish the self messages for each sensor
	reqMsg->setSensorIndex(index);

	send(reqMsg, "toSensorDeviceManager");
}

void VirtualApplication::toNetworkLayer(cMessage * msg)
{
	if (msg->getKind() == APPLICATION_PACKET)
		opp_error("toNetworkLayer() function used incorrectly to send APPLICATION_PACKET without destination Network address");
	send(msg, "toCommunicationModule");
}

void VirtualApplication::toNetworkLayer(cPacket * pkt, const char *dst)
{
	ApplicationGenericDataPacket *appPkt = check_and_cast <ApplicationGenericDataPacket*>(pkt);
	appPkt->getApplicationInteractionControl().destination = string(dst);
	appPkt->getApplicationInteractionControl().source = string(SELF_NETWORK_ADDRESS);
	appPkt->getApplicationInteractionControl().applicationID = applicationID;
	appPkt->getApplicationInteractionControl().timestamp = simTime();
	int size = appPkt->getByteLength();
	if (size == 0) size = constantDataPayload;
	if (packetHeaderOverhead > 0) size += packetHeaderOverhead;
	trace() << "Sending [" << appPkt->getName() << "] of size " << 
		size << " bytes to communication layer";
	appPkt->setByteLength(size);
	send(appPkt, "toCommunicationModule");
}

