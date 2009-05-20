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




#ifndef _RESOURCEGENERICMANAGER_H_
#define _RESOURCEGENERICMANAGER_H_

#include <omnetpp.h>
#include "ResourceGenericManager_Message_m.h"
#include "DebugInfoWriter.h"
using namespace std;


class ResourceGenericManager : public cSimpleModule 
{
	private:
	// parameters and variables
	
	/*--- The .ned file's parameters ---*/
		bool printDebugInfo;
		double sigmaCPUClockDrift;
		double cpuClockDrift;
		double initialEnergy;
		double ramSize;
		double periodicEnergy;
		double periodicTimer;
		
	/*--- Custom class parameters ---*/
		double remainingEnergy;
		double totalRamData;
		int self;
		
		
	protected:
		virtual void initialize();
		virtual void handleMessage(cMessage *msg);
		virtual void finish();
	
	
	public:
		double getCPUClockDrift(void);
		void consumeEnergy(double amount);
		double getSpentEnergy(void);
		void destroyNode(void);
		int RamStore(int numBytes);
		void RamFree(int numBytes);
};

#endif // _RESOURCEGENERICMANAGER_H_
