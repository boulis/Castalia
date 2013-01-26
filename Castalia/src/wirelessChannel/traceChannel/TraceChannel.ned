//********************************************************************************
//*  Copyright: National ICT Australia,  2007 - 2012                             *
//*  Developed at the ATP lab, Networked Systems research theme                  *
//*  Author(s): Yuriy Tselishchev                                                *
//*  This file is distributed under the terms in the attached LICENSE file.      *
//*  If you do not find this file, copies can be found by writing to:            *
//*                                                                              *
//*      NICTA, Locked Bag 9013, Alexandria, NSW 1435, Australia                 *
//*      Attention:  License Inquiry.                                            *
//*                                                                              *
//*******************************************************************************/*

package wirelessChannel.traceChannel;

// The wireless channel module simulates the wireless medium. Nodes sent packets to it
// and according to various conditions (fading, interference etc) it is decided which
// nodes can receive this packet

simple TraceChannel like wirelessChannel.iWirelessChannel {
 parameters:
	bool collectTraceInfo = default (false);
	int coordinator = default(0); 				// ID of coordinator node
	int traceStep = default(5);					// time interval between trace entrys, in ms

	string traceFile = default("");				// Node with coordinator ID is in the center, and 
												// traceFile defines the maximum number of nodes that is 
												// allowed. traceFile MUST be given in the czonfiguration, 
												// or simulation fails

	double signalDeliveryThreshold = default (-100);	
												// threshold in dBm above which, wireless channel module
												// is delivering signal messages to radio modules of 
												// individual nodes
	
	string pathlossMapFile = default("");		// optional static map to define pathloss between all
												// possible pairs of leaf nodes, if not specified, then
												// leafLinkProbability and leafPathloss are used to 
												// populate pathloss map

	double pathlossMapOffset = default(0);		// optional offset value for pathlossMap. This offset will
												// be added to all pathloss entries

	string temporalModelParametersFile = default("");
												// opional temporal model variation, only works if 
												// pathlossMap file is defined

	double leafLinkProbability = default(0);	// Probability to have reception between a pari of leaf nodes
	double leafPathloss = default(80.0);		// Pathloss between leaf nodes
 gates:
 	output toNode[];
	input fromMobilityModule @ directIn;
	input fromNode[];
}
