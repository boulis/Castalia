/****************************************************************************
 *  Copyright: National ICT Australia,  2007 - 2012                         *
 *  Developed at the ATP lab, Networked Systems research theme              *
 *  Author(s): Yuriy Tselishchev                                            *
 *  This file is distributed under the terms in the attached LICENSE file.  *
 *  If you do not find this file, copies can be found by writing to:        *
 *                                                                          *
 *      NICTA, Locked Bag 9013, Alexandria, NSW 1435, Australia             *
 *      Attention:  License Inquiry.                                        *
 *                                                                          *  
 ****************************************************************************/

#ifndef STATIC_GTS_802154_H_
#define STATIC_GTS_802154_H_

#include <string>
#include <vector>

#include "Basic802154.h"
#include "Basic802154Packet_m.h"

class StaticGTS802154: public Basic802154 {
 protected:
 	/*--- 802154Mac GTS list --- */
	vector<Basic802154GTSspec> GTSlist;	// list of GTS specifications (for PAN coordinator)
	int assignedGTS,requestGTS,totalGTS,totalSlots,baseSlot,minCap,frameOrder;
	bool gtsOnly;

	virtual void startup();
	virtual int gtsRequest_hub(Basic802154Packet *);
	virtual void prepareBeacon_hub(Basic802154Packet *);
	virtual void disconnectedFromPAN_node();
	virtual void assignedGTS_node(int);
	virtual bool acceptNewPacket(Basic802154Packet *);
	virtual void transmissionOutcome(Basic802154Packet *, bool, string);
};

#endif	//STATIC_GTS_802154
