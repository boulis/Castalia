/****************************************************************************
 *  Copyright: National ICT Australia,  2007 - 2010                         *
 *  Developed at the Networks and Pervasive Computing program               *
 *  Author(s): Yuriy Tselishchev                                            *
 *  This file is distributed under the terms in the attached LICENSE file.  *
 *  If you do not find this file, copies can be found by writing to:        *
 *                                                                          *
 *      NICTA, Locked Bag 9013, Alexandria, NSW 1435, Australia             *
 *      Attention:  License Inquiry.                                        *
 *                                                                          *  
 ****************************************************************************/

#ifndef BYPASSMACMODULE
#define BYPASSMACMODULE

#include <omnetpp.h>
#include "BypassMacSimpleFrame_m.h"
#include "VirtualMacModule.h"

using namespace std;

class BypassMacModule: public VirtualMacModule {
    /**
     * In order to create a MAC based on VirtualMacModule, we need to define only two functions:
     * to handle a packet received from layer above (network) and layer below (radio)
     */
 protected:
	void fromRadioLayer(cPacket *, double, double);
	void fromNetworkLayer(cPacket *, int);
};

#endif
