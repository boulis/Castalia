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


simple simpleTreeRoutingModule

    parameters:
		printDebugInfo: bool,
		maxNetFrameSize	: const,	// in bytes
		netBufferSize	: const,	//in number of messages
		netDataFrameOverhead: const,		// in bytes
		netTreeSetupFrameOverhead: const,	// in bytes
		netSetupTimeout:const, //in msec
		topoSetupUdateTimeout:const,  //in msec
		maxNumberOfParents:const,
		maxNeighborsTableSize:const,
		rssiBased_NeighborQuality:bool,
		neighbor_RSSIThreshold:const; //in dBm

    gates:
        out:	toCommunicationModule, toMacModule;
        in:	fromCommunicationModule, fromMacModule, fromCommModuleResourceMgr;

endsimple
