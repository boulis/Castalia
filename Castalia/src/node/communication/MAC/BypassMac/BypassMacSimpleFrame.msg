//****************************************************************************
//*  Copyright: National ICT Australia,  2007 - 2010                         *
//*  Developed at the Networks and Pervasive Computing program               *
//*  Author(s): Yuriy Tselishchev                                            *
//*  This file is distributed under the terms in the attached LICENSE file.  *
//*  If you do not find this file, copies can be found by writing to:        *
//*                                                                          *
//*      NICTA, Locked Bag 9013, Alexandria, NSW 1435, Australia             *
//*      Attention:  License Inquiry.                                        *
//*                                                                          *  
//****************************************************************************/

cplusplus {{
#include "MacGenericPacket_m.h"
}}

class MacGenericPacket;

packet BypassMacSimpleFrame extends MacGenericPacket {
	int source;
	int destination;
}

