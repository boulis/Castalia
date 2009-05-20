//*************************************************************************
//*  Copyright: National ICT Australia,  2007, 2008, 2009
//*  Developed at the ATP lab, Networked Systems theme
//*  Author(s): Athanassios Boulis, Dimosthenis Pediaditakis
//*  This file is distributed under the terms in the attached LICENSE file.
//*  If you do not find this file, copies can be found by writing to:
//*
//*      NICTA, Locked Bag 9013, Alexandria, NSW 1435, Australia
//*      Attention:  License Inquiry.
//*************************************************************************/


import 
	"WirelessChannel",
	"PhysicalProcess",
	"Node";


module SensorNetwork

    parameters:
    	field_x: 			const,	    // the length of the deployment field
    	field_y: 			const,	    // the width of the deployment field
    	field_z: 			const,	    // the height of the deployment field
    	
    	xCellSize:			const,      // if we define cells (to handle mobility)
    	yCellSize:			const,      // how big are the cells in each dimension
    	zCellSize:			const,
    	
    	numNodes:			const,	    // the number of nodes

    	deploymentType: 		const,    // a number identifying the way the nodes are deployed
    	xGridSize:			const,
    	yGridSize:			const,
    	zGridSize:			const,
    	
	physicalProcessModuleName:	string,
	numPhysicalProcesses:		const,
	debugInfoFilename:		string;
	
    submodules:
		wirelessChannel: WirelessChannel
			gatesizes:
				toNode[numNodes],
				fromNode[numNodes];
		
		physicalProcess: physicalProcessModuleName[numPhysicalProcesses] like PhysicalProcessGenericModule	// in this network we have only one kind of sensing device 
			gatesizes:
				toNode[numNodes],
				fromNode[numNodes];
	
		node: Node[numNodes]
			parameters if deploymentType == 0:      // the default option, random uniform deployment
				xCoor = uniform(0, field_x),
				yCoor = uniform(0, field_y),
				zCoor = uniform(0, field_z);
				
			parameters if deploymentType == 1:	// grid deployment
				xCoor = (index % xGridSize) * (field_x/(xGridSize-1)),
				yCoor = (floor(index/xGridSize) % yGridSize) * (field_y/(yGridSize-1)),
				zCoor = (floor(index/(xGridSize*yGridSize)) % zGridSize) * (field_z/(zGridSize-1));
                                
			parameters if deploymentType == 2:	// randomized grid deployment (i.e. grid + noise)
				xCoor = (index % xGridSize) * (field_x/(xGridSize-1)) + normal(0, (field_x/(xGridSize-1))/3),
				yCoor = (floor(index/xGridSize) % yGridSize) * (field_y/(yGridSize-1)) + normal(0, (field_y/(yGridSize-1))/3),
				zCoor = (floor(index/(xGridSize*yGridSize)) % zGridSize) * (field_z/(zGridSize-1)) + normal(0, (field_z/(zGridSize-1))/3);
			
			gatesizes:
				toPhysicalProcess[numPhysicalProcesses],
				fromPhysicalProcess[numPhysicalProcesses];
	
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
