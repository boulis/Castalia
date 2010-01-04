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


import Wireless_Channel;
import Physical_Process;
import Node;


network SN

{
    parameters:
        int field_x = default(30);		// the length of the deployment field
        int field_y = default(30);		// the width of the deployment field
        int field_z = default(1);		// the height of the deployment field

        int xCellSize = default(5);		// if we define cells (to handle mobility)
        int yCellSize = default(5);		// how big are the cells in each dimension
        int zCellSize = default(1);

	int numNodes;				// the number of nodes

	int deploymentType = default(1);	// 0 -> uniform random deployment inside the network's X,Y boundaries
	                                        // 1 -> grid deployment
	                                        // 2 -> means randomized grid deployment (i.e. grid + noise)
	                                        // any other number means that the user must specify manually the locations of

	double randomFactor = default(0.3);     // randomFactor as a % of network boundaries for randomized grid deployment

	int xGridSize = default(5);		// Number of nodes along each axis for grid deployment
	int yGridSize = default(5);		
	int zGridSize = default(1);		

        int numPhysicalProcesses = default(1);
        string physicalProcessModuleName = default("CustomizablePhysicalProcess");

        string debugInfoFilename = default("Castalia-Trace.txt");

    submodules:

        wirelessChannel: Wireless_Channel.WirelessChannel {
	    gates:
		toNode[numNodes];
		fromNode[numNodes];
        }

	physicalProcess[numPhysicalProcesses]: <physicalProcessModuleName> like Physical_Process.PhysicalProcessGenericModule {
	    gates:
		toNode[numNodes];
		fromNode[numNodes];
        }

        node[numNodes]: Node.Node {
            gates:
		toPhysicalProcess[numPhysicalProcesses];
		fromPhysicalProcess[numPhysicalProcesses];
		
        }

    connections:
        for i=0..numNodes-1 {
            node[i].toWirelessChannel --> wirelessChannel.fromNode[i];
            node[i].fromWirelessChannel <-- wirelessChannel.toNode[i];
        }

        for i=0..numNodes-1, for j=0..numPhysicalProcesses-1 {
            node[i].toPhysicalProcess[j] --> physicalProcess[j].fromNode[i];
            node[i].fromPhysicalProcess[j] <-- physicalProcess[j].toNode[i];
        }
}
