// **************************************************************************************
// *  Copyright: National ICT Australia,  2006											*
// *  Developed at the Networks and Pervasive Computing program							*
// *  Author(s): Athanassios Boulis, Dimosthenis Pediaditakis							*
// *  This file is distributed under the terms in the attached LICENSE file.			*
// *  If you do not find this file, copies can be found by writing to:					*
// *																					*
// *      NICTA, Locked Bag 9013, Alexandria, NSW 1435, Australia						*
// *      Attention:  License Inquiry.													*
// *																					*
// **************************************************************************************/


simple MacModuleSimple

    parameters:
		dutyCycle		: numeric,	// sleeping / (sleeping+listening)
		listenInterval		: numeric,	// how long do we leave the radio in listen mode, in ms
		beaconIntervalFraction	: numeric, 	// fraction of the sleeping interval that we send beacons
		probTx			: numeric,	// the probability of a single try of Transmission to happen
		numTx			: numeric,	// when we have something to Tx, how many times we try
		randomTxOffset		: numeric,	// Tx after time chosen randomly from interval [0..randomTxOffset]
		reTxInterval		: numeric,	// Interval between retransmissions in ms, (numTx-1) retransmissions
		
		maxMACFrameSize		: const,	// in bytes
		macFrameOverhead	: const,	// in 802.15.4 = 5 + (4-->20) bytes
		
		beaconFrameSize		: const,	
		ACKFrameSize		: const,	// in 802.15.4 is always fix-sized = 5 bytes
		
		macBufferSize		: const,	//in number of messages
		carrierSense		: bool;		//if we want the MAC to perform carrier sense before sending to Radio

    gates:
        out:	toCommunicationModule, toRadioModule;
        in:	fromCommunicationModule, fromRadioModule, fromCommModuleResourceMgr;

endsimple
