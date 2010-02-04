//***************************************************************************************
//*  Copyright: Athens Information Technology (AIT),  2007, 2008, 2009			*
//*		http://www.ait.gr							*
//*             Developed at the Broadband Wireless and Sensor Networks group (B-WiSe) 	*
//*		http://www.ait.edu.gr/research/Wireless_and_Sensors/overview.asp	*
//*											*
//*  Author(s): Dimosthenis Pediaditakis						*
//*											*
//*  This file is distributed under the terms in the attached LICENSE file.		*
//*  If you do not find this file, copies can be found by writing to:			*
//*											*
//*      NICTA, Locked Bag 9013, Alexandria, NSW 1435, Australia			*
//*      Attention:  License Inquiry.							*
//**************************************************************************************/

#include "valueReporting_ApplicationModule.h"


Define_Module(valueReporting_ApplicationModule);

void valueReporting_ApplicationModule::startup() {

    maxSampleInterval = ((double)par("maxSampleInterval"))/1000.0;
    minSampleInterval = ((double)par("minSampleInterval"))/1000.0;

    routingLevel = (isSink)?0:NO_ROUTING_LEVEL;
    lastSensedValue = 0.0;
    currSentSampleSN = 0;

    randomBackoffIntervalFraction = genk_dblrand(0);
    sentOnce = false;

    setTimer(REQUEST_SAMPLE,maxSampleInterval*randomBackoffIntervalFraction);
}

void valueReporting_ApplicationModule::timerFiredCallback(int index) {
    switch (index) {
	case REQUEST_SAMPLE: {
	    requestSensorReading();
	    setTimer(REQUEST_SAMPLE,maxSampleInterval);
	    break;
	}
	
	case SEND_DATA: {
	    if (routingLevel != NO_ROUTING_LEVEL && !isSink) {
		
		valueReportData tmpData;
		tmpData.nodeID = (unsigned short)self;
		tmpData.locX = mobilityModule->getLocation().x;
		tmpData.locY = mobilityModule->getLocation().y;
		tmpData.sampleSN = currSentSampleSN;
		tmpData.value = lastSensedValue;
		
		valueReporting_DataPacket *packet2Net = new valueReporting_DataPacket("Application Packet Application->Mac", APPLICATION_PACKET);

		packet2Net->setExtraData(tmpData);
		packet2Net->setByteLength(constantDataPayload + packetHeaderOverhead);
		
		toNetworkLayer(packet2Net,SINK_NETWORK_ADDRESS);
		sentOnce = true;
	    }
	}
    }
}

void valueReporting_ApplicationModule::fromNetworkLayer(ApplicationGenericDataPacket* genericPacket, const char * source, const char* path, double rssi, double lqi) {
    valueReporting_DataPacket *rcvPacket = check_and_cast<valueReporting_DataPacket*>(genericPacket);

    valueReportData theData = rcvPacket->getExtraData();

    if(isSink) trace() << "Sink received from:" << theData.nodeID << " \tvalue=" << theData.value;
}

void valueReporting_ApplicationModule::handleSensorReading(SensorReadingGenericMessage * rcvReading) {
    // int sensIndex =  rcvReading->getSensorIndex();
    string sensType(rcvReading->getSensorType());
    double sensValue = rcvReading->getSensedValue();

    lastSensedValue = sensValue;

    // schedule the TX of the value
    setTimer(SEND_DATA,0);
    trace() << "Sensed = " << sensValue;
}
