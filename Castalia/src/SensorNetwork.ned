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



import 
	"WirelessChannel",
	"PhysicalProcess",
	"Node";


module SensorNetwork

    parameters:
    	field_x: 				const,	    // the length of the deployment field
    	field_y: 				const,	    // the width of the deployment field
        numNodes:				const,	    // the number of nodes
        deploymentType: 		numeric,    // a number identifying the way the nodes are deployed
		numPhysicalProcesses:	const;
	


    submodules:
        wirelessChannel: WirelessChannel
		gatesizes:
			toNode[numNodes],
			fromNode[numNodes];
		display: "p=195,-150;o=#3399CC";

	physicalProcess: PhysicalProcess[numPhysicalProcesses]	// in this network we have only one kind of sensing device 
		gatesizes:
			toNode[numNodes],
			fromNode[numNodes];
		display: "p=195,460;b=60,25,oval;o=#FF7F00";

	node: Node[numNodes]
		//parameters:			// the default option, random uniform deployment
			//xCoor = uniform(0, field_x),
			//yCoor = uniform(0, field_y);
			
		//parameters if deploymentType == 1:	// grid deployment
			//xCoor = (index % sqrt(numNodes*field_x/field_y))    * field_x /( sqrt(numNodes*field_x/field_y)-1 ),
			//yCoor = floor(index/sqrt(numNodes*field_x/field_y)) * field_y /( sqrt(numNodes*field_y/field_x)-1 );
	
		parameters if deploymentType == 2:	// randomized grid deployment (i.e. grid + noise)
			xCoor = (index % sqrt(numNodes*field_x/field_y))    * field_x /( sqrt(numNodes*field_x/field_y)-1 ) + normal(0, field_x/((sqrt(numNodes*field_x/field_y)-1)*4)),
			yCoor = floor(index/sqrt(numNodes*field_x/field_y)) * field_y /( sqrt(numNodes*field_y/field_x)-1 ) + normal(0, field_y/((sqrt(numNodes*field_y/field_x)-1)*4));
		
		parameters:			// the default option, random uniform deployment
			xCoorVis = ((index % sqrt(numNodes*field_x/field_y))    * field_x /( sqrt(numNodes*field_x/field_y)-1 ))*5,
			yCoorVis = (floor(index/sqrt(numNodes*field_x/field_y)) * field_y /( sqrt(numNodes*field_y/field_x)-1 ))*5;
		
		gatesizes:
			toPhysicalProcess[numPhysicalProcesses],
			fromPhysicalProcess[numPhysicalProcesses];

		display: "p=$xCoorVis,$yCoorVis;o=#668877";




    connections nocheck:
        for i=0..numNodes-1 do
            node[i].toWirelessChannel --> wirelessChannel.fromNode[i];
            node[i].fromWirelessChannel <-- wirelessChannel.toNode[i];
        endfor;

        for i=0..numNodes-1, j=0..numPhysicalProcesses-1 do
        	node[i].toPhysicalProcess[j] --> physicalProcess[j].fromNode[i];
            node[i].fromPhysicalProcess[j] <-- physicalProcess[j].toNode[i];
        endfor;

endmodule


//Instantiates a sensor network
network SN : SensorNetwork

endnetwork