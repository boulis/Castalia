/*******************************************************************************
 *  Copyright: National ICT Australia,  2007 - 2010                            *
 *  Developed at the Networks and Pervasive Computing program                  *
 *  Author(s): Athanassios Boulis, Dimosthenis Pediaditakis, Yuriy Tselishchev *
 *  This file is distributed under the terms in the attached LICENSE file.     *
 *  If you do not find this file, copies can be found by writing to:           *
 *                                                                             *
 *      NICTA, Locked Bag 9013, Alexandria, NSW 1435, Australia                *
 *      Attention:  License Inquiry.                                           *
 *                                                                             *  
 *******************************************************************************/

#ifndef BYPASSROUTINGMODULE
#define BYPASSROUTINGMODULE

#include "VirtualNetworkModule.h"
#include "BypassRoutingPacket_m.h"

using namespace std;

class BypassRoutingModule:public VirtualNetworkModule {
 protected:
	void fromApplicationLayer(cPacket *, const char *);
	void fromMacLayer(cPacket *, int, double, double);
};

#endif				//BYPASSROUTINGMODULE
