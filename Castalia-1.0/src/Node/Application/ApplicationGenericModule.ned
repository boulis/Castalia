//***********************************************************
//*  Wireless Sensor Network simulator based on OMNet++     *
//*  copyright Athanassios Boulis, National ICT Australia,  *
//*  Networks and Pervasive Computing program, May 2006     *
//***********************************************************


// The sensor node module. Connects to the wireless channel in order to communicate 
// with other nodes. Connects to psysical processes so it can sample them.

simple ApplicationGenericModule

    parameters:
	priority 	  : const,
	maxAppPacketSize  : const,	// in bytes
	packetHeaderOverhead: const;	// in bytes

    gates:
        out:	toCommunicationModule, toSensorDeviceManager;
        in:	fromCommunicationModule, fromSensorDeviceManager, fromResourceManager;

endsimple
