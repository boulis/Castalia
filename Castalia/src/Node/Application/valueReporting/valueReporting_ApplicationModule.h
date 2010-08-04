/****************************************************************************
 *  Copyright: National ICT Australia,  2007 - 2010                         *
 *  Developed at the Networks and Pervasive Computing program               *
 *  Author(s): Dimosthenis Pediaditakis, Yuriy Tselishchev                  *
 *  This file is distributed under the terms in the attached LICENSE file.  *
 *  If you do not find this file, copies can be found by writing to:        *
 *                                                                          *
 *      NICTA, Locked Bag 9013, Alexandria, NSW 1435, Australia             *
 *      Attention:  License Inquiry.                                        *
 *                                                                          *  
 ****************************************************************************/

#ifndef _VALUEREPORTING_APPLICATIONMODULE_H_
#define _VALUEREPORTING_APPLICATIONMODULE_H_

#include "VirtualApplicationModule.h"
#include "valueReporting_DataPacket_m.h"

using namespace std;

enum Timers {
	REQUEST_SAMPLE = 1,
	SEND_DATA = 2,
};

class valueReporting_ApplicationModule:public VirtualApplicationModule {
 private:
	double maxSampleInterval;
	double minSampleInterval;

	int routingLevel;
	double lastSensedValue;
	int currSentSampleSN;

	double randomBackoffIntervalFraction;
	bool sentOnce;

 protected:
	void startup();
	void fromNetworkLayer(ApplicationGenericDataPacket *, const char *, double, double);
	void handleSensorReading(SensorReadingGenericMessage *);
	void timerFiredCallback(int);
};

#endif				// _VALUEREPORTING_APPLICATIONMODULE_H_
