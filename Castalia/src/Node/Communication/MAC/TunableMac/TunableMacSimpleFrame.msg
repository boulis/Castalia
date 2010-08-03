// **************************************************************************************
// *  Copyright: National ICT Australia,  2010                                          *
// *  Developed at the Networks and Pervasive Computing program                         *
// *  Author(s): Yuriy Tselishchev                                                      *
// *  This file is distributed under the terms in the attached LICENSE file.            *
// *  If you do not find this file, copies can be found by writing to:                  *
// *                                                                                    *
// *      NICTA, Locked Bag 9013, Alexandria, NSW 1435, Australia                       *
// *      Attention:  License Inquiry.                                                  *
// *                    CASTALIA 2.0                                                    *
// **************************************************************************************/

cplusplus {{
#include "MacGenericPacket_m.h"
}}

class MacGenericPacket;

enum TunableMacFrameTypeDef {
	DATA_FRAME = 1;
	BEACON_FRAME = 2;
}

packet TunableMacSimpleFrame extends MacGenericPacket {
	int frameType enum (TunableMacFrameTypeDef);	//1 byte
	int source;		//4 bytes
	int destination;	//4 bytes
}

