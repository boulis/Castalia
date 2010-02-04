//***************************************************************************************
//*  Copyright: National ICT Australia,  2009, 2010					*
//*  Developed at the Networks and Pervasive Computing program				*
//*  Author(s): Yuri Tselishchev							*
//*  This file is distributed under the terms in the attached LICENSE file.		*
//*  If you do not find this file, copies can be found by writing to:			*
//*											*
//*      NICTA, Locked Bag 9013, Alexandria, NSW 1435, Australia			*
//*      Attention:  License Inquiry.							*
//*											*
//***************************************************************************************


#include "BridgeTest_ApplicationModule.h"


#define MEMORY_PROFILE_FREE(numBytes) resMgrModule->RamFree(numBytes)

#define NO_ROUTING_LEVEL -1

#define REPORT_PACKET "Report packet"
#define VERSION_PACKET "Version packet"

Define_Module(BridgeTest_ApplicationModule);

void BridgeTest_ApplicationModule::startup() {

    reportTreshold = par("reportTreshold");
    sampleInterval = (double)par("sampleInterval")/1000;
    broadcastReports = par("broadcastReports");
    reprogramInterval = par("reprogramInterval");
    reprogramPacketDelay = (double)par("reprogramPacketDelay")/1000;
    reprogramPayload = par("reprogramPayload");
    sampleSize = par("sampleSize");

    currentVersionPacket = 0;
    currentVersion = 0;
    currSampleSN = 1;
    outOfEnergy = 0;
    currentSampleAccumulated = 0;

    maxPayload = (int)par("maxAppPacketSize")-(int)par("packetHeaderOverhead");
    div_t tmp_div = div(reprogramPayload,maxPayload);
    totalVersionPackets = tmp_div.rem > 0 ? tmp_div.quot + 1 : tmp_div.quot;
    tmp_div = div(maxPayload,sampleSize);
    maxSampleAccumulated = tmp_div.quot * sampleSize;

    version_info_table.clear();
    report_info_table.clear();
    
    if (isSink) {
	setTimer(REPROGRAM_NODES,10); //add initial delay parameter
    } else {
	setTimer(REQUEST_SAMPLE,sampleInterval);
    }
}

void BridgeTest_ApplicationModule::timerFiredCallback(int timer) {
    switch (timer) {
    
	case REQUEST_SAMPLE: {
	    setTimer(REQUEST_SAMPLE,sampleInterval);
	    requestSensorReading();
	    break;
	}
	
	case REPROGRAM_NODES: {
	    currentVersion++;
	    currentVersionPacket = 1;
	    setTimer(REPROGRAM_NODES,reprogramInterval);
	    setTimer(SEND_REPROGRAM_PACKET,0);
	    break;
	}
	
	case SEND_REPROGRAM_PACKET: {
	    ApplicationGenericDataPacket *newPkt = createGenericDataPacket(currentVersion,currentVersionPacket,maxPayload);
	    toNetworkLayer(newPkt,BROADCAST_NETWORK_ADDRESS);
	    currentVersionPacket++;
	    if (currentVersionPacket < totalVersionPackets) setTimer(SEND_REPROGRAM_PACKET,reprogramPacketDelay);
	    break;
	}
    }
}

void BridgeTest_ApplicationModule::fromNetworkLayer(ApplicationGenericDataPacket* rcvPacket, const char * source, const char * path, double rssi, double lqi) {
    string packetType(""); //rcvPacket->getHeader().applicationID.c_str());
    
    int versionData = rcvPacket->getData();
    int sequenceNumber = rcvPacket->getSequenceNumber();

    if (packetType.compare(REPORT_PACKET) == 0) {
	// this is report packet which contains sensor reading information
	if (isSink || broadcastReports) {
	    if (updateReportTable(atoi(source),sequenceNumber) && broadcastReports) {
		// forward the packet only if we broadcast reports and this is a new (unseen) report
		// updateReportTable returns 0 for duplicate packets
		toNetworkLayer(rcvPacket->dup(),BROADCAST_NETWORK_ADDRESS);
	    }
	}

    } else if (packetType.compare(VERSION_PACKET) == 0) {
	// this is version (reprogramming) packet
	if (!isSink && updateVersionTable(versionData,sequenceNumber)) {
	    // forward the packet only if not sink and its a new packet 
	    // updateVersionTable returns 0 for duplicate packets
	    toNetworkLayer(rcvPacket->dup(),BROADCAST_NETWORK_ADDRESS);
	}

    } else {
        trace() << "unknown packet type received " << packetType;
    }
}

void BridgeTest_ApplicationModule::handleSensorReading(SensorReadingGenericMessage * sensorMsg) {
    string sensType(sensorMsg->getSensorType());
    double sensValue = sensorMsg->getSensedValue();

    if (isSink) {
	trace() << "Sink recieved SENSOR_READING (while it shouldnt) " << sensValue << " (int)"<<(int)sensValue;
	return;
    }

    if (sensValue < reportTreshold) return;

    currentSampleAccumulated += sampleSize;
    if (currentSampleAccumulated < maxSampleAccumulated) return;

    if (broadcastReports)
	toNetworkLayer(createGenericDataPacket(sensValue,currSampleSN,currentSampleAccumulated),BROADCAST_NETWORK_ADDRESS); 
    else
	toNetworkLayer(createGenericDataPacket(sensValue,currSampleSN,currentSampleAccumulated),SINK_NETWORK_ADDRESS);

    currSampleSN++;
    currentSampleAccumulated = 0;
}

void BridgeTest_ApplicationModule::finishSpecific() {
    // output the spent energy of the node
    EV <<  "Node [" << self << "] spent energy: " << resMgrModule->getSpentEnergy() << "\n";
    if (isSink) {
	EV << "Sink is at version: " << currentVersion << " packet: " << currentVersionPacket << " total: " << totalVersionPackets << "\n";
	EV << "Sink received from:\n";
	for (int i=0; i<(int)report_info_table.size(); i++) {
	    declareOutput("Report reception",report_info_table[i].source);
	    collectOutput("Report reception",report_info_table[i].source,"Success",report_info_table[i].parts.size());
	    collectOutput("Report reception",report_info_table[i].source,"Fail",report_info_table[i].seq-report_info_table[i].parts.size());
	    EV << report_info_table[i].source << " " << report_info_table[i].parts.size() << "\n";
	}
    } else {
	if (outOfEnergy > 0) EV << "Node "<<self<<" ran out of energy at "<<outOfEnergy<<"\n";
	EV << "Node "<<self<<" sent "<<currSampleSN<<" packets\n";
	EV << "Node "<<self<<" received version information:\n";
	declareOutput("Reporgram reception");
        for (int i=0; i<(int)version_info_table.size(); i++) {
	    EV << version_info_table[i].version << " " << version_info_table[i].parts.size() << "\n";
	    collectOutput("Reporgram reception","Success",version_info_table[i].parts.size());
	    collectOutput("Reporgram reception","Fail",version_info_table[i].seq-version_info_table[i].parts.size());
        }
    }
}

int BridgeTest_ApplicationModule::updateReportTable(int src, int seq) {
    int pos = -1;
    for (int i=0; i<(int)report_info_table.size(); i++) {
	if (report_info_table[i].source == src) pos = i;
    }

    if (pos == -1) {
	report_info newInfo;
	newInfo.source = src;
	newInfo.parts.clear();
	newInfo.parts.push_back(seq);
	newInfo.seq = seq;
	report_info_table.push_back(newInfo);
    } else {
	for (int i=0; i<(int)report_info_table[pos].parts.size(); i++) {
	    if (report_info_table[pos].parts[i] == seq) return 0;
	}
	report_info_table[pos].parts.push_back(seq);
	if (seq > report_info_table[pos].seq) {
	    report_info_table[pos].seq = seq;
	}
    }
    return 1;
}

int BridgeTest_ApplicationModule::updateVersionTable(int version, int seq) {
    int pos = -1;
    for (int i=0; i<(int)version_info_table.size(); i++) {
	if (version_info_table[i].version == version) pos = i;
    }

    if (pos == -1) {
	version_info newInfo;
	newInfo.version = version;
	newInfo.parts.clear();
	newInfo.parts.push_back(seq);
	newInfo.seq = seq;
	version_info_table.push_back(newInfo);
    } else {
	for (int i=0; i<(int)version_info_table[pos].parts.size(); i++) {
	    if (version_info_table[pos].parts[i] == seq) return 0;
	}
	version_info_table[pos].parts.push_back(seq);
	if (seq > version_info_table[pos].seq) {
	    version_info_table[pos].seq = seq;
	}
	
    }
    return 1;
}

