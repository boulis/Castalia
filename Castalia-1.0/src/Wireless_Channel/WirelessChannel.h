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




#ifndef _WIRELESSCHANNEL_H_
#define _WIRELESSCHANNEL_H_


#include "WChannelGenericMessage_m.h"
#include <omnetpp.h>
using namespace std;

typedef struct listel {
	int index;
	int powerLevel;
	struct listel *next;
} ListElement;



class WirelessChannel : public cSimpleModule {
	private:
		// parameters and variables
		int numNodes;				// the number of nodes
		double noiseFloor;			// in dBm
		double pathLossExponent;	// the path loss exponent
		double sigma;				// std of a zero-mean Gaussian RV
		double PLd0;				// Power loss at a reference distance d0 (in dBm)
		double d0;					// reference distance (in metre)
		double f;					// frame size (in Bytes)
		double thresholdSNRdB;		// below this SNR (in dB) we assume reception in not possible
		int numTxPowerLevels;		// the number of different Tx Power levels (max is 15)
		double txPowerLevels[15];	// the array holding the different Tx PowerLevels,
									// txPowerLevel[0] is the default

		double ***rxSignal;			// N by N by L array stores the received signal power (in dB)
									// N = numNodes, L = numTxPowerLevels
									// rxSignal[i][j][k] is the received signal power at node j
		                   			// when node i is transmitting with TxPowerLevel k

		double **maxInterferenceForCurrentTx;   // maxInterferenceForCurrentTx[i][j] is the interference
												// power (in dBm) witnessed by node j for the *current*
												// transmission of node i. Array N by N.
												// The array needed to be only A by N, where A is the number
												// of active (transmitting) nodes but N by N helps with
												// fast indexing

		ListElement *activeNodes;				// a linked-list containing active (transmitting) nodes

		ListElement *carrierSensingNodes;		// a linked-list containing nodes who are carrier sensing

		int **stats;							// N by 5, holds various statistics:
												// stats[i][0] is transmissions from node i
												// stats[i][1] is the possible receptions for the transmissions of node i
												// stats[i][2] is the interferred receptions for the transmissions of node i
												// stats[i][3] is the actual receptions for the transmissions of node i
												// stats[i][4] is the receptions of node i

	protected:
		virtual void initialize();
		virtual void handleMessage(cMessage *msg);
		virtual void finish();
		
		ListElement *addToFront(ListElement *n, ListElement *newListElement);
		ListElement *deleteListElement(ListElement* start, int ind);
		double addPower_dBm(double a, double b)const;
		
};


#endif //_WIRELESSCHANNEL_H_
