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


// This file describes the message(s) used in the SN-simulator 
// based on OMNeT++ designed at NICTA

enum App_DataPacketType
{
	APP_DATA_PACKET = 1100;
};

struct AppHeaderInfo
{
	fields:
		string source;
		string destination;
		int seqNumber;
		string applicationID;
};

message App_GenericDataPacket
{
    fields:
        AppHeaderInfo header;
        
		// simulation-specific fields
		double rssi;
		string currentPathFromSource;
};
