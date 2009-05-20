//***************************************************************************************
//*  Copyright: National ICT Australia,  2007, 2008, 2009				*
//*  Developed at the Networks and Pervasive Computing program				*
//*  Author(s): Athanassios Boulis, Dimosthenis Pediaditakis				*
//*  This file is distributed under the terms in the attached LICENSE file.		*
//*  If you do not find this file, copies can be found by writing to:			*
//*											*
//*      NICTA, Locked Bag 9013, Alexandria, NSW 1435, Australia			*
//*      Attention:  License Inquiry.							*
//*											*
//***************************************************************************************




simple RadioModule

    parameters:
    	printDebugInfo: bool,
    	printDroppedPacketsStatistics:	bool,
    	printStateTransitions:	bool,
		dataRate:		numeric,	// the transmission rate in Kbps
		noiseBandwidth:		const,
        noiseFloor: 		const,	// the power of the noise (in dBm)
		modulationType:		string,  	// "IDEAL" --> MODULATION_IDEAL (no transmission errors)
							// "FSK" --> FSK (noncoherent) -- for MICA2 motes
							// "PSK" --> BPSK, QPSK, OQPSK -- for CC2420 (Telos motes)
							// "CUSTOM [SNR:BER], ..., [SNR:BER]" --> Custom modulation 
							//	explicitly given SNR to BER mapping.

		encodingType:		const,
		receiverSensitivity:const,
		txPowerLevels:		string,	// transmission power levels in dBm, given as a string (MAX # of pwr levels = 15)
		rxPower:			const,		// the rx power in mW
		listenPower:		const,		// the listening power in mW (in many platforms it is almost the same with rxPower)
		sleepPower:			const,		// the sleeping power in mW
		txPowerConsumptionPerLevel:	string,		// the consumptions of TX at the different power levels in mW
		txPowerLevelUsed:	numeric,	// index for the txPowerLevels array.
		txModeUsed:			numeric,	// a)0 for CARRIER_SENSE_NONE , b) 1 for CARRIER_SENSE_ONCE_CHECK, c)2 for CARRIER_SENSE_PERSISTENT	
		bufferSize:			const,		// in bytes
			
		maxPhyFrameSize:	const,		// in bytes
		phyFrameOverhead:	const,		// in bytes (802.15.4. = 6 bytes)
			
		delaySleep2Listen:	const,
		delayListen2Sleep:	const,
		delayTx2Sleep:		const,
		delaySleep2Tx:		const,
		delayListen2Tx:		const,
		delayTx2Listen:		const,
			
		delayCSValid:		const;		//time that needs the Carrier Sense pin to get valid after switching to LISTENING mode

    gates:
        out:	toCommunicationModule, toMacModule;
        in:	fromCommunicationModule, fromMacModule, fromCommModuleResourceMgr;

endsimple
