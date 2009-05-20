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

 
 

#ifndef _VALUEPROPAGATION_APPLICATIONMODULE_H_
#define _VALUEPROPAGATION_APPLICATIONMODULE_H_

#include <omnetpp.h>
#include <string>
#include <vector>
#include "SensorDevMgr_GenericMessage_m.h"
#include "valuePropagation_DataPacket_m.h"
#include "App_GenericDataPacket_m.h"
#include "App_ControlMessage_m.h"
#include "NetworkControlMessage_m.h"
#include "ResourceGenericManager.h"
#include "DebugInfoWriter.h"
using namespace std;


class valuePropagation_ApplicationModule : public cSimpleModule 
{
	private:
	// parameters and variables
	
	/*--- The .ned file's parameters ---*/
		string applicationID;
		bool printDebugInfo;
		int priority;
		int maxAppPacketSize;
		int packetHeaderOverhead;
		int constantDataPayload;
		
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
		
		vector <double> sensedValues;
		
	protected:
		virtual void initialize();
		virtual void handleMessage(cMessage *msg);
		virtual void finish();
		
		void send2NetworkDataPacket(const char *destID, double data, int pckSeqNumber);
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
		void onReceiveMessage(int senderID, double receivedData);
		void onSensorReading(int sensorIndex, string sensorType, double sensedValue);


	public:
		/**
		   ADD HERE YOUR CUSTOM public MEMBER VARIABLES AND FUNCTIONS
		 **/
};

#endif // _VALUEPROPAGATION_APPLICATIONMODULE_H_
