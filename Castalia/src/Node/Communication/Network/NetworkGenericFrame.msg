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



//simulation related message type (used to set/get the message kind)
enum CastaliaNetwork_GenericFrameType
{
	NETWORK_FRAME = 3100;  	//data frame MAC-->Network and Network-->MAC
};


//Network layer protocol types (used to give values to the frameType field of struct networkHeaderInfo)
//A NET_FRAME can be of any of the following types:
enum NetProtocol_FrameType
{
	NET_PROTOCOL_DATA_FRAME = 0001; // can get integer values within the range [0,65535]
									// as the frameType field in the header of Network_GenericFrame
									// is unsigned short int
	
	// -- Other possible routing-protcol related massage types
	 // NET_PROTOCOL_ACK_FRAME = 0002;
	 // NET_PROTOCOL_NACK_FRAME = 0003;
	 // NET_PROTOCOL_ROUTE_UPDATE_FRAME = 0004;
	 // NET_PROTOCOL_ROUTE_ADVERTISEMENT = 0005;
	 // NET_PROTOCOL_CONTROL_FRAME = 0006;
};



struct netHeaderInfo
{
	fields:
		//real data-specific fields
		unsigned short frameType;
		string source;
		string destinationCtrl;
		string lastHop;
		string nextHop;
		unsigned short ttl;
		
		string applicationID;
};


message Network_GenericFrame
{
    fields:
		netHeaderInfo header;
		
		// simulation-specific fields
		double rssi;
		string currentPathFromSource;
};
