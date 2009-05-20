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

simple simpleAggregation_ApplicationModule

    parameters:
    	applicationID:  string,
    	printDebugInfo:	bool,
		priority 	  : const,
		maxAppPacketSize  : const,	 // in bytes
		packetHeaderOverhead: const, // in bytes
		constantDataPayload: const,	 // in bytes
		isSink: bool;

    gates:
        out:	toCommunicationModule, toSensorDeviceManager;
        in:	fromCommunicationModule, fromSensorDeviceManager, fromResourceManager;

endsimple
