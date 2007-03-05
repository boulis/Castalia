
// This file describes the message(s) used in the SN-simulator 
// based on OMNeT++ designed at NICTA

enum App_PacketType
{
	APP_DATA_PACKET = 80;
	APP_NODE_STARTUP = 81;
	APP_SELF_REQUEST_SAMPLE = 82;

	APP_TIMER_1 = 151;
	APP_TIMER_2 = 152;
	APP_TIMER_3 = 153;
	APP_TIMER_4 = 154;
	APP_TIMER_5 = 155;
	APP_TIMER_6 = 156;
	APP_TIMER_7 = 157;
	APP_TIMER_8 = 158;
};

struct AppHeaderInfo
{
	fields:
		int srcID;
		int destID;
		int seqNumber;
};

message App_GenericDataPacket
{
    fields:
        AppHeaderInfo header;
};
