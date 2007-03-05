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



#ifndef _RESOURCEGENERICMANAGER_H_
#define _RESOURCEGENERICMANAGER_H_

#include <omnetpp.h>
#include "ResourceGenericManager_Message_m.h"
using namespace std;


class ResourceGenericManager : public cSimpleModule 
{
	private:
	// parameters and variables
	
	/*--- The .ned file's parameters ---*/
		double sigmaCPUClockDrift;
		double cpuClockDrift;
		double initialEnergy;
		double ramSize;
		
	/*--- Custom class parameters ---*/
		double remainingEnergy;
		double totalRamData;
		
		
	protected:
		virtual void initialize();
		virtual void handleMessage(cMessage *msg);
		virtual void finish();
	
	
	public:
		double getCPUClockDrift(void);
		void consumeEnergy(double amount);
		int RamStore(int numBytes);
		void RamFree(int numBytes);
};

#endif // _RESOURCEGENERICMANAGER_H_
