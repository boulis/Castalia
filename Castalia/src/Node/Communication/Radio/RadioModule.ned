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

package Node.Communication.Radio;

//==============================================
// Default parameters correspond to Telos mote
//==============================================

simple RadioModule

{
    parameters:
        bool printDebugInfo = default(false);
        bool printDroppedPacketsStatistics = default(false);
        bool printStateTransitions = default(false);

        double dataRate = default(250.0);		// the transmission rate in Kbps
    							// 250kbps for Telos motes and 19.2kbps for MICA2 motes
        int bitsPerSymbol = default(8);			// used to compute symbol rate that some MAC protocols need
        double noiseBandwidth = default(194);		// 194KHz for Telos motes and 30KHz for MICA2 motes
        double noiseFloor = default(-100);		// -100dBm for Telos motes and -105dBm for MICA2 motes
        double receiverSensitivity = default(-95);	// -95dBm for Telos motes and -98dBm for MICA2 motes
        string modulationType = default("PSK"); 	// "IDEAL" --> MODULATION_IDEAL (no transmission errors)
                                            		// "FSK" --> FSK (noncoherent) -- for MICA2 motes
                                            		// "PSK" --> BPSK, QPSK, OQPSK -- for CC2420 (Telos motes)
                                            		// "CUSTOM [SNR:BER], ..., [SNR:BER]" --> Custom modulation
                                    			//	explicitly given SNR to BER mapping.
                                                                                                                                                                                                                                                                                    
        int encodingType = default(0);			// 0 --> NRZ (only NRZ is currently implemented)
                                            		// 1 --> 4B5B (future work, not yet implemented)
                                            		// 2 --> Manchester (future work, not yet implemented)
                                            		// 3 --> SECDEC (future work, not yet implemented)
                                                

        
        // transmission power levels in dBm, given as a string (MAX # of pwr levels = 15)
        // first element is default
        string txPowerLevels = default("0 -1 -3 -5 -7 -10 -15 -25");
	
	// the consumptions of TX at the different power levels in mW
        string txPowerConsumptionPerLevel = default("57.42 55.18 50.69 46.2 42.24 36.3 32.67 29.04");		
        int txPowerLevelUsed = default(0);		// index for the txPowerLevels array.
        
        double rxPower = default(62);			// the rx power in mW, 62mW for Telos
        double listenPower = default(62);		// the listening power in mW (in many platforms it is almost the same with rxPower)
        double sleepPower = default(1.4);		// the sleeping power in mW, 1.4mW for Telos
        


        int txModeUsed = default(0);			// 0 for CARRIER_SENSE_NONE
    							// 1 for CARRIER_SENSE_ONCE_CHECK
    							// 2 for CARRIER_SENSE_PERSISTENT	

        int bufferSize = default(16);		// in number of frames
        int maxPhyFrameSize = default(127);	// in bytes
        int phyFrameOverhead = default(6);	// in bytes (802.15.4. = 6 bytes)

        double delaySleep2Listen = default(0.194);
        double delayListen2Sleep = default(0.05);
        double delayTx2Sleep = default(0.05);
        double delaySleep2Tx = default(0.194);
        double delayListen2Tx = default(0.01);
        double delayTx2Listen = default(0.01);
        double delayCSValid = default(0.128);	//time that needs the Carrier Sense pin to get valid after switching to LISTENING mode

    gates:
        output toCommunicationModule;
        output toMacModule;
        input fromCommunicationModule;
        input fromMacModule;
        input fromCommModuleResourceMgr;

}
