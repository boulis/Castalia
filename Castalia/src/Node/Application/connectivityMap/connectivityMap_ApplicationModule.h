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
 
 

#ifndef _CONNECTIVITYMAP_APPLICATIONMODULE_H_
#define _CONNECTIVITYMAP_APPLICATIONMODULE_H_

#include <omnetpp.h>
#include <string>
#include <vector>
#include "SensorDevMgr_GenericMessage_m.h"
#include "connectivityMap_DataPacket_m.h"
#include "App_GenericDataPacket_m.h"
#include "MacGenericFrame_m.h"
#include "MacControlMessage_m.h"
#include "ResourceGenericManager.h"
using namespace std;

struct neighborRecord
{
	int id;
	int timesRx;
};


class connectivityMap_ApplicationModule : public cSimpleModule 
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
		
		cOutVector appVector;	// the vector object to write the statistics
		
		/**
		   ADD HERE YOUR CUSTOM private MEMBER VARIABLES AND FUNCTIONS
		 **/
		vector <neighborRecord> neighborTable;
		int packetsSent;
		double last_sensed_value;
		int totalSNnodes;
		double totalSimTime;
		double txInterval;
		int currentNodeTx;
		
	protected:
		virtual void initialize();
		virtual void handleMessage(cMessage *msg);
		virtual void finish();
		
		void send2MacDataPacket(int destID, double data, int pckSeqNumber);
		void requestSampleFromSensorManager();
		
		/**
		   ADD HERE YOUR CUSTOM protected MEMBER VARIABLES AND FUNCTIONS
		 **/
		void updateNeighborTable(int nodeID);

	public:
		/**
		   ADD HERE YOUR CUSTOM public MEMBER VARIABLES AND FUNCTIONS
		 **/
};

#endif // _CONNECTIVITYMAP_APPLICATIONMODULE_H_
