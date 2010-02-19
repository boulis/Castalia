/********************************************************************************
 *  Copyright: National ICT Australia,  2007 - 2010				*
 *  Developed at the ATP lab, Networked Systems theme				*
 *  Author(s): Athanassios Boulis, Yuriy Tselishchev				*
 *  This file is distributed under the terms in the attached LICENSE file.	*
 *  If you do not find this file, copies can be found by writing to:		*
 *										*
 *      NICTA, Locked Bag 9013, Alexandria, NSW 1435, Australia			*
 *      Attention:  License Inquiry.						*
 ********************************************************************************/


#ifndef VIRTUALAPPLICATIONMODULE
#define VIRTUALAPPLICATIONMODULE

#include <sstream>
#include <string>
#include <omnetpp.h>
#include "ApplicationGenericPacket_m.h"
#include "SensorReadingGenericMessage_m.h"
#include "ResourceGenericManager.h"
#include "RadioModule.h"
#include "VirtualMobilityModule.h"
#include "VirtualCastaliaModule.h"
#include "TimerModule.h"

#define SELF_NETWORK_ADDRESS selfAddress.c_str()

using namespace std;

class VirtualApplicationModule : public VirtualCastaliaModule, public TimerModule {
    protected:
	/*--- The .ned file's parameters ---*/
	string applicationID;
	bool printDebugInfo;
	int priority;
	int maxAppPacketSize;
	int packetHeaderOverhead;
	int constantDataPayload;
	bool isSink;

	/*--- Custom class parameters ---*/
	int self;	// the node's ID
	string selfAddress;
	ResourceGenericManager *resMgrModule;	//a pointer to the object of the Resource Manager Module
	VirtualMobilityModule *mobilityModule;  //a pointer to the mobilityModule object
	RadioModule *radioModule;				//a pointer to the radio module object
	int disabled;
	double cpuClockDrift;

	virtual void initialize();
	virtual void startup() {}
	virtual void handleMessage(cMessage *msg);
	virtual void finish();
	virtual void finishSpecific() {}

	void requestSensorReading(int index = 0);
	void toNetworkLayer(cMessage *);
	void toNetworkLayer(cPacket *, const char*);

	ApplicationGenericDataPacket* createGenericDataPacket(double, int, int = -1);

	virtual void fromNetworkLayer(ApplicationGenericDataPacket *, const char *, double, double) = 0;
	virtual void handleSensorReading(SensorReadingGenericMessage *) {}
	virtual void handleNetworkControlMessage(cMessage *) {}
	virtual void handleMacControlMessage(cMessage *) {}
	virtual void handleRadioControlMessage(RadioControlMessage *);
};

#endif
