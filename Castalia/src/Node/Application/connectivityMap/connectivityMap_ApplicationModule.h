//***************************************************************************************
//*  Copyright: National ICT Australia,  2007, 2008, 2009				*
//*  Developed at the Networks and Pervasive Computing program				*
//*  Author(s): Athanassios Boulis, Dimosthenis Pediaditakis				*
//*  This file is distributed under the terms in the attached LICENSE file.		*
//*  If you do not find this file, copies can be found by writing to:			*
//*											*
//*      NICTA, Locked Bag 9013, Alexandria, NSW 1435, Australia			*
//*      Attention:  License Inquiry.							*
//*											*
//***************************************************************************************



#ifndef _CONNECTIVITYMAP_APPLICATIONMODULE_H_
#define _CONNECTIVITYMAP_APPLICATIONMODULE_H_

#include <omnetpp.h>
#include <string>
#include <vector>
#include "SensorDevMgr_GenericMessage_m.h"
#include "connectivityMap_DataPacket_m.h"
#include "App_GenericDataPacket_m.h"
#include "App_ControlMessage_m.h"
#include "NetworkControlMessage_m.h"
#include "ResourceGenericManager.h"
#include "DebugInfoWriter.h"
using namespace std;


struct neighborRecord
{
	int id;
	int timesRx;
	int receivedPackets;
};


class connectivityMap_ApplicationModule : public cSimpleModule
{
	private:
	// parameters and variables

	/*--- The .ned file's parameters ---*/
		string applicationID;
		bool printDebugInfo;
		int priority;
		int maxAppPacketSize;
		int packetHeaderOverhead;
		bool printConnMap;
		int constantDataPayload;

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
		int serialNumber;
		int totalSNnodes;
		double totalSimTime;
		int numTxPowerLevels;
		char txPowerLevelNames[15][7];
		double txInterval_perNode;
		double txInterval_perPowerLevel;
		int currentPowerLevel;

	protected:
		virtual void initialize();
		virtual void handleMessage(cMessage *msg);
		virtual void finish();

		void send2NetworkDataPacket(const char *destID, int data, int pckSeqNumber);
		void requestSampleFromSensorManager();


		/************ Header file for Declaration of Functions of TunableMAC set functions  ******************************/
		// If you are not going to use the TunableMAC module, then you can comment the following line and build Castalia
		// the following includes are located at ../commonIncludeFiles
		#include "radioControl.h"
		#include "tunableMacControl.h"
		/************ Connectivity Map Definitions  ************************************************/


		/**
		   ADD HERE YOUR CUSTOM protected MEMBER VARIABLES AND FUNCTIONS
		 **/
		void updateNeighborTable(int nodeID, int theSN);

	public:
		/**
		   ADD HERE YOUR CUSTOM public MEMBER VARIABLES AND FUNCTIONS
		 **/
};

#endif // _CONNECTIVITYMAP_APPLICATIONMODULE_H_
