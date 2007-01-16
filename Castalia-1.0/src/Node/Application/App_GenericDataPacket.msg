
// This file describes the message(s) used in the SN-simulator 
// based on OMNeT++ designed at NICTA

enum App_PacketType
{
	APP_DATA_PACKET = 80;
	APP_NODE_STARTUP = 81;
	APP_SELF_REQUEST_SAMPLE = 82;
};


message App_GenericDataPacket
{
    fields:
        int srcID;
        int destID;
	int seqNumber;
	int frameLength; //in bytes
        double data;
};
