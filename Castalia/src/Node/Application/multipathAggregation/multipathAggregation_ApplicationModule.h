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
 
 

#ifndef _MULTIPATHAGGREGATION_APPLICATIONMODULE_H_
#define _MULTIPATHAGGREGATION_APPLICATIONMODULE_H_

#include <omnetpp.h>
#include <string>
#include <vector>
#include "SensorDevMgr_GenericMessage_m.h"

/*** 
     You have to MODIFY the following "#define" statement by placing the
     header file (*_m.h) that is produced from you custom *.msg message definition file
 ***/
#include "multipathAggregation_DataPacket_m.h"

#include "App_GenericDataPacket_m.h"
#include "MacGenericFrame_m.h"
#include "MacControlMessage_m.h"
#include "ResourceGenericManager.h"
using namespace std;

/*
struct neighborRec
{
	int id;
	int isParent;
	int timesTxTo;
	int timesRxFrom;
	double lastValue;
	time_t lastRcv;
};
*/

class multipathAggregation_ApplicationModule : public cSimpleModule 
{
	private:
	// parameters and variables
	
	/*--- The .ned file's parameters ---*/
		int priority;
		int maxAppPacketSize;
		int packetHeaderOverhead;
		
		int sinkID;
		/*double tableUpdateRate;*/
		double epoch;
		
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
		int inInitializationPhase;
		int isSink;
		int treeLevel;
		int isInitialized;
		/*vector <neighborRec> neighborTable;*/
		double lastSensedValue;
		double initPhase_time_delay;
		
	protected:
		virtual void initialize();
		virtual void handleMessage(cMessage *msg);
		virtual void finish();
		
		void send2MacDataPacket(int destID, double data, int level, int pckSeqNumber);
		void send2MacInitPacket(int destID, int initiatorID, int level, int parentID, int pckSeqNumber);
		void requestSampleFromSensorManager();

		/**
		   ADD HERE YOUR CUSTOM protected MEMBER VARIABLES AND FUNCTIONS
		 **/
		void bootStrapInitPhase();
		void updateNeighborTable();
		
	public:
		/**
		   ADD HERE YOUR CUSTOM public MEMBER VARIABLES AND FUNCTIONS
		 **/
};

#endif // _MULTIPATHAGGREGATION_APPLICATIONMODULE_H_
