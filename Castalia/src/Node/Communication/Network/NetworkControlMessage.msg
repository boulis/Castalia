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


// Message that is sent from Network module to Application module
// in order to pass routing information to it

enum Network_ControlMessageType
{
	NETWORK_2_APP_FULL_BUFFER = 3000;
	NETWORK_2_APP_NOT_CONNECTED = 3001;
	NETWORK_2_APP_CONNECTED_2_TREE = 3002; // used by simpleTreeRouting
	NETWORK_2_APP_TREE_LEVEL_UPDATED = 3003; // used by multipathRingsRouting
	
	NETWORK_SELF_CHECK_TX_BUFFER = 3004;
	NETWORK_SELF_BOOTSTRAP_NET_SETUP = 3005;
	NETWORK_SELF_TOPOLOGY_SETUP_TIMEOUT = 3006;	
};


cplusplus {{
	#define BROADCAST "BROADCAST"
	#define PARENT_LEVEL "PARENT"
	#define SINK 	"SINK"
	#define ROUTE_DEST_DELIMITER "#"
}};




message Network_ControlMessage
{
    fields:    	
    	int sinkID;    	
	
		int level;
	
		string parents;
    	
};
