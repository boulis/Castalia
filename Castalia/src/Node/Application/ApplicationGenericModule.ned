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

package Node.Application;

// The sensor node module. Connects to the wireless channel in order to communicate 
// with other nodes. Connects to psysical processes so it can sample them.

moduleinterface ApplicationGenericModule

{
    parameters:
        string applicationID;
        bool printDebugInfo;
        int priority;
        int maxAppPacketSize;		// in bytes
        int packetHeaderOverhead;	// in bytes

    gates:
        output toCommunicationModule;
        output toSensorDeviceManager;
        input fromCommunicationModule;
        input fromSensorDeviceManager;
        input fromResourceManager;

}
