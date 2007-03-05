// **************************************************************************************
// *  Copyright: National ICT Australia,  2006											*
// *  Developed at the Networks and Pervasive Computing program							*
// *  Author(s): Athanassios Boulis, Dimosthenis Pediaditakis							*
// *  This file is distributed under the terms in the attached LICENSE file.			*
// *  If you do not find this file, copies can be found by writing to:					*
// *																					*
// *      NICTA, Locked Bag 9013, Alexandria, NSW 1435, Australia						*
// *      Attention:  License Inquiry.													*
// *																					*
// **************************************************************************************/




simple ResourceGenericManager

	parameters:
		ramSize:		const, //in kB
		flashSize:		const,
		flashWriteCost:	const, //per kB
		flashReadCost:	const, //per kB
		imageSize:		const, //the space that the OS (e.g. Contiki or TinyOS) occupies in the flash
		
		cpuPowerSpeedLevelNames:string,
		cpuPowerPerLevel:	string,  //spent energy per time unit
		cpuSpeedPerLevel:	string,
		cpuInitialPowerLevel:	numeric, // index for the cpuPowerLevels array
		sigmaCPUClockDrift:	numeric,	// the standard deviation of the Drift of the CPU
		
		initialEnergy:		numeric;// energy of the node in Joules


	gates:
        	out:	toSensorDevManager, toApplication, toMac, toRadio;

endsimple
