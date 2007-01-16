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



// The physical process module simulates a physical process that could be measured/sampled
// by a sensing device on the nodes. Different sensing devices (e.g temperature, pressure,
// light, acceleration) would be represented by distinct PhysicalProcess modules. A node
// simply ask the process for a sample which is returned as soon as it is calculated.

simple PhysicalProcess

    parameters:
        multiplicative_k: 	numeric,
        attenuation_exp_a: 	numeric,
        sigma: 			numeric,
        max_num_snapshots:	numeric,
        numSources: 	  	numeric, // how many points in space will return this value
        source_0:		string,
        source_1:		string,
        source_2:		string,
        source_3:		string,
        source_4:		string,
        description:		string;

    gates:
        out: toNode[];
        in: fromNode[];

endsimple
