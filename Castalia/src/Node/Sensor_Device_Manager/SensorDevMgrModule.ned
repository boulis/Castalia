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




// The sensor node module. Connects to the wireless channel in order to communicate 
// with other nodes. Connects to psysical processes so it can sample them.

simple SensorDevMgrModule

    parameters:
    	printDebugInfo: bool,
    	numSensingDevices:	const,	// how many sensing devices the node has (has to be <= SensorNetwork.numPhysicalProcesses)
		pwrConsumptionPerDevice:string,
		sensorTypes:		string, // Humidity OR Temperature OR Light
		corrPhyProcess:		string, //holds the indexes of the corresponding phy processes for each sensor (usually it should be : "0 1 2 3")
		maxSampleRates:		string,	//number of samples per second for each sensor
		devicesBias:		string,	//If the output signal is not zero when the measured property is zero
		devicesDrift:		string,	//If the output signal slowly changes independent of the measured property
		devicesNoise:		string, //random deviation of the signal that varies in time
		devicesHysterisis:	string;	//the sensor not instantly follows the change of the property being measured
						 				//and therefore involves the history of the measured property

	gates:
        	out:	toApplicationModule, toNodeContainerModule[];
        	in:	fromNodeContainerModule[], fromApplicationModule, fromResourceManager;

endsimple
