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

#define ERFINV_ERROR 100000.0f

#define THESHOLD_PROB 0.05

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


double WirelessChannel::erfInv( double y ) const
{
	static double a[] = {0,  0.886226899, -1.645349621,  0.914624893, -0.140543331 };
	static double b[] = {0, -2.118377725,  1.442710462, -0.329097515,  0.012229801 };
	static double c[] = {0, -1.970840454, -1.624906493,  3.429567803,  1.641345311 };
	static double d[] = {0,  3.543889200,  1.637067800 };
	
	double x, z;
  
 	if ( y > -1. ) 
  	{
  		if ( y >= -.7 ) 
  		{
  			if ( y <= .7 ) 
  			{
				z = y*y;
				x = y * (((a[4]*z+a[3])*z+a[2])*z+a[1]) / ((((b[4]*z+b[3])*z+b[2])*z+b[1])*z+1);
	  		}
	  		else
	  		if ( y < 1 ) 
	  		{
				z = sqrt(-log((1-y)/2));
				x = (((c[4]*z+c[3])*z+c[2])*z+c[1]) / ((d[2]*z+d[1])*z+1);
	  		}
	  		else 
	  			return ERFINV_ERROR;
		}
		else
		{
			z = sqrt(-log((1+y)/2));
	  		x = -(((c[4]*z+c[3])*z+c[2])*z+c[1]) / ((d[2]*z+d[1])*z+1);
		}
	}
	else 
		return ERFINV_ERROR;
  	
  	return x;
}


double WirelessChannel::erfcInv( double y )const
{
	double result;
	result  = erfInv(1-y);
  	return result;
}


void WirelessChannel::initialize() {
	readIniFileParameters();
	
	// calculate thresholdSNRdB using the parameter thresholdProb
	// this is for specific coding and modulating schemes! TODO: extend to other schemes
	//thresholdSNRdB = receiverSensitivity - noiseFloor;
	/*****  OLD CALCULATION OF thresholdSNRdB  */
	if(modulationType == MODULATION_FSK)
		thresholdSNRdB = 10.0f * log10(   (-2.0f*(dataRate/noiseBandwidth)) * log( 2.0f * (1.0f -  pow(THESHOLD_PROB, 1.0f/(8.0f*minumumPacketSize))) )   );
	else // (modulationType == MODULATION_PSK)
		thresholdSNRdB = 10.0f * log10(   (dataRate/noiseBandwidth) * pow(erfcInv(2.0f * ( 1.0f - pow(THESHOLD_PROB, 1.0f/(8.0f*minumumPacketSize)) )), 2.0f)   );
	
	
	
	/****************************************************************************************
	 * Calculation of the Bn for Telos motes.
	 * Based on the given values of Bn an R for MICA we find the BER.
	 * We then apply this BER in the BER formula for Telos and we solve the equation having
	 * as unknown the Bn.
	 * noise floor = -105
	 * MICA2: sensitivity = -98, Bn=30, R=19.2
	 * TELOS: senstivity =  -94, Bn=??, R=250
	 ****************************************************************************************
	// Calculate the MICA2 BER  (SNRdb = -98-(-105) = 7)
	double mySNRdB = 7.0f;
	double BER = 0.5f * exp( (-0.5f) * (30.0f / 19.2f) * pow(10.0f, (mySNRdB/10.0f)) );
	printf("\n**** Mica2 BER=%lf *****\n", BER);
	// Calculate the Telos Bn   (SNRdb = -94-(-105) = 11)
	mySNRdB = 11.0f;
	double Bn = (250.0f/pow(10.0f, mySNRdB/10.0f)) * pow( erfcInv(2.0f*BER), 2.0f);
	opp_error("\n*** Telos Bn=%lf **\n", Bn);
	//-----------------------------------------------------------------------------------------
	*/
	
	
	//opp_error("\n\n---%lf---\n\n---%lf---\n", thresholdSNRdB, thresholdSNRdB_2);
	/*double thresholdA;
	double probB;
	for(double tempThreshold=1.0f; tempThreshold<=13.7f; tempThreshold+=0.025f)
	{
		//double xxx = erfc(sqrt(pow(10,(tempThreshold/10)) * noiseBandwidth / dataRate));
		probB = pow( 1- 0.5 * erfc(  sqrt(pow(10,(tempThreshold/10)) * noiseBandwidth / dataRate)  ),  (8*minumumPacketSize));
		//double tt = erfcInv(2 * ( 1 - pow(probB, 1/(8*minumumPacketSize)) ));
		//if(tt == ERFINV_ERROR)
		//	opp_error("\n\!!!!!!!!!!!!!!!!!!!!!!!\n\n");
		thresholdA = 10 * log10(   (dataRate/noiseBandwidth) * pow(erfcInv(2 * ( 1 - pow(probB, 1/(8*minumumPacketSize)) )), 2)   );
		
		printf("\n[prob=%1.30lf]\nReal Value(SNRdB): \t%1.15lf\nBiased Value(SNRdB): \t%1.15lf\n", probB, tempThreshold, thresholdA);
	}
	opp_error("\n****\n");*/
	//thresholdSNRdB = 10*log10( -1.28*log(2*(1 -  pow((double)par("thresholdProb"), 1/(8*minumumPacketSize))   )));
	
	
	// activeNodes is empty
	activeNodes = NULL;
	
	//carrierSensingNodes is empty
	carrierSensingNodes = NULL;

	//EV << "\n[WChannel]:\n Wireless Channel Initialization - numNodes= " << numNodes << ", numTxPowerLevels= "  << numTxPowerLevels;
	//EV << "\nthresholdSNRdB = " << thresholdSNRdB;


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
//printf("\nNode[%d]  x=%f, y=%f\n", i, x1, y1);
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
	
	//printRxSignalTable();
	parseRxPowerMap();
	parsePrrMap();
	//printRxSignalTable();


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
		stats[i] = new int [6];
		if (stats[i]==NULL) opp_error("Could not allocate array stats[%d]\n", i);
	}
	for ( int i = 0; i < numNodes; i++)
		for (int j = 0; j < 6; j++)
			stats[i][j] = 0;


	// Schedule a self message at the sim-time-limit so the simulation does
	// finish before the sim-time-limit
	scheduleAt(ev.config()->getAsTime("General", "sim-time-limit"), new cMessage);
}


void WirelessChannel::handleMessage(cMessage *msg) {

	//process the messages received
	WChannel_GenericMessage* message = dynamic_cast<WChannel_GenericMessage*>(msg);
	// Ignore the self message intended to prolong the sim time
	if (message == NULL) { delete msg; EV << "\n[WChannel]:\n Wireless Channel: Ignoring bogus message at time:" << simTime(); return;}

	ListElement *node, *node2;
	int srcAddr = message->getSrcAddress();
	int packetByteSize = message->byteLength();
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

				if (rxSignal[tempAddr][srcAddr][powerLevel] > receiverSensitivity) {
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

			for (node = activeNodes; node != NULL; node = node -> next) 
			{   //iterate through the active list
				tempAddr = node->index;
				
				powerLevel = node->powerLevel;
				
				if ((powerLevel < 0) || (powerLevel >= numTxPowerLevels)) 
					powerLevel = 0;  //assign the default value
					
				if (tempAddr == srcAddr) 
					srcPowerLevel = powerLevel; // set srcPowerLevel to be used later

				
				for (int i = 0; i < numNodes; i++)
				{
					
					if (rxSignal[tempAddr][i][powerLevel] - noiseFloor > thresholdSNRdB) 
					{
						// got one potential receiver with index i
						// iterate once more through the active list to find the
						// interference to the transmision from node tempAddr "seen" at node i
						currentInterferencePower = -200; 	// a very small value in dBm
						for (node2 = activeNodes; node2 != NULL; node2 = node2 -> next)
						{
							// a node does not interfere with its own transmission
							if (node2 == node) continue;
							
							powerLevel2 = node2->powerLevel;
							
							if ((powerLevel2 < 0) || (powerLevel2 >= numTxPowerLevels)) 
								powerLevel2 = 0;
							
							currentInterferencePower = addPower_dBm(rxSignal[node2->index][i][powerLevel2], currentInterferencePower);
						}//for_node2
						
						if (maxInterferenceForCurrentTx[tempAddr][i] < currentInterferencePower)
							maxInterferenceForCurrentTx[tempAddr][i] = currentInterferencePower;
					}
					
				}//for_i
			}

			// It's the end of transmission from node srcAddr so we have to check based on the
			// rxSignal and the maxInterference experienced, which receivers get the transmission.

			for (int i = 0; i < numNodes; i++) {

				if (i == srcAddr) continue;  // we cannot receive our own transmission

				if (rxSignal[srcAddr][i][srcPowerLevel] > receiverSensitivity)
					// update stats
					stats[srcAddr][1]++;	// possible reception
				else 
					continue;				// if not possible reception, go to the next node (next i)

				SNR_dB = rxSignal[srcAddr][i][srcPowerLevel] - addPower_dBm(noiseFloor, maxInterferenceForCurrentTx[srcAddr][i]);
//printf("\n***** %lf ******\n", rxSignal[srcAddr][i][srcPowerLevel]);				
				
//EV << "\n[WChannel]:\n SNR_dB " << srcAddr << "-->" << i << " = " << SNR_dB << ", srcPowerLevel = " << srcPowerLevel << ", rxSignal = " << rxSignal[srcAddr][i][srcPowerLevel];
				if (SNR_dB > thresholdSNRdB) {
					// transmission has a good chance of being received, i.e. :
					// noise + max interference were not strong enough for packet to
					// be received at receiver i with prob < thresholdProb

					// Calculate the prob of packet reception
					if(modulationType == MODULATION_FSK)
						prob = pow( 1- 0.5f * exp( ((-0.5f) * noiseBandwidth / dataRate)* pow(10.0f, (SNR_dB/10.0f))), (8.0f*packetByteSize) );   //  1/1.28 = 0.781
					else // (modulationType == MODULATION_PSK)
						prob = pow( 1- 0.5f * erfc( sqrt(pow(10.0f,(SNR_dB/10.0f)) * noiseBandwidth / dataRate)  ),  (8.0f*packetByteSize));
						
/*printf("\nSNRdB, Bn=50, Bn=60, Bn=70, Bn=150, Bn=200, Bn=300, Bn=400, Bn=500\n");
for(double j=5; j<=20; j+=0.2)
{
	double probA = pow( 1- 0.5f * erfc( sqrt(pow(10.0f,(j/10.0f)) * 50.0f / dataRate)  ),  (8.0f*packetByteSize));
	
	double probB = pow( 1- 0.5f * erfc( sqrt(pow(10.0f,(j/10.0f)) * 60.0f / dataRate)  ),  (8.0f*packetByteSize));
	
	double probC = pow( 1- 0.5f * erfc( sqrt(pow(10.0f,(j/10.0f)) * 70.0f / dataRate)  ),  (8.0f*packetByteSize));
	
	double probD = pow( 1- 0.5f * erfc( sqrt(pow(10.0f,(j/10.0f)) * 150.0f / dataRate)  ),  (8.0f*packetByteSize));
	
	double probE = pow( 1- 0.5f * erfc( sqrt(pow(10.0f,(j/10.0f)) * 200.0f / dataRate)  ),  (8.0f*packetByteSize));
	
	double probF = pow( 1- 0.5f * erfc( sqrt(pow(10.0f,(j/10.0f)) * 300.0f / dataRate)  ),  (8.0f*packetByteSize));
	
	double probG = pow( 1- 0.5f * erfc( sqrt(pow(10.0f,(j/10.0f)) * 400.0f / dataRate)  ),  (8.0f*packetByteSize));
	
	double probH = pow( 1- 0.5f * erfc( sqrt(pow(10.0f,(j/10.0f)) * 500.0f / dataRate)  ),  (8.0f*packetByteSize));
	
	printf("%lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf\n", j, probA, probB, probC, probD, probE, probF, probG, probH);
}						
opp_error("!!!");	*/				
						
//printf("\n***** %1.25lf ******\n", prob);
					//printf("\n--%lf--\n--%lf--\n", prob, prob2);
					//double prob2 = pow( 1- 0.5 * exp( ((-0.5) * noiseBandwidth / dataRate)* pow(10, (SNR_dB/10))), (8*minumumPacketSize) );   //  1/1.28 = 0.781
					//prob = pow( 1- 0.5*exp(-0.781* pow(10, (SNR_dB/10))), (8*minumumPacketSize) );   //  1/1.28 = 0.781
					//printf("\n--%lf--\n--%lf--\n", prob, ((-0.5) * noiseBandwidth / dataRate));
						
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
				else 
				{
					stats[srcAddr][2]++;  // the transmission was interferred
					stats[i][5]++;
				}
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

	if(printStatistics)
	{
		// output aggregate transmission/reception info
		// useful for debugging purposes
		for(int i = 0;  i < numNodes; i++)
		{
			EV << "\n\n## WChannel --> Node[" << i << "] ##";
			EV << "\n - transmissions = " << stats[i][0];
			EV << "\n     These transmissions resulted in " << stats[i][1] << " possible receptions at other nodes.";
			EV << "\n     From these possible receptions:";
			EV << "\n            " << stats[i][2] << " were interferred and";
			EV << "\n            " << stats[i][3] << " were strong enough to be received (radio and MAC states allowing).";
			EV << "\n - receptions = " << stats[i][4] << " (packets strong enough to be received (radio and MAC states allowing)";
			EV << "\n - collisions = " << stats[i][5] << " (packets intented for Node[" << i << "] that got interferred)\n";
		}
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



void WirelessChannel::readIniFileParameters(void)
{
	printStatistics = par("printStatistics");

	numNodes = parentModule()->par("numNodes");
	
	noiseFloor = par("noiseFloor");
	
	pathLossExponent = par("pathLossExponent");
	
	sigma = par("sigma");
	
	PLd0 = par("PLd0");
	
	d0 = par("d0");
	
	rxSignal_ConnectivityMap = par("rxSignal_ConnectivityMap");
	
	PRR_ConnectivityMap = par("PRR_ConnectivityMap");
	
	//thresholdProb = par("thresholdProb");
	
	//find the minimum packet size
	cModule *node_0;
	if(parentModule()->findSubmodule("node", 0) != -1)
		node_0 = check_and_cast<cModule*>(parentModule()->submodule("node", 0));
	else
		opp_error("\n[Wireless Channel]:\n Error in geting a valid reference to  node[0].\n");
		
	if( node_0->findSubmodule("nodeApplication")  &&
		node_0->findSubmodule("networkInterface") &&
		node_0->submodule("networkInterface")->findSubmodule("MAC") &&
		node_0->submodule("networkInterface")->findSubmodule("Radio") )
	{
		int appOverhead = node_0->submodule("nodeApplication")->par("packetHeaderOverhead");
		int macOverhead = node_0->submodule("networkInterface")->submodule("MAC")->par("macFrameOverhead");
		int radioOverhead = node_0->submodule("networkInterface")->submodule("Radio")->par("phyFrameOverhead");
		minumumPacketSize = (double)(appOverhead + macOverhead + radioOverhead + 2);
		
		
		dataRate = node_0->submodule("networkInterface")->submodule("Radio")->par("dataRate");
		
		receiverSensitivity = node_0->submodule("networkInterface")->submodule("Radio")->par("receiverSensitivity");
		
		noiseBandwidth = node_0->submodule("networkInterface")->submodule("Radio")->par("noiseBandwidth");


		modulationType = node_0->submodule("networkInterface")->submodule("Radio")->par("modulationType");
		if(modulationType == 0)
			modulationType = MODULATION_FSK;
		else
		if(modulationType == 1)
			modulationType = MODULATION_PSK;
		else
			opp_error("\n[Wireless Channel]: Illegal value of parameter \"modulationType\" in omnetpp.ini\n" );
		
		
		
		encodingType = node_0->submodule("networkInterface")->submodule("Radio")->par("encodingType");
		switch(encodingType)
		{
			case 0:
			{
				encodingType = ENCODING_NRZ;
				break;
			}
			
			case 1:
			{
				opp_error("\n[Wireless Channel]: Illegal value of parameter \"encodingType\" in omnetpp.ini. ENCODING_4B5B is not supported yet.\n" );
				break;
			}
			
			case 2:
			{
				opp_error("\n[Wireless Channel]: Illegal value of parameter \"encodingType\" in omnetpp.ini. ENCODING_MANCHESTER is not supported yet.\n" );
				break;
			}
			
			case 3:
			{
				opp_error("\n[Wireless Channel]: Illegal value of parameter \"encodingType\" in omnetpp.ini. ENCODING_SECDEC is not supported yet.\n" );
				break;
			}
			
			default:
			{
				opp_error("\n[Wireless Channel]: Illegal value of parameter \"encodingType\" in omnetpp.ini.\n" );
				break;
			}
		}
		
		// extract the txPowerLevels from the string module parameter
		const char *str = node_0->submodule("networkInterface")->submodule("Radio")->par("txPowerLevels");
		cStringTokenizer tokenizer(str);
		const char *token;
		numTxPowerLevels = 0;
		while ((token = tokenizer.nextToken())!=NULL){
			txPowerLevels[numTxPowerLevels] = atof(token);   // convert and store
			numTxPowerLevels++;
		}
		
		defaultPowerLevel = node_0->submodule("networkInterface")->submodule("Radio")->par("txPowerLevelUsed");
	}
	else
		opp_error("\n[Wireless Channel]:\n Error in geting a valid reference to  node[0] Application OR NetworkInterface (MAC/Radio) module.\n");
	
	
	
	collisionModel = par("collisionModel");
	switch(collisionModel)
	{
		case 0:
		{
			collisionModel = NO_COLLISIONS;
			break;
		}
		
		case 1:
		{
			collisionModel = SIMPLE_COLLISION_MODEL; 
			break;
		}
		
		case 2:
		{
			collisionModel = ADDITIVE_INTERFERENCE_MODEL;
			break;
		}
		
		case 3:
		{
			collisionModel = COMPLEX_INTERFERENCE_MODEL;
			break;
		}
		
		default:
		{
			opp_error("\n[Wireless Channel]: Illegal value of parameter \"collisionModel\" in omnetpp.ini\n" );
			break;
		}
	}
}


void WirelessChannel::parseRxPowerMap(void)
{
	cStringTokenizer valuesTokenizer(rxSignal_ConnectivityMap, ",");
	
	double * fields = new double [5];
	
	while (valuesTokenizer.hasMoreTokens()) 
	{
		const char * next = valuesTokenizer.nextToken();
		
		if(next == NULL)		
			continue;
		
		string token(next);
		
	
		if(token.size() <= 2)
			continue;
		
		string::size_type posA = token.find("(", 0);
		string::size_type posB = token.find(")", 0);
		string::size_type posC = token.find(">", 0); 
		string::size_type posD = token.find("=", 0);  
		
			 
		if( (posC == string::npos) || (posD == string::npos) || (( posA != string::npos) && (posB == string::npos)) )
			opp_error("\nError!(A) Illegal parameter format \"rxPowerMap\" of WirelessChannel module.\n");
		
		
		cStringTokenizer fieldsTokenizer(token.c_str(), ">()=");
		
		
		int loop = 0;
		
		
		while(fieldsTokenizer.hasMoreTokens())
		{				
			string field(fieldsTokenizer.nextToken());
		
			string::size_type fieldSize = field.length();
			
			if(fieldSize >= 1)
			{
				fields[loop] = atof(field.c_str());
				//printf("%f # ", fields[loop]);
				loop++;
			}
			
		}
		//printf("\n");
			
		if( ( posA != string::npos) && (loop < 4))
			opp_error("\nError!(B) Illegal parameter format \"rxPowerMap\" of WirelessChannel module.\n");
		
		if( (fields[0] < 0) || (fields[0] >= numNodes) || (fields[1] < 0) || (fields[1] >= numNodes) )
			opp_error("\nError!(C) Illegal parameter format \"rxPowerMap\" of WirelessChannel module.\n");
		
		if(loop == 3)
		{
			for (int k = 0; k < numTxPowerLevels; k++) 
				rxSignal[(int)fields[0]][(int)fields[1]][k] = fields[2] - txPowerLevels[k];
		}
		else
		if(loop == 4)
		{
			if( (fields[2] >= numTxPowerLevels) || (fields[2] < 0))
				opp_error("\nError!(D) Illegal parameter format \"rxPowerMap\" of WirelessChannel module.\n");
				
			rxSignal[(int)fields[0]][(int)fields[1]][(int)fields[2]] = fields[3] - txPowerLevels[defaultPowerLevel];
		}
		else
		{
			opp_error("\nError!(E) Illegal parameter format \"rxPowerMap\" of WirelessChannel module.\n");
		}

	}
	
	delete [] fields;
}


void WirelessChannel::parsePrrMap(void)
{
	cStringTokenizer valuesTokenizer(PRR_ConnectivityMap, ",");
	
	double * fields = new double [5];
	
	double meanPacketSize = minumumPacketSize;
	
	while (valuesTokenizer.hasMoreTokens()) 
	{
		const char * next = valuesTokenizer.nextToken();
		
		if(next == NULL)		
			continue;
		
		string token(next);
	
		if(token.size() <= 2)
			continue;
		
		
		string::size_type posA = token.find("(", 0);
		string::size_type posB = token.find(")", 0);
		string::size_type posC = token.find(">", 0); 
		string::size_type posD = token.find("=", 0);
		string::size_type posE = token.find("<", 0);  
		
		
		if( posE != string::npos)
		{
			meanPacketSize = atof((token.substr(1, token.size()-1)).c_str());
			continue;
		}
		
		
		if( (posC == string::npos) || (posD == string::npos) || (( posA != string::npos) && (posB == string::npos)) )
			opp_error("\nError!(A) Illegal parameter format \"rxPowerMap\" of WirelessChannel module.\n");
			
		
		cStringTokenizer fieldsTokenizer(token.c_str(), ">()=");
		
		
		int loop = 0;
		
		
		while(fieldsTokenizer.hasMoreTokens())
		{				
			string field(fieldsTokenizer.nextToken());
		
			string::size_type fieldSize = field.length();
			
			if(fieldSize >= 1)
			{
				fields[loop] = atof(field.c_str());
				//printf("%f # ", fields[loop]);
				loop++;
			}
			
		}
		//printf("\n");
			
		if( ( posA != string::npos) && (loop < 4))
			opp_error("\nError!(B) Illegal parameter format \"rxPowerMap\" of WirelessChannel module.\n");
		
		if( (fields[0] < 0) || (fields[0] >= numNodes) || (fields[1] < 0) || (fields[1] >= numNodes) )
			opp_error("\nError!(C) Illegal parameter format \"rxPowerMap\" of WirelessChannel module.\n");
		
		if(loop == 3)
		{
			double tmpSNRdB;
			if(modulationType == MODULATION_FSK)
				tmpSNRdB = 10.0f * log10(   (-2.0f*(dataRate/noiseBandwidth)) * log( 2.0f * (1.0f -  pow(fields[2], 1.0f/(8.0f*meanPacketSize))) )   );
			else // (modulationType == MODULATION_PSK)
				tmpSNRdB = 10.0f * log10(   (dataRate/noiseBandwidth) * pow(erfcInv(2.0f * ( 1.0f - pow(fields[2], 1.0f/(8.0f*meanPacketSize)) )), 2.0f)   );
			
			for (int k = 0; k < numTxPowerLevels; k++) 
				rxSignal[(int)fields[0]][(int)fields[1]][k] = tmpSNRdB + noiseFloor;
		}
		else
		if(loop == 4)
		{
			double tmpSNRdB;
			if(modulationType == MODULATION_FSK)
				tmpSNRdB = 10.0f * log10(   (-2.0f*(dataRate/noiseBandwidth)) * log( 2.0f * (1.0f -  pow(fields[3], 1.0f/(8.0f*meanPacketSize))) )   );
			else // (modulationType == MODULATION_PSK)
				tmpSNRdB = 10.0f * log10(   (dataRate/noiseBandwidth) * pow(erfcInv(2.0f * ( 1.0f - pow(fields[3], 1.0f/(8.0f*meanPacketSize)) )), 2.0f)   );
			
			if( (fields[2] >= numTxPowerLevels) || (fields[2] < 0))
				opp_error("\nError!(D) Illegal parameter format \"rxPowerMap\" of WirelessChannel module.\n");
				
			rxSignal[(int)fields[0]][(int)fields[1]][(int)fields[2]] = tmpSNRdB + noiseFloor;
		}
		else
		{
			opp_error("\nError!(E) Illegal parameter format \"rxPowerMap\" of WirelessChannel module.\n");
		}

	}
	
	delete [] fields;
}


void WirelessChannel::printRxSignalTable(void)
{
	for (int i = 0; i < numNodes; i++)
	{	
		for (int j = 0; j < numNodes; j++)
		{		
			printf("  [");
			for (int k = 0; k < numTxPowerLevels; k++)
			{
				printf("%f ", rxSignal[i][j][k]);
			}
			printf("]  ");
		}
		printf("\n");
	}
	printf("\n\n\n");
}
