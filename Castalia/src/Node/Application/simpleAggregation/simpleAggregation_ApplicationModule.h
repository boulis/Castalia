//***************************************************************************************
//*  Copyright: National ICT Australia,  2007, 2008, 2009, 2010				*
//*  Developed at the Networks and Pervasive Computing program				*
//*  Author(s): Athanassios Boulis, Dimosthenis Pediaditakis, Yuriy Tselishchev		*
//*  This file is distributed under the terms in the attached LICENSE file.		*
//*  If you do not find this file, copies can be found by writing to:			*
//*											*
//*      NICTA, Locked Bag 9013, Alexandria, NSW 1435, Australia			*
//*      Attention:  License Inquiry.							*
//*											*
//***************************************************************************************

#ifndef _SIMPLEAGGREGATION_APPLICATIONMODULE_H_
#define _SIMPLEAGGREGATION_APPLICATIONMODULE_H_

#include "VirtualApplicationModule.h"

using namespace std;

enum Timers {
    REQUEST_SAMPLE = 1,
    SEND_AGGREGATED_VALUE = 2
};

class simpleAggregation_ApplicationModule : public VirtualApplicationModule
{
	private:
	     double aggregatedValue;
	     int routingLevel;
	     double waitingTimeForLowerLevelData;
	     double lastSensedValue;
	     double sampleInterval;
	     int totalPackets;
	
	protected:
	    void startup();
	    void timerFiredCallback(int);
	    void handleSensorReading(SensorReadingGenericMessage*);
	    void handleNeworkControlMessage(cMessage*);
	    void fromNetworkLayer(ApplicationGenericDataPacket*, const char*, double, double);
};

#endif // _SIMPLEAGGREGATION_APPLICATIONMODULE_H_
