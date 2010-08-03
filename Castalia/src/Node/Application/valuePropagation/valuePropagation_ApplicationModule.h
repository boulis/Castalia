//***************************************************************************************
//*  Copyright: National ICT Australia,  2007, 2008, 2009, 2010                         *
//*  Developed at the Networks and Pervasive Computing program                          *
//*  Author(s): Athanassios Boulis, Dimosthenis Pediaditakis, Yuriy Tselishchev         *
//*  This file is distributed under the terms in the attached LICENSE file.             *
//*  If you do not find this file, copies can be found by writing to:                   *
//*                                                                                     *
//*      NICTA, Locked Bag 9013, Alexandria, NSW 1435, Australia                        *
//*      Attention:  License Inquiry.                                                   *
//*                                                                                     *
//***************************************************************************************

#ifndef _VALUEPROPAGATION_APPLICATIONMODULE_H_
#define _VALUEPROPAGATION_APPLICATIONMODULE_H_

#include "VirtualApplicationModule.h"

using namespace std;

enum Timers {
	REQUEST_SAMPLE = 1,
};

class valuePropagation_ApplicationModule: public VirtualApplicationModule {
 private:
	int totalPackets;
	double currMaxReceivedValue;
	double currMaxSensedValue;
	int sentOnce;
	double theValue;
	double tempTreshold;
	vector<double> sensedValues;

 protected:
	void startup();
	void finishSpecific();
	void fromNetworkLayer(ApplicationGenericDataPacket *, const char *, double, double);
	void handleSensorReading(SensorReadingGenericMessage *);
	void timerFiredCallback(int);
};

#endif				// _VALUEPROPAGATION_APPLICATIONMODULE_H_
