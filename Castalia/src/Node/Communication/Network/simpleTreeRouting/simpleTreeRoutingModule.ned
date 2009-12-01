//***************************************************************************************
//*  Copyright: Athens Information Technology (AIT),  2007, 2008, 2009			*
//*		http://www.ait.gr							*
//*             Developed at the Broadband Wireless and Sensor Networks group (B-WiSe) 	*
//*		http://www.ait.edu.gr/research/Wireless_and_Sensors/overview.asp	*
//*											*
//*  Author(s): Dimosthenis Pediaditakis						*
//*											*
//*  This file is distributed under the terms in the attached LICENSE file.		*
//*  If you do not find this file, copies can be found by writing to:			*
//*											*
//*      NICTA, Locked Bag 9013, Alexandria, NSW 1435, Australia			*
//*      Attention:  License Inquiry.							*
//**************************************************************************************/

package Node.Communication.Network.simpleTreeRouting;

simple simpleTreeRoutingModule like Node.Communication.Network.NetworkGenericModule

//====================================================================================
// Network_GenericFrame has the following real-world
// (non-simulation-specific) fields:
//    unsigned short int frameType; --> 2bytes
//    string source;  ----------------> 2bytes
//    string destinationCtrl; --------> 2bytes
//    string lastHop; ------------> 2bytes
//    string nextHop; ------------> 2bytes
//    unsigned short int ttl; ----> 2bytes
//    string applicationID; ------> 2bytes
// Total bytes = 7*2 = 14 (|*|)

// Additionally, the simpleTreeRouting_DataFrame (sub-class
// of Network_GenericFrame) has some extra fields:
//    unsigned short int sinkID; -----------> 2bytes
//    short int senderLevel; ---------------> 2bytes
//    unsigned short int senderParentID; ---> 2bytes
// Total bytes (|*|) 14 + 2*3 = 14 + 6 = 20

// From these 20bytes, simpleTreeRoutingModule doesn't use everything.
// It doesn't use the ttl and applicationID fields.
// Concluding the calculations, the simpleTreeRouting_DataFrame has
// a total overhead of 20-(2+2) = 16 bytes

// The same holds also for the simpleTreeRouting_TopoSetup_Frame (16 bytes overhead)
//====================================================================================

{
    parameters:
        bool printDebugInfo = default(false);
        int maxNetFrameSize = default(84);			// bytes
        int netBufferSize = default(32);			// number of messages
        int netDataFrameOverhead = default(16);			// bytes
        int netTreeSetupFrameOverhead = default(16);		// bytes
        int netSetupTimeout = default(50); 			// msec
        int topoSetupUdateTimeout = default(10000);  		// msec
        int maxNumberOfParents = default(1);
        int maxNeighborsTableSize = default(30);
        bool rssiBased_NeighborQuality = default(true);
        double neighbor_RSSIThreshold = default(-90.0);		// dBm

    gates:
        output toCommunicationModule;
        output toMacModule;
        input fromCommunicationModule;
        input fromMacModule;
        input fromCommModuleResourceMgr;

}
