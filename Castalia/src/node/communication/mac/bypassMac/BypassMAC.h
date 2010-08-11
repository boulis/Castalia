/****************************************************************************
 *  Copyright: National ICT Australia,  2007 - 2010                         *
 *  Developed at the ATP lab, Networked Systems research theme              *
 *  Author(s): Yuriy Tselishchev                                            *
 *  This file is distributed under the terms in the attached LICENSE file.  *
 *  If you do not find this file, copies can be found by writing to:        *
 *                                                                          *
 *      NICTA, Locked Bag 9013, Alexandria, NSW 1435, Australia             *
 *      Attention:  License Inquiry.                                        *
 *                                                                          *  
 ****************************************************************************/

#ifndef _BYPASSMAC_H_
#define _BYPASSMAC_H_

#include <omnetpp.h>
#include "BypassMacPacket_m.h"
#include "VirtualMac.h"

using namespace std;

class BypassMAC: public VirtualMac {
    /**
     * In order to create a MAC based on VirtualMacModule, we need to define only two functions:
     * to handle a packet received from layer above (network) and layer below (radio)
     */
 protected:
	void fromRadioLayer(cPacket *, double, double);
	void fromNetworkLayer(cPacket *, int);
};

#endif
