//***************************************************************************************
//*  Copyright: National ICT Australia,  2010                                           *
//*  Developed at the Networks and Pervasive Computing program                          *
//*  Author(s): Yuriy Tselishchev                                                       *
//*  This file is distributed under the terms in the attached LICENSE file.             *
//*  If you do not find this file, copies can be found by writing to:                   *
//*                                                                                     *
//*      NICTA, Locked Bag 9013, Alexandria, NSW 1435, Australia                        *
//*      Attention:  License Inquiry.                                                   *
//*                                                                                     *
//***************************************************************************************

enum MacControlMessage_type {
	MAC_BUFFER_FULL = 1;
}

struct MacInteractionControl_type {
	double RSSI;
	double LQI;
}

packet MacGenericPacket {
	MacInteractionControl_type MacInteractionControl;
} 

message MacControlMessage {
	int macControlMessageKind enum (MacControlMessage_type);
}

