//***************************************************************************************
//*  Copyright: Athens Information Technology (AIT),  2007, 2008, 2009			*
//*		http://www.ait.gr							*
//*             Developed at the Broadband Wireless and Sensor Networks group (B-WiSe) 	*
//*		http://www.ait.edu.gr/research/Wireless_and_Sensors/overview.asp	*
//*											*
//*  Author(s): Dimosthenis Pediaditakis						*
//*											*
//*  This file is distributed under the terms in the attached LICENSE file.		*
//*  If you do not find this file, copies can be found by writing to:			*
//*											*
//*      NICTA, Locked Bag 9013, Alexandria, NSW 1435, Australia			*
//*      Attention:  License Inquiry.							*
//**************************************************************************************/
 
 

#ifndef _VALUEREPORTING_APPLICATIONMODULE_H_
#define _VALUEREPORTING_APPLICATIONMODULE_H_

#include <omnetpp.h>
#include <string>
#include "SensorDevMgr_GenericMessage_m.h"

/*** 
     You have to MODIFY the following "#define" statement by placing the
     header file (*_m.h) that is produced from you custom *.msg message definition file
 ***/
#include "valueReporting_DataPacket_m.h"

#include "App_GenericDataPacket_m.h"
#include "App_ControlMessage_m.h"
#include "NetworkControlMessage_m.h"
#include "ResourceGenericManager.h"
#include "DebugInfoWriter.h"
using namespace std;


class valueReporting_ApplicationModule : public cSimpleModule 
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
		bool isSink;
		double maxSampleInterval;
		double minSampleInterval;
		
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
		 
		App_ControlMessage *app_timer_A;

		int routingLevel;
		double lastSensedValue;
		int currSentSampleSN;
		
		double randomBackoffIntervalFraction;
		bool sentOnce;
		
	protected:
		virtual void initialize();
		virtual void handleMessage(cMessage *msg);
		virtual void finish();
		
		void send2NetworkDataPacket(const char *destID, valueReportData data, int pckSeqNumber);
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

	public:
		/**
		   ADD HERE YOUR CUSTOM public MEMBER VARIABLES AND FUNCTIONS
		 **/
};

#endif // _VALUEREPORTING_APPLICATIONMODULE_H_
