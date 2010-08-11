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

#ifndef VIRTUALMACMODULE
#define VIRTUALMACMODULE

#define SELF_MAC_ADDRESS self

#include <queue>
#include <map>
#include <omnetpp.h>

#include "RoutingPacket_m.h"
#include "MacPacket_m.h"
#include "RadioControlMessage_m.h"

#include "ResourceManager.h"
#include "Radio.h"
#include "TimerService.h"
#include "CastaliaModule.h"

using namespace std;

class VirtualMac: public CastaliaModule, public TimerService {
 private:
	int disabled;
	int macBufferSize;
	int macMaxFrameSize;
	int macFrameOverhead;

	void createAndSendRadioControlCommand(RadioControlCommand_type, double, const char *, BasicState_type);

 protected:
	int self;		// the node's ID
	
	//a pointer to the object of the Radio Module (used for direct method calls)
	Radio *radioModule;
	
	//a pointer to the object of the Resource Manager Module (used for direct method calls)
	ResourceManager *resMgrModule;	
	
	queue<cPacket*> TXBuffer;

	void initialize();
	void handleMessage(cMessage * msg);
	void finish();
	virtual void finishSpecific() {}
	virtual void startup() {}

	void toNetworkLayer(cMessage *);
	void toRadioLayer(cMessage *);
	virtual void fromNetworkLayer(cPacket *, int) = 0;
	virtual void fromRadioLayer(cPacket *, double, double) = 0;
	int bufferPacket(cPacket *);

	virtual int handleControlCommand(cMessage * msg);
	virtual int handleRadioControlMessage(cMessage * msg);

	void encapsulatePacket(cPacket *, cPacket *);
	cPacket *decapsulatePacket(cPacket *);
};

#endif
