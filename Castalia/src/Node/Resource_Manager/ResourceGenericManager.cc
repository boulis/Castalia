//***************************************************************************************
//*  Copyright: National ICT Australia,  2007, 2008, 2009				*
//*  Developed at the Networks and Pervasive Computing program				*
//*  Author(s): Athanassios Boulis, Dimosthenis Pediaditakis, Yuri Tselishchev		*
//*  This file is distributed under the terms in the attached LICENSE file.		*
//*  If you do not find this file, copies can be found by writing to:			*
//*											*
//*      NICTA, Locked Bag 9013, Alexandria, NSW 1435, Australia			*
//*      Attention:  License Inquiry.							*
//*											*
//***************************************************************************************

 


#include "ResourceGenericManager.h"

#define EV   ev.disabled() ? (ostream&)ev : ev

#define CASTALIA_DEBUG (!printDebugInfo)?(ostream&)DebugInfoWriter::getStream():DebugInfoWriter::getStream()


Define_Module(ResourceGenericManager);



void ResourceGenericManager::initialize() 
{
	self = parentModule()->index();

	printDebugInfo = par("printDebugInfo");
	sigmaCPUClockDrift = par("sigmaCPUClockDrift");
	cpuClockDrift = normal(0, sigmaCPUClockDrift);  //using the "0" rng generator of the ResourceGenericManager module 
	initialEnergy = par("initialEnergy");
	ramSize = par("ramSize");
	periodicEnergy = par("periodicEnergy");
	periodicTimer = par("periodicTimer");

	if (periodicEnergy > 0 && periodicTimer > 0) {
	    periodicTimer = periodicTimer / 1000;
	    periodicEnergy = (periodicEnergy / 1000) * periodicTimer;
	    scheduleAt(simTime()+ periodicTimer, new ResourceGenericManager_Message("Periodic energy message", RESOURCE_MGR_PERIODIC_ENERGY));
	}
	
	remainingEnergy = initialEnergy;
	totalRamData = 0;
}


void ResourceGenericManager::handleMessage(cMessage *msg)
{
	//The ResourceGenericManager module is not connected with other modules.
	//They use instead its public methods.
	//The only possible message is periodic energy consumption
	
	int msgKind = msg->kind();
	switch (msgKind) {
	    
	    case RESOURCE_MGR_PERIODIC_ENERGY: {
		consumeEnergy(periodicEnergy);
		if (remainingEnergy > 0) {
		    scheduleAt(simTime()+ periodicTimer, new ResourceGenericManager_Message("Periodic energy message", RESOURCE_MGR_PERIODIC_ENERGY));
		}
	    }
	    
	    default: {
	    	CASTALIA_DEBUG << "\n[Resource Manager] t= " << simTime() << ": WARNING: Unexpected message: " << msgKind;
	    }
	}
	
	delete msg;
	msg = NULL;
}


void ResourceGenericManager::finish()
{
	// DO NOT ADD HERE ANY CODE FOR RECORDING SCALARS OR VECTORS
	// SOME MODULES MAY CONSUME ADDITIONAL ENERGY OR OCCUPY EXTRA RESOURCES
	// AFTER THE END OF THIS FINISH() 
}

double ResourceGenericManager::getSpentEnergy(void)
{
	Enter_Method("getSpentEnergy()");
	
	return (initialEnergy - remainingEnergy);
}


double ResourceGenericManager::getCPUClockDrift(void)
{
	Enter_Method("getCPUClockDrift(void)");
	return (1.0f+cpuClockDrift);
}


void ResourceGenericManager::consumeEnergy(double amount)
{
	Enter_Method("consumeEnergy(double amount)");
	
	if(remainingEnergy < amount)
	{
		send(new ResourceGenericManager_Message("Out of energy message", RESOURCE_MGR_OUT_OF_ENERGY), "toSensorDevManager");
		send(new ResourceGenericManager_Message("Out of energy message", RESOURCE_MGR_OUT_OF_ENERGY), "toApplication");
		send(new ResourceGenericManager_Message("Out of energy message", RESOURCE_MGR_OUT_OF_ENERGY), "toNetwork");
		send(new ResourceGenericManager_Message("Out of energy message", RESOURCE_MGR_OUT_OF_ENERGY), "toMac");
		send(new ResourceGenericManager_Message("Out of energy message", RESOURCE_MGR_OUT_OF_ENERGY), "toRadio");
		
	}
	else
		remainingEnergy -= amount;
		
	//CASTALIA_DEBUG << "\n[Resource Manager]: consume: " << amount << "  Remaining energy: " << remainingEnergy << "\n";
}



void ResourceGenericManager::destroyNode(void)
{
	Enter_Method("destroyNode(void)");
	
	send(new ResourceGenericManager_Message("Destroy node message", RESOURCE_MGR_DESTROY_NODE), "toSensorDevManager");
	send(new ResourceGenericManager_Message("Destroy node message", RESOURCE_MGR_DESTROY_NODE), "toApplication");
	send(new ResourceGenericManager_Message("Destroy node message", RESOURCE_MGR_DESTROY_NODE), "toNetwork");
	send(new ResourceGenericManager_Message("Destroy node message", RESOURCE_MGR_DESTROY_NODE), "toMac");
	send(new ResourceGenericManager_Message("Destroy node message", RESOURCE_MGR_DESTROY_NODE), "toRadio");
	
	//EV << "\n[Resource Manager_" << self << "]: Node destroyed" << "\n";
}


int ResourceGenericManager::RamStore(int numBytes)
{
	Enter_Method("RamStore(int numBytes)");
	
	int ramHasSpace = ((totalRamData+numBytes) <= ramSize)?1:0;
	
	if(!ramHasSpace)
	{
		CASTALIA_DEBUG << "\n[Resource Manager] t= " << simTime() << ": WARNING: Data not stored to Ram. Not enough space to store them.";
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
