/****************************************************************************
 *  Copyright: National ICT Australia,  2007, 2008, 2009
 *  Developed at the ATP lab, Networked Systems theme
 *  Author(s): Athanassios Boulis, Dimosthenis Pediaditakis, Yuri Tselishchev
 *  This file is distributed under the terms in the attached LICENSE file.
 *  If you do not find this file, copies can be found by writing to:
 *
 *      NICTA, Locked Bag 9013, Alexandria, NSW 1435, Australia
 *      Attention:  License Inquiry.
 ****************************************************************************/


#ifndef _WIRELESSCHANNEL_H
#define _WIRELESSCHANNEL_H


#include "WirelessChannelMessages_m.h"
#include "WCsupportFunctions.h"
#include "WirelessChannelTemporal.h"
#include "MacGenericFrame_m.h"
#include "DebugInfoWriter.h"
#include "VirtualMobilityModule.h"
#include "cstrtokenizer.h"
#include "time.h"
#include <omnetpp.h>
#include <math.h>
#include <list>
using namespace std;

#define EV ev.disabled()?(ostream&)ev:ev
#define CASTALIA_DEBUG (!printDebugInfo)?(ostream&)DebugInfoWriter::getStream():DebugInfoWriter::getStream()

#define BAD_LINK_PROB_THRESHOLD  0.05
#define GOOD_LINK_PROB_THRESHOLD 0.98
#define IDEAL_MODULATION_THRESHOLD_SNR_DB 5.0

#define CS_DETECTION_DELAY  0.0001 // delay for carrier sensing

enum CollisionModel
{
	NO_INTERFERENCE_NO_COLLISIONS = 8100,
	SIMPLE_COLLISION_MODEL = 8101,
	ADDITIVE_INTERFERENCE_MODEL = 8102,
	COMPLEX_INTERFERENCE_MODEL = 8103
};


enum modulationTypeAvail
{
	MODULATION_FSK = 8104,
	MODULATION_PSK = 8105,
	MODULATION_IDEAL = 8106,
	MODULATION_CUSTOM = 8107
};


enum encodingTypeAvail
{
	ENCODING_NRZ = 8108,
	ENCODING_4B5B = 8109,
	ENCODING_MANCHESTER = 8110,
	ENCODING_SECDEC = 8111
};


class PathLossElement {
	public:
		int cellID;
		float avgPathLoss;
		float lastObservedDiffFromAvgPathLoss;
		simtime_t lastObservationTime;

		PathLossElement(int c, float PL){
			cellID = c;
			avgPathLoss = PL;
			lastObservedDiffFromAvgPathLoss = PL;
			lastObservationTime = 0.0;
		};

		~PathLossElement(){};

};

class ReceivedSignalElement {
	public:
		int transmitterID;
		float signal;
		float currentInterference;
		float maxInterference;

		ReceivedSignalElement(int ID, float receivedSignal, float interference) {
			transmitterID = ID;
			signal = receivedSignal;
			currentInterference = interference;
			maxInterference = interference;
		};

		~ReceivedSignalElement(){};
};

struct TxRxStatistics_type {
    int transmissions;				// num of Tranmissions at the node.
    								// The Tx* variables below refer to possible RECEPTIONS
    								// at OTHER nodes caused by the above Transmissions
    int TxReachedNoInterference;	// packets reached
    int TxReachedInterference;  	// packets reached despite interference
    int TxFailedNoInterference;		// packets failed even without interference
    int TxFailedInterference;		// packets failed with interference
	int TxFailedTemporalFades;		// packets failed due to temporal fades

									// The Rx* variables below refer to RECEPTIONS at the self node
    int RxReachedNoInterference;	// packets reached
    int RxReachedInterference;  	// packets reached despite interference
    int RxFailedNoInterference;		// packets failed even without interference
    int RxFailedInterference;		// packets failed with interference


	/* initialize the struct (C++ syntax)*/
    TxRxStatistics_type() : transmissions(0), TxReachedNoInterference(0), TxReachedInterference(0),
	TxFailedNoInterference(0), TxFailedInterference(0), TxFailedTemporalFades(0), RxReachedNoInterference(0),
	RxReachedInterference(0), RxFailedNoInterference(0), RxFailedInterference(0)  {}
};

struct CustomModulation_type {
    float SNR;
    float BER;
};

class WirelessChannel : public cSimpleModule {
	private:

		/*--- variables corresponding to .ned file's parameters ---*/
		bool printStatistics;
		bool printDebugInfo;
		int numOfNodes;

		double xFieldSize;
		double yFieldSize;
		double zFieldSize;
		double xCellSize;
		double yCellSize;
    		double zCellSize;

		// variables corresponding to Wireless Channel module parameters
		double pathLossExponent;	// the path loss exponent
		double noiseFloor;			// in dBm
		double PLd0;				// Power loss at a reference distance d0 (in dBm)
		double d0;					// reference distance (in meters)
		int collisionModel;
		double sigma;				// std of a zero-mean Gaussian RV
		double bidirectionalSigma;	// std of a zero-mean Gaussian RV

		const char * pathLossMapFile;
		const char * PRRMapFile;
		const char * temporalModelParametersFile;
		const char * modulationTypeParam;

    		bool onlyStaticNodes;

		// variables corresponding to Radio module parameters
		double dataRate;
		double noiseBandwidth;
		int modulationType;
		CustomModulation_type *customModulationArray;
		int numOfCustomModulationValues;
		int encodingType;
		double receiverSensitivity;
		double maxTxPower;	// this is derived, by reading all the Tx power levels

		/*--- other class member variables ---*/

		int numOfXCells, numOfYCells, numOfZCells;
		int numOfSpaceCells;
		int xIndexIncrement, yIndexIncrement, zIndexIncrement;


		list<PathLossElement *> *pathLoss;		// an array of lists (numOfSpaceCels long)
								// holding info on path loss. Element i of the
								// array is a list elements that describe which
								// cells are affected (and how) when a
								// node in cell i transmits.

		list<ReceivedSignalElement *> *currentSignalAtReceiver;	// an array of lists (numOfNodes long).
																// array element i holds info (as a list)
																// about the signals and interference
																// received at node i.

		list<int> *nodesAffectedByTransmitter;	// an array of lists (numOfNodes long). The list
												// at array element i holds the node IDs that are
												// affected when node i transmits.

		list<int> *cellOccupation;				// an array of lists (numOfSpaceCels long) that
												// tells us which nodes are in cell i.

		NodeLocation_type *nodeLocation;		// an array (numOfNodes long) that gives the
												// location for each node.

		bool *isNodeCarrierSensing;				// an array (numOfNodes long) that tells us
												// whether node i is carrier sensing or not.

		float thresholdSNRdB;					// below this SNR (in dB) we assume reception in not possible
		float goodLinkSNRdB;					// above this SNR (in dB) we assume reception is very possible


		TxRxStatistics_type *stats;				// an array of stats for every node

		int minPacketSize;
		int maxPacketSize;

		bool temporalModelDefined;
		channelTemporalModel * temporalModel;

	protected:
		virtual void initialize();
		virtual void handleMessage(cMessage *msg);
		virtual void finish();

		void readIniFileParameters(void);
		void parsePathLossMap(void);
		void parsePrrMap(void);
		int parseInt(const char *, int *);
		int parseFloat(const char *, float *);
		void printRxSignalTable(void);
		void updatePathLossElement(int,int,float);
		float calculateProb(float, int);
};


#endif //_WIRELESSCHANNEL_H
