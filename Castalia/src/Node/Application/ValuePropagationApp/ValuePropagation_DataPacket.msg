cplusplus {{
#include "App_GenericDataPacket_m.h"
}};

class App_GenericDataPacket;

message ValuePropagation_DataPacket extends App_GenericDataPacket
{
    fields:
        double data;
};
