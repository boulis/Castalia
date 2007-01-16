/************************************************************************************
 *  Copyright: National ICT Australia,  2006										*
 *  Developed at the Networks and Pervasive Computing program						*
 *  Author(s): Athanassios Boulis, Dimosthenis Pediaditakis							*
 *  This file is distributed under the terms in the attached LICENSE file.			*
 *  If you do not find this file, copies can be found by writing to:				*
 *																					*
 *      NICTA, Locked Bag 9013, Alexandria, NSW 1435, Australia						*
 *      Attention:  License Inquiry.												*
 *																					*
 ************************************************************************************/
 
 

#ifndef _APPLICATIONMODULESIMPLE_H_
#define _APPLICATIONMODULESIMPLE_H_

#include <omnetpp.h>
#include <string>
#include "SensorDevMgr_GeneriMessage_m.h"
#include "App_GenericDataPacket_m.h"
#include "MacGenericFrame_m.h"
#include "MacControlMessage_m.h"
#include "ResourceGenericManager.h"
using namespace std;


class ApplicationModuleSimple : public cSimpleModule 
{
	private:
	// parameters and variables
	
	/*--- The .ned file's parameters ---*/
		int priority;
		int maxAppPacketSize;
		int packetHeaderOverhead;
		
	/*--- Custom class parameters ---*/
		int self;	// the node's ID
		double self_xCoo;
		double self_yCoo;
		ResourceGenericManager *resMgrModule;	//a pointer to the object of the Radio Module (used for direct method calls)
		int disabled;
		double cpuClockDrift;
		
		int totalPackets;
		double currMaxReceivedValue;
		double currMaxSensedValue;
		
		cOutVector appVector;	// the vector object to write the statistics
		
		int sentOnce;
		double theValue;
		
	protected:
		virtual void initialize();
		virtual void handleMessage(cMessage *msg);
		virtual void finish();
		
		void send2MacDataPacket(int destID, double data);
		void onReceiveMessage(double receivedData);
		void onSensorReading(int sensorIndex, string sensorType, double sensedValue);

};

#endif // _APPLICATIONMODULESIMPLE_H_
