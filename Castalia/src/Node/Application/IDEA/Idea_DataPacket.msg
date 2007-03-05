cplusplus {{
#include "App_GenericDataPacket_m.h"
#include "Idea.h"
}};

class App_GenericDataPacket;
struct estim;

message Idea_DataPacket extends App_GenericDataPacket
{
    fields:
        estim data;
};
