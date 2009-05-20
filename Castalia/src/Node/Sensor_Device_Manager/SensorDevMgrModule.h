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




#ifndef _APPLICATIONMODULESIMPLE_H_
#define _APPLICATIONMODULESIMPLE_H_

#include <omnetpp.h>
#include <vector>
#include <string>
#include "SensorDevMgr_GenericMessage_m.h"
#include "ResourceGenericManager.h"
#include "VirtualMobilityModule.h"
#include "PhyProcessGenericMessage_m.h"
#include "App_ControlMessage_m.h"
#include "DebugInfoWriter.h"
using namespace std;


class SensorDevMgrModule : public cSimpleModule 
{
	private:
	// parameters and variables
	
	/*--- The .ned file's parameters ---*/
		bool printDebugInfo;
		vector <int> corrPhyProcess;
		vector <double> pwrConsumptionPerDevice;
		vector <double> minSamplingIntervals;
		vector <string> sensorTypes;
		vector <double> sensorBiasSigma;
		vector <double> sensorNoiseSigma;
		
	/*--- Custom class member variables ---*/
		int self;	// the node's ID
		double self_xCoo;
		double self_yCoo;
		int totalSensors;
		vector <double> sensorlastSampleTime;
		vector <double> sensorLastValue;
		vector <double> sensorBias;
		ResourceGenericManager *resMgrModule;	//a pointer to the object of the Radio Module (used for direct method calls)
		VirtualMobilityModule *nodeMobilityModule;
		int disabled;
		
		
	protected:
		virtual void initialize();
		virtual void handleMessage(cMessage *msg);
		virtual void finish();
		
		void parseStringParams(void);
		
	public:
		double getSensorDeviceBias(int index);
		
};

#endif // _APPLICATIONMODULESIMPLE_H_
