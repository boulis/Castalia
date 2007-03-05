cplusplus {{
#include "App_GenericDataPacket_m.h"
}};

class App_GenericDataPacket;

enum multiTree_PacketType
{
	APP_MULTI_TREE_INIT_PACKET = 201;
	APP_MULTI_TREE_DATA_PACKET = 202;
};

struct initMsgFields
{
	fields:
		int initiatorID;
		int parentID;
};

message multipathAggregation_DataPacket extends App_GenericDataPacket
{
    fields:
	int messageType enum(multiTree_PacketType);
        initMsgFields initInfo;
	double data;
	int level;
};
