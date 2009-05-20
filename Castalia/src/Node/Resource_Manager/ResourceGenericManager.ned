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





simple ResourceGenericManager

	parameters:
		printDebugInfo: bool,
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
		
		initialEnergy:		numeric, // energy of the node in Joules
		periodicEnergy: 	numeric, // periodic energy spending, in watts
		periodicTimer:		numeric; // periodic timer for calculating periodic energy spending, in msec

	gates:
        	out:	toSensorDevManager, toApplication, toNetwork, toMac, toRadio;

endsimple
