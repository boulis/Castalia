cplusplus {{
#include "App_GenericDataPacket_m.h"
}};

class App_GenericDataPacket;

message Template_DataPacket extends App_GenericDataPacket
{
    fields:
        int data; //you can define one or more of your own data fields (double, int, struct, float etc... )
		  //and replace this default one. It is always convenient for the user to use structs if
		  // he/she plans to use more than one data fields.
};
