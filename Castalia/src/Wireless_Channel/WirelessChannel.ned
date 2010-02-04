//***************************************************************************************
//*  Copyright: National ICT Australia,  2007, 2008, 2009				*
//*  Developed at the Networks and Pervasive Computing program				*
//*  Author(s): Athanassios Boulis, Dimosthenis Pediaditakis, Yuri Tselishchev		*
//*  This file is distributed under the terms in the attached LICENSE file.		*
//*  If you do not find this file, copies can be found by writing to:			*
//*											*
//*      NICTA, Locked Bag 9013, Alexandria, NSW 1435, Australia			*
//*      Attention:  License Inquiry.							*
//*											*
//***************************************************************************************

package Wireless_Channel;


// The wireless channel module simulates the wireless medium. Nodes sent packets to it
// and according to various conditions (fading, interference etc) it is decided which
// nodes can receive this packet

simple WirelessChannel

{
    parameters:
        bool collectTraceInfo = default(false);
        bool onlyStaticNodes = default(true);

        double pathLossExponent = default(2.4);			// how fast is the signal strength fading
        double PLd0 = default(55);				// path loss at reference distance d0 (in dBm)
        double d0 = default(1.0);				// reference distance d0 (in meters)
        int collisionModel = default(2);			// 0 --> No Collisions (no Interference)
                            					// 1 --> Simple Collision Model (more than one transmissions heard by a receiver, leads to a collision)
                            		    			// 2 --> Additive interference model (transmissions seen as interference) 
                            					// 3 --> Complex interference model (as above (2) but multiple transmissions do not add-up linearly)

        double sigma = default(4.0);				// how variable is the average fade for nodes at the same distance
                    						// from eachother. std of a gaussian random variable.

        double bidirectionalSigma = default(1.0);     		// how variable is the average fade for link B->A if we know
                                    				// the fade of link A->B. std of a gaussian random variable

        // the above parameters are used if no PRR_ConnectivityMap or rxSignal_ConnectivityMap are set
        string PRRMapFile = default("");                      	// describes a map of the connectivity based on PRR
        string pathLossMapFile = default("");                 	// describes a map of the connectivity based on pathloss
        string temporalModelParametersFile = default("");     	// the filename that contains all parameters for 
                                            		        // the temporal channel variation model
    gates:
        output toNode[];
        input fromMobilityModule @directIn;
        input fromNode[];
}
