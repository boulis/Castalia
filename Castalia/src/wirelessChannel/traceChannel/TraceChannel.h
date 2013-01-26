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

#ifndef _TRACECHANNEL_H
#define _TRACECHANNEL_H

#include "WirelessChannelMessages_m.h"
#include "CastaliaModule.h"
#include "WirelessChannelTemporal.h"

#include <list>
#include <vector>
#include <string>
#include <cstring>
#include <map>

using namespace std;

class PathLossElement {
 public:
	float avgPathLoss;
	float lastObservedDiff;
	simtime_t lastObservationTime;

	PathLossElement(float PL) {
		avgPathLoss = PL;
		lastObservedDiff = 0.0;
		lastObservationTime = 0.0;
	};

	~PathLossElement() {
	};
};

class TraceChannel: public CastaliaModule {
 private:
	int numNodes;

	/*--- variables corresponding to .ned file's parameters ---*/
	int coordinator;
	double traceStep;
	double leafLinkProbability;
	double leafPathloss;
	double pathlossMapOffset;
	double signalDeliveryThreshold;
	const char *pathlossMapFile;
	const char *temporalModelParametersFile;
	channelTemporalModel *temporalModel;
	map < int,map<int,PathLossElement*> > pathlossMap;

	/*--- state variables for trace file processing ---*/
	ifstream traceFile;
 	simtime_t nextLine;
 	vector <float>traceValues; 	
	
	list <int>*nodesAffectedByTransmitter;	// an array of lists (numOfNodes long). The list
											// at array element i holds the node IDs that are
											// affected when node i transmits.
 protected:
	virtual void initialize();
	virtual void handleMessage(cMessage * msg);
	virtual void finishSpecific();
	float currentPathloss(int);
	float parseFloat(const char *);
	int parseFloat(const char *, float *);
	int parseInt(const char *, int *);
	void parsePathLossMap(double);
};

#endif				//_WIRELESSCHANNEL_H
