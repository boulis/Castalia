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
 


#include "ResourceGenericManager.h"

#define EV   ev.disabled() ? (ostream&)ev : ev



Define_Module(ResourceGenericManager);



void ResourceGenericManager::initialize() 
{
	sigmaCPUClockDrift = par("sigmaCPUClockDrift");
	cpuClockDrift = normal(0, sigmaCPUClockDrift);  //using the "0" rng generator of the ResourceGenericManager module 
	initialEnergy = par("initialEnergy");
	ramSize = par("ramSize");
	
	remainingEnergy = initialEnergy;
	totalRamData = 0;
}


void ResourceGenericManager::handleMessage(cMessage *msg)
{
	//The ResourceGenericManager module is not connected with other modules.
	//They use instead its public methods.
}


void ResourceGenericManager::finish()
{
	//recordScalar("Node's remaining Energy", remainingEnergy);
	recordScalar("Spent Energy", initialEnergy - remainingEnergy);
}


double ResourceGenericManager::getCPUClockDrift(void)
{
	Enter_Method("getCPUClockDrift(void)");
	return (1+cpuClockDrift);
}


void ResourceGenericManager::consumeEnergy(double amount)
{
	Enter_Method("consumeEnergy(double amount)");
	
	if(remainingEnergy < amount)
	{
		send(new ResourceGenericManager_Message("Out of energy message", RESOURCE_MGR_OUT_OF_ENERGY), "toSensorDevManager");
		send(new ResourceGenericManager_Message("Out of energy message", RESOURCE_MGR_OUT_OF_ENERGY), "toApplication");
		send(new ResourceGenericManager_Message("Out of energy message", RESOURCE_MGR_OUT_OF_ENERGY), "toMac");
		send(new ResourceGenericManager_Message("Out of energy message", RESOURCE_MGR_OUT_OF_ENERGY), "toRadio");
	}
	else
		remainingEnergy -= amount;
		
	//EV << "\n[Resource Manager]: consume: " << amount << "  Remaining energy: " << remainingEnergy << "\n";
}


int ResourceGenericManager::RamStore(int numBytes)
{
	Enter_Method("RamStore(int numBytes)");
	
	int ramHasSpace = ((totalRamData+numBytes) <= ramSize)?1:0;
	
	if(!ramHasSpace)
	{
		EV << "\n[Resource Manager]: Data not stored to Ram. Not enough space to store them!\n";
		return 0;
	}
	else
		totalRamData += numBytes;

	return 1;
}


void ResourceGenericManager::RamFree(int numBytes)
{
	Enter_Method("RamFree(int numBytes)");
	
	totalRamData -= numBytes;
	
	totalRamData = (totalRamData < 0)?0:totalRamData;
}
