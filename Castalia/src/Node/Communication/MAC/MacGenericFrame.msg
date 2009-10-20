// **************************************************************************************
// *  Copyright: National ICT Australia,  2006- 2009					*
// *  Developed at the Networks and Pervasive Computing program				*
// *  Author(s): Athanassios Boulis, Dimosthenis Pediaditakis				*
// *  This file is distributed under the terms in the attached LICENSE file.		*
// *  If you do not find this file, copies can be found by writing to:			*
// *											*
// *      NICTA, Locked Bag 9013, Alexandria, NSW 1435, Australia			*
// *      Attention:  License Inquiry.							*
// *			CASTALIA 2.0							*
// **************************************************************************************/


// simulation related message type (used to set/get the message kind)
enum MAC_GenericFrameType
{
	MAC_FRAME = 2100;  	//data frame MAC-->Radio and Radio-->MAC
	MAC_FRAME_SELF_PUSH_TX_BUFFER = 2101;
};



// MAC layer protocol types (used to give values to the frameType field of struct macHeaderInfo)
// A MAC_FRAME can be of any of the following types:
enum MAC_ProtocolFrameType
{
	MAC_PROTO_DATA_FRAME = 2102;
	MAC_PROTO_BEACON_FRAME = 2103;
	MAC_PROTO_ACK_FRAME = 2104;

	MAC_PROTO_SYNC_FRAME = 93;
	MAC_PROTO_RTS_FRAME = 94;
	MAC_PROTO_CTS_FRAME = 95;
	MAC_PROTO_DS_FRAME  = 96;
	MAC_PROTO_FRTS_FRAME = 97;
	
	MAC_802154_BEACON_FRAME = 1001;
	MAC_802154_ASSOCIATE_FRAME = 1002;
	MAC_802154_DATA_FRAME = 1003;
	MAC_802154_ACK_FRAME = 1004;
	MAC_802154_GTS_REQUEST_FRAME = 1005;
};

cplusplus {{
#define MAC_BROADCAST_ADDR -1
}};

struct macHeaderInfo
{
	fields:
		int frameType enum(MAC_ProtocolFrameType);

		int srcID;
		int destID;
	
	//fields for Tmac
		int dataSequenceNumber;
		double NAV;
		double SYNC;
		int SYNC_ID;
		int SYNC_SN;
	
	//fields for 802.15.4
		int PANid;
		int beaconOrder;
		int frameOrder;
		int BSN;
		int CAPlength;
		int GTSlength;
};

struct GTSspec
{
    fields:
	int owner;
	int start;
	int length;
}

message MAC_GenericFrame
{
    fields:
	macHeaderInfo header;

	GTSspec GTSlist[]; //for 802.15.4

	double rssi;
};
