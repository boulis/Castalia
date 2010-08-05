/*******************************************************************************
 *  Copyright: National ICT Australia,  2007 - 2010                            *
 *  Developed at the Networks and Pervasive Computing program                  *
 *  Author(s): Athanassios Boulis, Dimosthenis Pediaditakis, Yuriy Tselishchev *
 *  This file is distributed under the terms in the attached LICENSE file.     *
 *  If you do not find this file, copies can be found by writing to:           *
 *                                                                             *
 *      NICTA, Locked Bag 9013, Alexandria, NSW 1435, Australia                *
 *      Attention:  License Inquiry.                                           *
 *                                                                             *  
 *******************************************************************************/

#include "valuePropagation_ApplicationModule.h"

Define_Module(valuePropagation_ApplicationModule);

void valuePropagation_ApplicationModule::startup()
{
	tempTreshold = par("tempTreshold");
	totalPackets = 0;
	currMaxReceivedValue = -1.0;
	currMaxSensedValue = -1.0;
	sentOnce = 0;
	theValue = 0;
	setTimer(REQUEST_SAMPLE, 0);
}

void valuePropagation_ApplicationModule::timerFiredCallback(int index)
{
	switch (index) {
		case REQUEST_SAMPLE:{
			requestSensorReading();
			break;
		}
	}
}

void valuePropagation_ApplicationModule::fromNetworkLayer(ApplicationGenericDataPacket * rcvPacket, 
		const char *source, double rssi, double lqi)
{
	double receivedData = rcvPacket->getData();

	totalPackets++;
	if (receivedData > currMaxReceivedValue)
		currMaxReceivedValue = receivedData;

	if (receivedData > tempTreshold && !sentOnce) {
		theValue = receivedData;
		toNetworkLayer(createGenericDataPacket(receivedData, 1), BROADCAST_NETWORK_ADDRESS);
		sentOnce = 1;
		trace() << "Got the value: " << theValue;
	}
}

void valuePropagation_ApplicationModule::handleSensorReading(SensorReadingGenericMessage * rcvReading)
{
	double sensedValue = rcvReading->getSensedValue();

	if (sensedValue > currMaxSensedValue)
		currMaxSensedValue = sensedValue;

	if (sensedValue > tempTreshold && !sentOnce) {
		theValue = sensedValue;
		toNetworkLayer(createGenericDataPacket(sensedValue, 1), BROADCAST_NETWORK_ADDRESS);
		sentOnce = 1;
	}
}

void valuePropagation_ApplicationModule::finishSpecific()
{
	declareOutput("Value");
	collectOutput("Value", "Celcius", theValue);
}

