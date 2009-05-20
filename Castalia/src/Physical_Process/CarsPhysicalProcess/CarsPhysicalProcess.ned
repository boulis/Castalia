//***************************************************************************************
//*  Copyright: National ICT Australia,  2009						*
//*  Developed at the Networks and Pervasive Computing program				*
//*  Author(s): Athanassios Boulis, Yuri Tselishchev					*
//*  This file is distributed under the terms in the attached LICENSE file.		*
//*  If you do not find this file, copies can be found by writing to:			*
//*											*
//*      NICTA, Locked Bag 9013, Alexandria, NSW 1435, Australia			*
//*      Attention:  License Inquiry.							*
//*											*
//***************************************************************************************




// The physical process module simulates a physical process that could be measured/sampled
// by a sensing device on the nodes. Different sensing devices (e.g temperature, pressure,
// light, acceleration) would be represented by distinct PhysicalProcess modules. A node
// simply ask the process for a sample which is returned as soon as it is calculated.

simple CarsPhysicalProcess

    parameters:
    	printDebugInfo: bool,

        max_num_cars: 	  	numeric, // how many cars can be present at once
        car_speed:		numeric,
        car_value: 		numeric,
        car_interarrival:	numeric,
        point1_x_coord:		numeric,
        point2_x_coord:		numeric,
        point1_y_coord:		numeric,
        point2_y_coord:		numeric,
        
        description:		string;
	        

    gates:
        out: toNode[];
        in: fromNode[];

endsimple
