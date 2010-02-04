//***************************************************************************************
//*  Copyright: National ICT Australia,  2007 - 2010					*
//*  Developed at the Networks and Pervasive Computing program				*
//*  Author(s): Dimosthenis Pediaditakis, Yuriy Tselishchev				*
//*  This file is distributed under the terms in the attached LICENSE file.		*
//*  If you do not find this file, copies can be found by writing to:			*
//*											*
//*      NICTA, Locked Bag 9013, Alexandria, NSW 1435, Australia			*
//*      Attention:  License Inquiry.							*
//*											*
//***************************************************************************************

cplusplus {{
#include "NetworkGenericPacket_m.h"
}}

class NetworkGenericPacket;

enum simpleTreeRoutingPacketDef {
    ST_ROUTING_DATA_PACKET = 1;
    ST_ROUTING_TOPOLOGY_SETUP_PACKET = 2;
}

packet simpleTreeRoutingPacket extends NetworkGenericPacket {
    int simpleTreeRoutingPacketKind enum(simpleTreeRoutingPacketDef);	// 1 byte
    string source;		// 4 bytes
    string destination;		// 4 bytes
    int sinkID;			// 2 bytes
    int senderLevel;		// 1 byte
    
    //both DATA and SETUP packets contain all fields, making their total size 12 bytes
}
