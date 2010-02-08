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

package Node.Application.throughputTest;

// The sensor node module. Connects to the wireless channel in order to communicate 
// with other nodes. Connects to psysical processes so it can sample them.

simple throughputTest_ApplicationModule like Node.Application.ApplicationGenericModule

{
    parameters:
        string applicationID = default("throughputTest");
        bool collectTraceInfo = default(false);
        int priority = default(1);
        int maxAppPacketSize = default(0);
        int packetHeaderOverhead = default(5);		// in bytes
        int constantDataPayload = default(1000);	// in bytes

        string nextRecipient = default("0");		// destination for packets, this value can be used
							// to create multihop throughput test involving many nodes
        double packet_rate = default(0);		// packets per second

    gates:
        output toCommunicationModule;
        output toSensorDeviceManager;
        input fromCommunicationModule;
        input fromSensorDeviceManager;
        input fromResourceManager;

}
