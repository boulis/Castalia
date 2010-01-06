//***************************************************************************************
//*  Copyright: National ICT Australia,  2007, 2008, 2009				*
//*  Developed at the Networks and Pervasive Computing program				*
//*  Author(s): Yuri Tselishchev							*
//*  This file is distributed under the terms in the attached LICENSE file.		*
//*  If you do not find this file, copies can be found by writing to:			*
//*											*
//*      NICTA, Locked Bag 9013, Alexandria, NSW 1435, Australia			*
//*      Attention:  License Inquiry.							*
//*											*
//***************************************************************************************

package Node.Application.BridgeTest;

// The sensor node module. Connects to the wireless channel in order to communicate 
// with other nodes. Connects to psysical processes so it can sample them.

simple BridgeTest_ApplicationModule like Node.Application.ApplicationGenericModule

{
    parameters:
        string applicationID = default("BridgeTest");
        bool printDebugInfo = default(false);
        int priority = default(1);
        int maxAppPacketSize = default(0);		// in bytes
        int packetHeaderOverhead = default(8);		// in bytes
        
        bool isSink = default(false);
        bool broadcastReports = default(false);
        double reportTreshold = default(10);
        double sampleInterval = default(100);

    gates:
        output toCommunicationModule;
        output toSensorDeviceManager;
        input fromCommunicationModule;
        input fromSensorDeviceManager;
        input fromResourceManager;

}
