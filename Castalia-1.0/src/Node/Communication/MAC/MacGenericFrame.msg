// **************************************************************************************
// *  Copyright: National ICT Australia,  2006											*
// *  Developed at the Networks and Pervasive Computing program							*
// *  Author(s): Athanassios Boulis, Dimosthenis Pediaditakis							*
// *  This file is distributed under the terms in the attached LICENSE file.			*
// *  If you do not find this file, copies can be found by writing to:					*
// *																					*
// *      NICTA, Locked Bag 9013, Alexandria, NSW 1435, Australia						*
// *      Attention:  License Inquiry.													*
// *																					*
// **************************************************************************************/




//MAC layer protocol types (used to give values to the frameType field of struct macHeaderInfo)
enum MAC_ProtocolFrameType
{
	MAC_PROTO_DATA_FRAME = 70;
	MAC_PROTO_BEACON_FRAME = 71;
	MAC_PROTO_ACK_FRAME = 72;
};

struct macHeaderInfo
{
	fields:
		char srcMacAddr[4];
		char destMacAddr[4];
		int frameType enum(MAC_ProtocolFrameType);
		int dataSequenceNumber;
};



//simulation related message type (used to set/get the message kind)
enum MAC_GenericFrameType
{
	MAC_FRAME = 73;  	//data frame MAC-->Radio and Radio-->MAC
	MAC_FRAME_SELF_PUSH_TX_BUFFER = 74;
};

message MAC_GenericFrame
{
    fields:
	int srcID;
	int destID;
	int frameLength; //in bytes
        double data;
	macHeaderInfo header;
};
