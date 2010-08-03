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

#ifndef VIRTUALNETWORKMODULE
#define VIRTUALNETWORKMODULE

#include "VirtualCastaliaModule.h"
#include "TimerModule.h"
#include "CastaliaMessages.h"
#include "RadioModule.h"

#include "NetworkGenericPacket_m.h"
#include "ApplicationGenericPacket_m.h"

#define SELF_NETWORK_ADDRESS selfAddress.c_str()
#define ROUTE_DEST_DELIMITER "#"

using namespace std;

class VirtualNetworkModule:public VirtualCastaliaModule, public TimerModule {
 protected:
	/*--- The .ned file's parameters ---*/
	int maxNetFrameSize;		//in bytes
	int netDataFrameOverhead;	//in bytes
	int netBufferSize;			//in # of messages

	/*--- Custom class parameters ---*/
	double radioDataRate;
	ResourceGenericManager *resMgrModule;

	queue<cPacket*> TXBuffer;

	double cpuClockDrift;
	int disabled;

	RadioModule *radioModule;
	string selfAddress;
	int self;

	virtual void initialize();
	virtual void startup() { } 
	virtual void handleMessage(cMessage * msg);
	virtual void finish();

	virtual void fromApplicationLayer(cPacket *, const char *) = 0;
	virtual void fromMacLayer(cPacket *, int, double, double) = 0;

	int bufferPacket(cPacket *);

	void toApplicationLayer(cMessage *);
	void toMacLayer(cMessage *);
	void toMacLayer(cPacket *, int);

	void encapsulatePacket(cPacket *, cPacket *);
	cPacket *decapsulatePacket(cPacket *);
	int resolveNetworkAddress(const char *);

	virtual void handleMacControlMessage(cMessage *);
	virtual void handleRadioControlMessage(cMessage *);
	virtual void handleNetworkControlCommand(cMessage *) { }
};

#endif				//BYPASSROUTINGMODULE
