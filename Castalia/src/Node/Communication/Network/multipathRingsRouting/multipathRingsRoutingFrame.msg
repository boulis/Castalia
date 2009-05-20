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


cplusplus {{
#include "NetworkGenericFrame_m.h"
}};


class Network_GenericFrame;



//Extra Network layer protocol types (used to give values to the frameType field of struct networkHeaderInfo)
//A NET_FRAME can be of any of the following types:
enum multipathRingsRouting_FrameType
{
	MPATH_RINGS_ROUTING_TOPOLOGY_SETUP_FRAME = 3102;
};





// In you code you should set the following message with
// header.frameType = TREE_ROUTING_TOPOLOGY_SETUP_FRAME
message multipathRingsRouting_DataFrame extends Network_GenericFrame
{
	fields:
		// information related to the sink (tree root)		
		unsigned short sinkID;  // value range: 0 to 65535
	
		
		// information related to the sender
		// the ID of the sender can be found in the "header" field,
		// which is inherited from the Network_GenericFrame
		
		short senderLevel; // value range: -32768 to 32767
};




// In you code you should set the following message with
// header.frameType = TREE_ROUTING_TOPOLOGY_SETUP_FRAME
message multipathRingsRouting_TopoSetup_Frame extends Network_GenericFrame
{
	fields:
		// information related to the sink (tree root)
		unsigned short sinkID;
		
		// information related to the sender
		// the ID of the sender can be found in the "header" field,
		// which is inherited from the Network_GenericFrame
		
		short senderLevel;
};


