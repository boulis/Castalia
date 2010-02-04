/********************************************************************************
 *  Copyright: National ICT Australia, 2009, 2010				*
 *  Developed at the Networked Systems theme, ATP lab				*
 *  Author(s): Athanassios Boulis, Yuriy Tselishchev				*
 *  This file is distributed under the terms in the attached LICENSE file.	*
 *  If you do not find this file, copies can be found by writing to:		*
 *										*
 *      NICTA, Locked Bag 9013, Alexandria, NSW 1435, Australia			*
 *      Attention:  License Inquiry.						*
 ********************************************************************************/


#ifndef _RADIOMODULE_H_
#define _RADIOMODULE_H_

#include <map>
#include <list>
#include <queue>
#include <omnetpp.h>
#include <iostream>
#include <fstream>

#include "RadioSupportFunctions.h"
#include "WirelessChannelMessages_m.h"
#include "RadioControlMessage_m.h"
#include "MacGenericPacket_m.h"
#include "ResourceGenericManager.h"

#define ALL_ERRORS -1
#define IDEAL_MODULATION_THRESHOLD 5.0

using namespace std;

enum Modulation_type {
	CUSTOM = 0,
	IDEAL = 1,
	FSK = 2,
	PSK = 3,
	DIFFBPSK = 4,
	DIFFQPSK = 5
};

enum CollisionModel_type {
	NO_INTERFERENCE_NO_COLLISIONS = 0,
	SIMPLE_COLLISION_MODEL = 1,
	ADDITIVE_INTERFERENCE_MODEL = 2,
	COMPLEX_INTERFERENCE_MODEL = 3
};

enum Encoding_type {
	NRZ = 0,
	CODE_4B5B = 1,
	MANCHESTER = 2,
	SECDEC = 3
};


struct RXmode_type {
	string name;
	double datarate;
	int bitsPerSymbol;
	Modulation_type modulation;
	double sensitivity;
	double bandwidth;
	double noiseBandwidth;
	double noiseFloor;
	double power;
};

struct ReceivedSignal_type {
	int ID;			 	// an ID to distinguish between signals, in single radio nodes the nodeID will suffice
	double power_dBm;   // in dBm
	Modulation_type modulation;
	Encoding_type encoding;
	double currentInterference;	//in dBm
	double maxInterference;	//in dBm
	int bitErrors; 		// number of bits with errors
};

struct TotalPowerReceived_type {
	double power_dBm;;	// in dBm
	simtime_t startTime;
};

struct TransitionElement {
	double delay;	// in ms
	double power;	// in mW
};

struct SleepLevel_type {
	string name;
	double power;
	TransitionElement transitionUp;
	TransitionElement transitionDown;
};

struct TxLevel_type {
	double txOutputPower;	// in dBm
	double txPowerConsumed;	// in mW
};

struct CustomModulationElement {
    float SNR;
    float BER;
};

enum CCA_result {
    CLEAR = 1,
    BUSY = 0,
    CS_NOT_VALID = 101,
    CS_NOT_VALID_YET = 102,
};

class RadioModule : public VirtualCastaliaModule
{
	private:
	// parameters and variables

		/*--- class member variables that are derived from module parameters
		      (either in RadioParametersFile or .ini file ---*/

		list <TxLevel_type> TxLevelList;
		list <RXmode_type> RXmodeList;
		list <SleepLevel_type> sleepLevelList;
		TransitionElement transition[3][3];
		int symbolsForRSSI;

		double carrierFreq;
		list <TxLevel_type>::iterator TxLevel;
		list <RXmode_type>::iterator RXmode;
		list <SleepLevel_type>::iterator sleepLevel;
		BasicState_type state;

		// if a custom modulation is defined in one of the RX modes this variable
		//  will hold the info. Only ONE custom modulation is currently supported
		vector <CustomModulationElement> customModulation;

		double CCAthreshold;
		bool carrierSenseInterruptEnabled;
		CollisionModel_type collisionModel;
		Encoding_type encoding;
		int maxPhyFrameSize;
		int PhyFrameOverhead;
		int bufferSize;		//in kbytes

		/*--- class member variables used internally ---*/
		int self;						// the node's ID. Can be considered as a full MAC address

		queue <MacGenericPacket *> radioBuffer;

		// a list of signals curently being received
		list <ReceivedSignal_type> receivedSignals;
		// last time the above list changed
		simtime_t timeOfLastSignalChange;

		// a history of recent changes in total received power to help calculate RSSI
		list <TotalPowerReceived_type> totalPowerReceived;

		// a pointer to the object of the Radio Module (used for direct method calls)
		ResourceGenericManager *resMgrModule;

		int changingToState;	// indicates that the Radio is in the middle of changing from one state (A)
								// to another (B). It also holds the value for state B

		// span of time the total received power is integrated to calculate RSSI
		double rssiIntegrationTime;

		// pointer to message that carries a future carrier sense interrupt
		RadioControlMessage *CSinterruptMsg;
		simtime_t latestCSinterruptTime;

		int disabled;

	protected:
		virtual void initialize();
		virtual void handleMessage(cMessage *msg);
		virtual void finishSpecific();
		void readIniFileParameters(void);
		void parseRadioParameterFile(const char *);

		double popAndSendToWirelessChannel();

		void updateTotalPowerReceived();
		void updateTotalPowerReceived(double newSignalPower);
		void updateTotalPowerReceived(list <ReceivedSignal_type>::iterator endingSignal);

		void updateInterference(list <ReceivedSignal_type>::iterator it1, WirelessChannelSignalBegin *wcMsg);
		void updateInterference(list <ReceivedSignal_type>::iterator it1, list <ReceivedSignal_type>::iterator endingSignal);

		void updatePossibleCSinterrupt();
		double SNR2BER(double SNR);
		int bitErrors(double, int, int);

		int maxErrorsAllowed(Encoding_type) { return 0; }

		int parseInt(const char*, int*);
		int parseFloat(const char*, double*);
		Modulation_type parseModulationType(const char*);

	public:
		double readRSSI();
		CCA_result isChannelClear();
};

#endif //_RADIOMODULE_H_
