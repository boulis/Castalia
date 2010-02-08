//***************************************************************************************
//*  Copyright: National ICT Australia, 2007 - 2010					*
//*  Developed at the Networks and Pervasive Computing program				*
//*  Author(s): Dimosthenis Pediaditakis, Yuriy Tselishchev				*
//*  This file is distributed under the terms in the attached LICENSE file.		*
//*  If you do not find this file, copies can be found by writing to:			*
//*											*
//*      NICTA, Locked Bag 9013, Alexandria, NSW 1435, Australia			*
//*      Attention:  License Inquiry.							*
//*											*
//***************************************************************************************

#include "valueReporting_ApplicationModule.h"


Define_Module(valueReporting_ApplicationModule);

void valueReporting_ApplicationModule::startup() {
    maxSampleInterval = ((double)par("maxSampleInterval"))/1000.0;
    minSampleInterval = ((double)par("minSampleInterval"))/1000.0;
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
    }
}

void valueReporting_ApplicationModule::fromNetworkLayer(ApplicationGenericDataPacket* genericPacket, const char * source, double rssi, double lqi) {
    valueReporting_DataPacket *rcvPacket = check_and_cast<valueReporting_DataPacket*>(genericPacket);
    valueReportData theData = rcvPacket->getExtraData();
    if(isSink) trace() << "Sink received from: " << theData.nodeID << " \tvalue=" << rcvPacket->getData();
}

void valueReporting_ApplicationModule::handleSensorReading(SensorReadingGenericMessage * rcvReading) {
    // int sensIndex =  rcvReading->getSensorIndex();
    // string sensType(rcvReading->getSensorType());
    double sensValue = rcvReading->getSensedValue();

    // schedule the TX of the value
    trace() << "Sensed = " << sensValue;

    valueReportData tmpData;
    tmpData.nodeID = (unsigned short)self;
    tmpData.locX = mobilityModule->getLocation().x;
    tmpData.locY = mobilityModule->getLocation().y;

    valueReporting_DataPacket *packet2Net = new valueReporting_DataPacket("Application Packet Application->Mac", APPLICATION_PACKET);
    packet2Net->setExtraData(tmpData);
    packet2Net->setData(sensValue);
    packet2Net->setSequenceNumber(currSentSampleSN);
    currSentSampleSN++;

    packet2Net->setByteLength(constantDataPayload + packetHeaderOverhead);
    toNetworkLayer(packet2Net,SINK_NETWORK_ADDRESS);
    sentOnce = true;
}
