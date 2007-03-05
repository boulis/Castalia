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
 
 

#ifndef _IDEA_APPLICATIONMODULE_H_
#define _IDEA_APPLICATIONMODULE_H_

#include <omnetpp.h>
#include <string>
#include "Idea.h"
#include "SensorDevMgr_GenericMessage_m.h"
#include "Idea_DataPacket_m.h"
#include "App_GenericDataPacket_m.h"
#include "MacGenericFrame_m.h"
#include "MacControlMessage_m.h"
#include "ResourceGenericManager.h"
using namespace std;


class Idea_ApplicationModule : public cSimpleModule 
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
		
		double currMaxReceivedValue;
		double currMaxSensedValue;
		
		cOutVector appVector;	// the vector object to write the statistics
		
		
		/***  Idea Specific ***/
		
		double sensorVariance;
		estim nodeGlobalEstimation;
		simtime_t globalEstimTimeStamp;
		double previousReadingWasClose;
		estim threshold;
		vector<NeighborRecord> neighTable;
		int minimumNeighNum;	// minimum number of neighbours to start deciding to send the new global estim. or not
					// if our neighbours are less TX anyway
		double sampleInterval;
		double initEnergy;
		int totalBroadcasts;
		int totalPacketsReceived;
		cOutVector valuesVector;
		cOutVector estimatesVector;
		cOutVector RXVector;
		
	protected:
		virtual void initialize();
		virtual void handleMessage(cMessage *msg);
		virtual void finish();
		
		void send2MacDataPacket(int destID, estim data, int pckSeqNumber);
		void requestSampleFromSensorManager();

		void onReceiveMessage(int senderID, estim receivedGlobalEstim);
		void onSensorReading(int sensorIndex, string sensorType, double sensedValue);

};

#endif // _IDEA_APPLICATIONMODULE_H_
