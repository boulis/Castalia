/************************************************************************************
 *  Copyright: National ICT Australia,  2006										*
 *  Developed at the Networks and Pervasive Computing program						*
 *  Author(s): Athanassios Boulis, Dimosthenis Pediaditakis							*
 *  This file is distributed under the terms in the attached LICENSE file.			*
 *  If you do not find this file, copies can be found by writing to:				*
 *																					*
 *      NICTA, Locked Bag 9013, Alexandria, NSW 1435, Australia						*
 *      Attention:  License Inquiry.												*
 *																					*
 ************************************************************************************/



#include <math.h>
#include "WirelessChannel.h"

#define EV ev.disabled() ? (ostream&)ev : ev

Define_Module(WirelessChannel);



ListElement * WirelessChannel::addToFront(ListElement *n, ListElement *newListElement) {
	newListElement->next = n;
	return newListElement;
}


ListElement * WirelessChannel::deleteListElement(ListElement* start, int ind) { //the start index of the linked-list is start
	ListElement *n, *p;

	if (start == NULL) {
		return NULL;
	}

	if (start -> index == ind){
		p = start->next;
		delete start;
		return p;
	}

	for (n = start; n->next != NULL; n = n->next) {
		p = n->next;
		if (p->index == ind) {
			n->next = p->next;
			delete p;
			return start;
		}
	}
	return start;
}


// approximates the addition of 2 signals expressed in dBm. Value returned in dBm
double WirelessChannel::addPower_dBm(double a, double b)const {

	double diff = a - b;
	// accurate additions for specific diff values
	// A = 10^(a/10), B = 10^(b/10), A/B = 10^(diff/10)
	// M = max(A,B), m = min(A,B)
	// sum = M*(1+ m/M)),  dB_add_to_max = 10*log(1+ m/M)
	// For diff -> dB_add_to_max
	// 0 -> 3
	// 1 -> 2.52
	// 2 -> 2.12
	// 3 -> 1.76
	// 4 -> 1.46
	// 5 -> 1.17
	// 6 -> 1
	// 7 -> 0.79

	if (diff > 7)  return a;
	if (diff < -7) return b;
	if (diff > 5)  return (a + 1);
	if (diff < -5) return (b + 1);
	if (diff > 3)  return (a + 1.5);
	if (diff < -3) return (b + 1.5);
	if (diff > 2)  return (a + 2);
	if (diff < -2) return (b + 2);
	if (diff > 1)  return (a + 2.5);
	if (diff < -1) return (b + 2.5);
	if (diff > 0)  return (a + 3);
	return (b + 3);
}


void WirelessChannel::initialize() {

	numNodes = parentModule()->par("numNodes");
	noiseFloor = par("noiseFloor");
	pathLossExponent = par("pathLossExponent");
	sigma = par("sigma");
	PLd0 = par("PLd0");
	d0 = par("d0");
	f = 25;			// frame size. TODO: change?

	// calculate thresholdSNRdB using the parameter thresholdProb
	// this is for specific coding and modulating schemes! TODO: extend to other schemes
	thresholdSNRdB = 10*log10( -1.28*log(2*(1 -  pow((double)par("thresholdProb"), 1/(8*f))   )));

	// activeNodes is empty
	activeNodes = NULL;
	//carrierSensingNodes is empty
	carrierSensingNodes = NULL;

	// extract the txPowerLevels from the string module parameter
	const char *str = par("txPowerLevels");
	cStringTokenizer tokenizer(str);
	const char *token;
	numTxPowerLevels = 0;
	while ((token = tokenizer.nextToken())!=NULL){
		txPowerLevels[numTxPowerLevels] = atof(token);   // convert and store
		numTxPowerLevels++;
	}

	EV << "\n[WChannel]:\n Wireless Channel Initialization - numNodes= " << numNodes << ", numTxPowerLevels= "  << numTxPowerLevels;
	EV << "\nthresholdSNRdB = " << thresholdSNRdB;

	// Allocate and Initialize rxSignal

	rxSignal = new double** [numNodes];
	if (rxSignal==NULL) opp_error("Could not allocate array rxSignal\n");
	for (int i = 0; i < numNodes; i++) {
		rxSignal[i] = new double* [numNodes];
		if (rxSignal[i]==NULL) opp_error("Could not allocate array rxSignal[%d]\n", i);
		for (int j = 0; j < numNodes; j++){
			rxSignal[i][j] = new double [numTxPowerLevels];
			if (rxSignal[i][j]==NULL) opp_error("Could not allocate array rxSignal[%d][%d]\n", i, j);
		}
	}

	cTopology *topo = new cTopology("topo");
	topo->extractByModuleType("Node", NULL);

	double x1, x2, y1, y2, dist;
	double PLd; // path loss at distance dist, in dB

	for (int i = 0; i < numNodes; i++)
		for (int j = 0; j < numNodes; j++) {
			if (i == j)
				for (int k = 0; k < numTxPowerLevels; k++)
					rxSignal[i][j][k] = 100;  // something very big, so you cannot receive anything while transmitting
			else {
				x1 = topo->node(i)->module()->par("xCoor");
				y1 = topo->node(i)->module()->par("yCoor");
				x2 = topo->node(j)->module()->par("xCoor");
				y2 = topo->node(j)->module()->par("yCoor");
				dist = sqrt((x2 - x1)*(x2 - x1) + (y2 - y1)*(y2 - y1));

				PLd = PLd0 + 10 * pathLossExponent * log10(dist/d0) + normal(0, sigma);

				// the Path Loss, **including the random part** remains
				// the same for different tx power levels, if we are
				// referring to the same receiver-transmitter pair.

				for (int k = 0; k < numTxPowerLevels; k++) {
					rxSignal[i][j][k] = txPowerLevels[k] - PLd;
				}
			}
		}
	delete(topo);

//debug info
//for (int i = 0; i < numNodes; i++) EV << "\n[WChannel]:\n rxSignal[" << i << "][9][0]= " << rxSignal[i][9][0];

	// Allocate and Initialize maxInterferenceForCurrentTx
	maxInterferenceForCurrentTx = new double* [numNodes];
	if (maxInterferenceForCurrentTx==NULL) opp_error("Could not allocate array maxInterferenceForCurrentTx\n");
	for (int i = 0; i < numNodes; i++) {
		maxInterferenceForCurrentTx[i] = new double [numNodes];
		if (maxInterferenceForCurrentTx[i]==NULL) opp_error("Could not allocate array maxInterferenceForCurrentTx[%d]\n", i);
	}
	for ( int i = 0; i < numNodes; i++)
		for (int j = 0; j < numNodes; j++)
			maxInterferenceForCurrentTx[i][j] = -200; // something very small, approaching 0 mW


	// Allocate and Initialize stats
	stats = new int* [numNodes];
	if (stats==NULL) opp_error("Could not allocate array stats\n");
	for (int i = 0; i < numNodes; i++) {
		stats[i] = new int [5];
		if (stats[i]==NULL) opp_error("Could not allocate array stats[%d]\n", i);
	}
	for ( int i = 0; i < numNodes; i++)
		for (int j = 0; j < 5; j++)
			stats[i][j] = 0;

	// Schedule a self message at the sim-time-limit so the simulation does
	// finish before the sim-time-limit
	scheduleAt(5, new cMessage);
}


void WirelessChannel::handleMessage(cMessage *msg) {

	//process the messages received
	WChannel_GenericMessage* message = dynamic_cast<WChannel_GenericMessage*>(msg);
	// Ignore the self message intended to prolong the sim time
	if (message == NULL) { delete msg; EV << "\n[WChannel]:\n Wireless Channel: Ignoring bogus message at time:" << simTime(); return;}

	ListElement *node, *node2;
	int srcAddr = message->getSrcAddress();
	//int destAddr = message->getDestAddress(); 	// this is not used for now
	int powerLevel, powerLevel2, srcPowerLevel;
	int tempAddr;
	double currentInterferencePower, SNR_dB, prob;


	switch (message -> kind()) {

		case WC_CARRIER_SENSE_BEGIN:
			node = new ListElement;
			node->index = srcAddr;
			carrierSensingNodes = addToFront(carrierSensingNodes, node);

			// Check the current carrier signal strength for this node
			// We adopt a simple testing: If any of the active nodes reaches
			// the carrier sensing node with power that exceeds the threshold SNR
			// we assume carrier is sensed. We do not add powers from different
			// nodes. TODO: Think of lowering the threshold by 1 or 2dB to account
			// for more sensitivity
			for (node = activeNodes; node != NULL; node = node -> next) { //iterate through the active list
				tempAddr = node->index;
				powerLevel = node->powerLevel;
				if ((powerLevel < 0) || (powerLevel >= numTxPowerLevels)) powerLevel = 0;  //assign the default value

				if (rxSignal[tempAddr][srcAddr][powerLevel] - noiseFloor > thresholdSNRdB) {
					// carrier sensed! send message to node.
					// Delay it by 0.1ms (the defined minimum carrier sense interval)
					sendDelayed( new cMessage("carrier signal sensed", WC_CARRIER_SENSED), 0.0001, "toNode", srcAddr);
					// no need to continue with the loop
					break;
				}
			}
			break;


		case WC_CARRIER_SENSE_END:
			// just delete this node from the list of carrierSensingNodes
			carrierSensingNodes = deleteListElement(carrierSensingNodes, srcAddr);
			break;


		case WC_PKT_BEGIN_TX:
			node = new ListElement;
			node->index = srcAddr;
			node->powerLevel = message->getPowerLevel();
			// add the transmitting node to the active list
			activeNodes = addToFront(activeNodes, node);
			// update the stats
			stats[srcAddr][0]++;

			// check whether carrier signal is sensed in the carrierSensingNodes due to this transmission
			// again we take a simple approach, we do not keep power seen before and add it.
			powerLevel = node->powerLevel;
			if ((powerLevel < 0) || (powerLevel >= numTxPowerLevels)) powerLevel = 0;  //assign the default value

			for (node = carrierSensingNodes; node != NULL; node = node -> next) { //iterate through the active list
				tempAddr = node->index;

				if (rxSignal[srcAddr][tempAddr][powerLevel] - noiseFloor > thresholdSNRdB) {
					// carrier sensed! send message to node.
					// Delay it by 0.1ms (the defined minimum carrier sense interval)
					sendDelayed( new cMessage("carrier signal sensed", WC_CARRIER_SENSED), 0.0001, "toNode", tempAddr);
				}
			}
			break;

		case WC_PKT_END_TX:

			// we calculate how much is the current interference to every transmission
			// from all the other transmissions (at the possible/eligible receivers)
			// and compare it with the max seen so far. This way we will know the max
			// experienced interference for each transmission when it ends and decide
			// on the reception packet prob (for each of the eligible receivers)

			for (node = activeNodes; node != NULL; node = node -> next) { //iterate through the active list
				tempAddr = node->index;
				powerLevel = node->powerLevel;
				if ((powerLevel < 0) || (powerLevel >= numTxPowerLevels)) powerLevel = 0;  //assign the default value
				if (tempAddr == srcAddr) srcPowerLevel = powerLevel; // set srcPowerLevel to be used later

				for (int i = 0; i < numNodes; i++) {
					if (rxSignal[tempAddr][i][powerLevel] - noiseFloor > thresholdSNRdB) {
						// got one potential receiver with index i
						// iterate once more through the active list to find the
						// interference to the transmision from node tempAddr "seen" at node i
						currentInterferencePower = -200; 	// a very small value in dBm
						for (node2 = activeNodes; node2 != NULL; node2 = node2 -> next) {
							// a node does not interfere with its own transmission
							if (node2 == node) continue;
							powerLevel2 = node2->powerLevel;
							if ((powerLevel2 < 0) || (powerLevel2 >= numTxPowerLevels)) powerLevel2 = 0;
							currentInterferencePower = addPower_dBm(rxSignal[node2->index][i][powerLevel2], currentInterferencePower);
						}
						if (maxInterferenceForCurrentTx[tempAddr][i] < currentInterferencePower)
							maxInterferenceForCurrentTx[tempAddr][i] = currentInterferencePower;
					}
				}
			}

			// It's the end of transmission from node srcAddr so we have to check based on the
			// rxSignal and the maxInterference experienced, which receivers get the transmission.

			for (int i = 0; i < numNodes; i++) {

				if (i == srcAddr) continue;  // we cannot receive our own transmission

				if (rxSignal[srcAddr][i][srcPowerLevel] - noiseFloor > thresholdSNRdB)
					// update stats
					stats[srcAddr][1]++;	// possiple reception
				else continue;				// if not possible reception, go to the next node (next i)

				SNR_dB = rxSignal[srcAddr][i][srcPowerLevel] - addPower_dBm(noiseFloor, maxInterferenceForCurrentTx[srcAddr][i]);
//EV << "\n[WChannel]:\n SNR_dB " << srcAddr << "-->" << i << " = " << SNR_dB << ", srcPowerLevel = " << srcPowerLevel << ", rxSignal = " << rxSignal[srcAddr][i][srcPowerLevel];
				if (SNR_dB > thresholdSNRdB) {
					// transmission has a good chance of being received, i.e. :
					// noise + max interference were not strong enough for packet to
					// be received at receiver i with prob < thresholdProb

					// Calculate the prob of packet reception
					prob = pow( 1- 0.5*exp(-0.781* pow(10, (SNR_dB/10))), (8*f) );   //  1/1.28 = 0.781
					// safeguarding
					//if (prob < (double)par("thresholdProb")) EV << "\n[WChannel]:\n Error: calculated prob =" << prob;
//EV << "\n[WChannel]:\n Prob " << srcAddr << "-->" << i << " = " << prob;
					// based on the above prob decide to send message to node or not
					if ( genk_dblrand(1) < prob ) {  // notice that we are using generator 1 not 0
						// Send it! we cannot send msg multiple times, we have to create copies
						cMessage *copy = (cMessage *) msg->dup();
						send(copy, "toNode", i);
						// update the stats, actual reception
						stats[srcAddr][3]++;
						stats[i][4]++;
					}
				}
				else stats[srcAddr][2]++;  // the transmission was interferred
			}

			// delete srcAddr from the active nodes and minimize the row in maxInterferenceForCurrentTx
			activeNodes = deleteListElement(activeNodes, srcAddr);
			for (int i = 0; i < numNodes; i++)
				maxInterferenceForCurrentTx[srcAddr][i] = -200; // a very small value in dBm

			break;

		default:
			EV << "\n[WChannel]:\n Warning: Wireless Channel received unknown message!";
	}

	delete msg;
}


void WirelessChannel::finish() {

	// output aggregate transmission/reception info
	// useful for debugging purposes
	for(int i = 0;  i < numNodes; i++){
		EV << "\n[WChannel]:\n Node[" << i << "]: transmissions= " << stats[i][0] << ", possible receptions= " << stats[i][1] << ", interferred receptions= " << stats[i][2] << ", actual receptions= " << stats[i][3];
		EV << "\n[WChannel]:\n Node[" << i << "]: received packets= " << stats[i][4];
	}

	// Delete dynamically allocated arrays
	// delete rxSignal
	for (int i = 0; i < numNodes; i++){
		for (int j = 0; j < numNodes; j++)
			delete[] rxSignal[i][j];			// the delete[] operator releases memory allocated with new []

		delete[] rxSignal[i];
	}
	delete[] rxSignal;

	//delete maxInterferenceForCurrentTx
	for (int i = 0; i < numNodes; i++)
		delete[] maxInterferenceForCurrentTx[i];
	delete[] maxInterferenceForCurrentTx;

	//delete stats
	for (int i = 0; i < numNodes; i++)
		delete[] stats[i];
	delete[] stats;

	// delete activeNodes elements
	for (ListElement *el = activeNodes; el != NULL; ) {
		activeNodes = el->next;
		delete el;
		el = activeNodes;
	}

	// delete carrierSensingNodes elements
	for (ListElement *el = carrierSensingNodes; el != NULL; ) {
		carrierSensingNodes = el->next;
		delete el;
		el = carrierSensingNodes;
	}

} 
