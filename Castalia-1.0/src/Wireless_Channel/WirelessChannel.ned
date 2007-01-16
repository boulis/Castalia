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



// The wireless channel module simulates the wireless medium. Nodes sent packets to it
// and according to various conditions (fading, interference etc) it is decided which
// nodes can receive this packet

simple WirelessChannel
    
    parameters:
        pathLossExponent: 	const,  // how fast is the signal strength fading
        sigma: 				const,	// how variable is the fading, std of a gaussian rv.
        noiseFloor: 		const,	// the power of the noise (in dBm)
        PLd0: 				const,	// path loss at reference distance d0 (in dBm)
        d0: 				const,	// reference distance d0 (in meters)
        thresholdProb: 		const,  // below this prob we assume packet is not received
		txPowerLevels:		string;	// transmission power levels in dBm, given as a string (MAX # of pwr levels = 15)

    gates:
        out: toNode[];
        in: fromNode[];

endsimple
