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

#include "TraceChannel.h"

Define_Module(TraceChannel);

void TraceChannel::initialize()
{
	DebugInfoWriter::setDebugFileName(
		getParentModule()->par("debugInfoFileName").stringValue());

	numNodes = getParentModule()->par("numNodes");
	coordinator = par("coordinator");

	if (coordinator >= numNodes || coordinator < 0) 
		opp_error("Invalid value of coordinator parameter in TraceChannel module\n");

	nodesAffectedByTransmitter = new list<int>[numNodes];
	if (nodesAffectedByTransmitter == NULL)
		opp_error("Could not allocate nodesAffectedByTransmitter array\n");
		
	traceStep = (double)par("traceStep")/1000.0;
		
	nextLine = 0;
	traceFile.open((const char *)par("traceFile"));
	if (!traceFile.is_open())
		opp_error("Could not open trace file for reading\n");
	
	signalDeliveryThreshold = par("signalDeliveryThreshold");
	pathlossMapFile = par("pathlossMapFile");
	temporalModelParametersFile = par("temporalModelParametersFile");
	leafLinkProbability = par("leafLinkProbability");
	temporalModel = NULL;
	
	if (strlen(pathlossMapFile) != 0) {
		parsePathLossMap(par("pathlossMapOffset"));
		/* Create temporal model object from parameters file (if given) */
		if (strlen(temporalModelParametersFile) > 0) {
			temporalModel = new channelTemporalModel(temporalModelParametersFile, 2);
		}
	} else {
		for (int i = 0; i < numNodes; i++) {
			for (int j = 0; j < numNodes; j++) {
				// uniform(0,1) generates a random number in range [0,1)
				if (leafLinkProbability > 0 && 	uniform(0,1) < leafLinkProbability) {
					pathlossMap[i][j] = new PathLossElement((double)par("leafPathloss"));
				} else {
					pathlossMap[i][j] = new PathLossElement(0);
				}
			}
		}
	}
	
	declareOutput("Sensor-to-sensor links");
}

void TraceChannel::parsePathLossMap(double offset)
{

	ifstream f(pathlossMapFile);
	if (!f.is_open())
		opp_error("\n[Wireless Channel]:\n Error reading from pathLossMapFile %s\n", pathlossMapFile);

	string s;
	const char *ct;
	int source, destination;
	float pathloss_db;

	/* each line in a file is in the same format:
	 * Transmitter_id>Receiver_id:PathLoss_dB, ... ,Receiver_id:PathLoss_dB
	 * based on these values we will update the pathloss array
	 * (using updatePathLossElement function)
	 */
	while (getline(f, s)) {
		ct = s.c_str();	//ct points to the beginning of a line
		while (ct[0] && (ct[0] == ' ' || ct[0] == '\t'))
			ct++;
		if (!ct[0] || ct[0] == '#')
			continue;	// skip comments
		if (parseInt(ct, &source))
			opp_error("\n[Wireless Channel]:\n Bad syntax in pathLossMapFile, expecting source identifier\n");
		while (ct[0] && ct[0] != '>')
			ct++;	//skip untill '>' character
		if (!ct[0])
			opp_error("\n[Wireless Channel]:\n Bad syntax in pathLossMapFile, expecting comma separated list of values\n");
		cStringTokenizer t(++ct, ",");	//divide the rest of the strig with comma delimiter
		while ((ct = t.nextToken())) {
			if (parseInt(ct, &destination))
				opp_error("\n[Wireless Channel]:\n Bad syntax in pathLossMapFile, expecting target identifier\n");
			while (ct[0] && ct[0] != ':')
				ct++;	//skip untill ':' character
			if (parseFloat(++ct, &pathloss_db))
				opp_error("\n[Wireless Channel]:\n Bad syntax in pathLossMapFile, expecting dB value for path loss\n");
			pathlossMap[source][destination] = new PathLossElement(pathloss_db + offset);
		}
	}
	f.close();
}

/*****************************************************************************
 * This is where the main work is done by processing all the messages received
 *****************************************************************************/
void TraceChannel::handleMessage(cMessage * msg)
{
	switch (msg->getKind()) {

	case WC_NODE_MOVEMENT:{
		opp_error("Error: Trace channel does not support WC_NODE_MOVEMENT message");
		break;
	}

	case WC_SIGNAL_START:{
		WirelessChannelSignalBegin *signalMsg =
		    check_and_cast <WirelessChannelSignalBegin*>(msg);
		int srcAddr = signalMsg->getNodeID();
		int receptioncount = 0;

		for (int i = 0; i < numNodes; i++) {
			if (i == srcAddr) continue;
			double signalPower = signalMsg->getPower_dBm();
			if (srcAddr == coordinator) {	
				//transmission from coordinator
				signalPower -= currentPathloss(i);
			} else if (i == coordinator) {	
				//transmission to coordinator
				signalPower -= currentPathloss(srcAddr);
			} else if (pathlossMap[i][srcAddr]->avgPathLoss != 0) { 		
				//transmission between two leaf nodes
				signalPower -= pathlossMap[i][srcAddr]->avgPathLoss;
				if (temporalModel) {
					simtime_t timePassed_msec = (simTime() - pathlossMap[i][srcAddr]->lastObservationTime) * 1000;
					simtime_t timeProcessed_msec =
							temporalModel->runTemporalModel(SIMTIME_DBL(timePassed_msec),
							&pathlossMap[i][srcAddr]->lastObservedDiff);
					signalPower += pathlossMap[i][srcAddr]->lastObservedDiff;
					/* Update the observation time */
					pathlossMap[i][srcAddr]->lastObservationTime = simTime() -
							(timePassed_msec - timeProcessed_msec) / 1000;
				}
			} else {						
				//no reception between these nodes
				continue;
			}
		
			if (signalPower < signalDeliveryThreshold) {
				collectOutput("Sensor-to-sensor links","Down");
				continue;
			}
			collectOutput("Sensor-to-sensor links","Up");
		
			WirelessChannelSignalBegin *signalMsgCopy = signalMsg->dup();
			signalMsgCopy->setPower_dBm(signalPower);
			send(signalMsgCopy, "toNode", i);
			nodesAffectedByTransmitter[srcAddr].push_front(i);
			receptioncount++;
		}

		if (receptioncount > 0)
			trace() << "signal from node[" << srcAddr << "] reached " <<
					receptioncount << " other nodes";
		break;
	}

	case WC_SIGNAL_END:{
		WirelessChannelSignalEnd *signalMsg =
			   check_and_cast <WirelessChannelSignalEnd*>(msg);
		int srcAddr = signalMsg->getNodeID();

		/* Go through the list of nodes that were affected
		 *  by this transmission. *it1 holds the node ID
		 */
		list <int>::iterator it1;
		for (it1 = nodesAffectedByTransmitter[srcAddr].begin();
				it1 != nodesAffectedByTransmitter[srcAddr].end(); it1++) {
			WirelessChannelSignalEnd *signalMsgCopy = signalMsg->dup();
			send(signalMsgCopy, "toNode", *it1);
		}	//for it1
		
		/* Now that we are done processing the msg we delete the whole list
		 * nodesAffectedByTransmitter[srcAddr], since srcAddr in not TXing anymore.
		 */
		nodesAffectedByTransmitter[srcAddr].clear();
		break;
	}

	default:{
		opp_error("ERROR: Wireless Channel received unknown message kind=%i", msg->getKind());
		break;
	}
	
	} //switch
	delete msg;
}

void TraceChannel::finishSpecific()
{
	traceFile.close();
	//close the output stream that CASTALIA_DEBUG is writing to
	DebugInfoWriter::closeStream();
}

float TraceChannel::currentPathloss(int nodeId) {
	simtime_t time = simTime();
	if (time >= nextLine) {
		std::string s;
		while (time >= nextLine) {
			getline(traceFile, s);
			nextLine += traceStep;
			trace() << "Read '" << s << "' next read at " << nextLine;
		}
		cStringTokenizer t(s.c_str(), ",");
		std::vector <std::string> v = t.asVector();
		traceValues.clear();
		for (int i = 0; i < (int)v.size(); i++) {
			traceValues.push_back(parseFloat(v[i].c_str()));
		}
	} 
	
	if (nodeId > coordinator) nodeId--;
	if (nodeId >= (int)traceValues.size()) {
		opp_error("trace file does not provide information for node %i (%i columns)",
				nodeId,(int)traceValues.size());
	} 
	return traceValues[nodeId] > 0 ? traceValues[nodeId] : -traceValues[nodeId];
}

//wrapper function for strtof(...)
float TraceChannel::parseFloat(const char *str)
{
	if (str != NULL && str[0]) {
		char *tmp = NULL;
		float result = strtof(str, &tmp);
		if (str != tmp) return result;
	}
	opp_error("ERROR: TraceChannel unable to parse traceFile at '%s'",str);
}

//wrapper function for atoi(...) call. returns 1 on error, 0 on success
int TraceChannel::parseInt(const char *c, int *dst)
{
	while (c[0] && (c[0] == ' ' || c[0] == '\t'))
		c++;
	if (!c[0] || c[0] < '0' || c[0] > '9')
		return 1;
	*dst = atoi(c);
	return 0;
}

//wrapper function for strtof(...) call. returns 1 on error, 0 on success
int TraceChannel::parseFloat(const char *c, float *dst)
{
	char *tmp;
	*dst = strtof(c, &tmp);
	if (c == tmp)
		return 1;
	return 0;
}

