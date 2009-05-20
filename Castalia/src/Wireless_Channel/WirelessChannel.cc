/***************************************************************************
 *  Copyright: National ICT Australia,  2007, 2008, 2009
 *  Developed at the ATP lab, Networked Systems theme
 *  Author(s): Athanassios Boulis, Yuri Tselishchev
 *  This file is distributed under the terms in the attached LICENSE file.
 *  If you do not find this file, copies can be found by writing to:
 *
 *      NICTA, Locked Bag 9013, Alexandria, NSW 1435, Australia
 *      Attention:  License Inquiry.
 ***************************************************************************/


#include "WirelessChannel.h"

Define_Module(WirelessChannel);


void WirelessChannel::initialize() {

	/* variable to report initialization run time */
	clock_t startTime;
	startTime = clock();

	readIniFileParameters();

	/****************************************************
	 * To handle mobile nodes we break up the space into
	 * cells. All path loss calculations are now done
	 * between cells. First we need to find out how many
	 * distinct cells we have in space, based on the
	 * dimensions of the space and the declared cell size.
	 * If we only have static nodes, then we keep the
	 * same variables and abstractions to make the code
	 * more compact and easier to read, but we do not
	 * really need to divide the space into cells. Even
	 * though we keep the variable names, cells in the
	 * static case only correspond to node positions
	 * and we only have numOfNodes cells, much like
	 * we used to do in Castalia 1.3
	 ****************************************************/


	if(onlyStaticNodes)
	{
		numOfSpaceCells =  numOfNodes;
  	}
	else
	{
    	if (xFieldSize <= 0) {xFieldSize = 1; xCellSize=1;}
    	if (yFieldSize <= 0) {yFieldSize = 1; yCellSize=1;}
    	if (zFieldSize <= 0) {zFieldSize = 1; zCellSize=1;}
    	if (xCellSize <= 0) xCellSize = xFieldSize;
    	if (yCellSize <= 0) yCellSize = yFieldSize;
    	if (zCellSize <= 0) zCellSize = zFieldSize;

    	numOfZCells = (int)ceil(zFieldSize/zCellSize);
    	numOfYCells = (int)ceil(yFieldSize/yCellSize);
    	numOfXCells = (int)ceil(xFieldSize/xCellSize);
    	numOfSpaceCells = numOfZCells * numOfYCells * numOfXCells;
	   /***************************************************************
	    * Calculate some values that  help us transform a 1D index in
	    * [0..numOfSpaceCells -1] to a 3D index x, y, z and vice versa.
	    * Each variable holds index increments (in the 1D large index)
	    * needed to move one space cell in the z, y, and x directions
	    **************************************************************/
	  	zIndexIncrement = numOfYCells * numOfXCells;
	   	yIndexIncrement = numOfXCells;
	   	xIndexIncrement = 1;
	}


	/***************************************************************
	 * Allocate and initialize cellOccupation and nodeLocation.
	 * nodeLocation keeps the state about all nodes locations and
	 * cellOccupation is an array of lists. List at index i contains
	 * the node IDs that reside in cell i. We define and use these
	 * arrays even for the static nodes case as it makes the code
	 * more compact and easier to follow.
	 **************************************************************/
	nodeLocation = new NodeLocation_type [numOfNodes];
	if (nodeLocation==NULL) opp_error("Could not allocate array nodeLocation\n");

	cellOccupation = new list<int> [numOfSpaceCells];
	if (cellOccupation==NULL) opp_error("Could not allocate array cellOccupation\n");

	cTopology *topo;  // temp variable to access initial location of the nodes
	topo = new cTopology("topo");
	topo->extractByModuleType("Node", NULL);

	for (int i = 0; i < numOfNodes; i++)
	{
		nodeLocation[i].x = (double)topo->node(i)->module()->par("xCoor");
		nodeLocation[i].y = (double)topo->node(i)->module()->par("yCoor");
		nodeLocation[i].z = (double)topo->node(i)->module()->par("zCoor");
		nodeLocation[i].phi = (double)topo->node(i)->module()->par("phi");
		nodeLocation[i].theta = (double)topo->node(i)->module()->par("theta");
		nodeLocation[i].cell = i; // will be overwriten if !onlyStaticNodes

		if (!onlyStaticNodes)
		{
			/******************************************************************
			 * Compute the cell this node is in and initialize cellOccupation.
			 * Cavaet in computing the XYZ indices:
			 * Because we allow cell resolutions that do not perfectly divide
			 * the field (we take the ceiling of the division when calculating
			 * numOfXCells) this means that the edge cells might be smaller than
			 * the others. So in some cases, the calculation we are doing
			 * below, might give the wrong cell by +1. That's why we are doing
			 * the test immediately after.
			 ******************************************************************/
			int xIndex = (int)floor(nodeLocation[i].x/xFieldSize * numOfXCells);
			if (((xIndex-1)*xCellSize) >= nodeLocation[i].x) xIndex--;
			else if (xIndex >= numOfXCells )
			{
				xIndex = numOfXCells - 1;   // the maximum possible x index
				if (nodeLocation[i].x > xFieldSize)
					CASTALIA_DEBUG << "WARNING at initialization: node position out of bounds in X dimension!\n";
			}

			int yIndex = (int)floor(nodeLocation[i].y/yFieldSize * numOfYCells);
			if (((yIndex-1)*yCellSize) >= nodeLocation[i].y) yIndex--;
			else if (yIndex >= numOfYCells )
			{
				yIndex = numOfYCells - 1;   // the maximum possible y index
				if (nodeLocation[i].y > yFieldSize)
					CASTALIA_DEBUG << "WARNING at initialization: node position out of bounds in Y dimension!\n";
			}

			int zIndex = (int)floor(nodeLocation[i].z/zFieldSize * numOfZCells);
			if (((zIndex-1)*zCellSize) >= nodeLocation[i].z) zIndex--;
			else if (zIndex >= numOfZCells )
			{
				zIndex = numOfZCells - 1;   // the maximum possible z index
				if (nodeLocation[i].z > zFieldSize)
					CASTALIA_DEBUG << "WARNING at initialization: node position out of bounds in Z dimension!\n";
			}

			int cell = zIndex * zIndexIncrement + yIndex * yIndexIncrement + xIndex * xIndexIncrement;

			nodeLocation[i].cell = cell;
		}
		/*************************************************
		 * pushing ID i into the list cellOccupation[cell]
		 * (if onlyStaticNodes cell=i )
		 *************************************************/
		cellOccupation[nodeLocation[i].cell].push_front(i);
	}
	delete(topo);



	/**********************************************
	 * Allocate and initialize the pathLoss array.
	 * This is the "propagation map" of our space.
	 **********************************************/
	pathLoss = new list<PathLossElement *> [numOfSpaceCells];
	if (pathLoss==NULL) opp_error("Could not allocate array pathLoss\n");

	int elementSize = sizeof(PathLossElement) + 3*sizeof(PathLossElement*);
	int totalElements = 0;  //keep track of pathLoss size for reporting purposes

	float x1, x2, y1, y2, z1, z2, dist;
	float PLd; // path loss at distance dist, in dB

	/*******************************************************
	 * Calculate the distance, beyond which we cannot
	 * have connectivity between two nodes. This calculation
	 * is based on the maximum TXPower the receiverSensitivity
	 * the pathLossExponent, the PLd0. For the random
	 * shadowing part we use 3*sigma to account for 99.7%
	 * of the cases. We use this value to considerably
	 * speed up the filling of the pathLoss array,
	 * especially for the mobile case.
	 *******************************************************/
	float distanceThreshold = d0 * pow(10.0, (maxTxPower - receiverSensitivity - PLd0 + 3*sigma)/(10.0 * pathLossExponent) );

	for (int i = 0; i < numOfSpaceCells; i++)
	{
		if (onlyStaticNodes)
		{
			x1 = nodeLocation[i].x;
			y1 = nodeLocation[i].y;
			z1 = nodeLocation[i].z;
		}
		else
		{
			z1 = zCellSize *   (int)floor(i/zIndexIncrement);
			y1 = yCellSize * (((int)floor(i/yIndexIncrement))%zIndexIncrement);
			x1 = xCellSize * (((int)floor(i/xIndexIncrement))%yIndexIncrement);
		}

		/* Path loss to yourself is 0.0 */
		pathLoss[i].push_front(new PathLossElement(i, 0.0));
		totalElements++; //keep track of pathLoss size for reporting purposes

	    for (int j = i+1; j < numOfSpaceCells; j++)
	    {
			if (onlyStaticNodes)
			{
				x2 = nodeLocation[j].x;
				y2 = nodeLocation[j].y;
				z2 = nodeLocation[j].z;
			}
			else
			{
				z2 = zCellSize *   (int)(j/zIndexIncrement);
				y2 = yCellSize * (((int)(j/yIndexIncrement))%zIndexIncrement);
				x2 = xCellSize * (((int)(j/xIndexIncrement))%yIndexIncrement);

				if (fabs(x1-x2) > distanceThreshold) continue;
				if (fabs(y1-y2) > distanceThreshold) continue;
				if (fabs(z1-z2) > distanceThreshold) continue;
			}

			dist = sqrt((x2 - x1)*(x2 - x1) + (y2 - y1)*(y2 - y1) + (z2 - z1)*(z2 - z1));
			if (dist > distanceThreshold) continue;

			PLd = PLd0 + 10.0 * pathLossExponent * log10(dist/d0) + normal(0, sigma);

			float bidirectionalPathLossJitter = normal(0, bidirectionalSigma) / 2;

			if (maxTxPower - PLd - bidirectionalPathLossJitter >= receiverSensitivity)
			{
				pathLoss[i].push_front(new PathLossElement(j, PLd + bidirectionalPathLossJitter));
				totalElements++;  //keep track of pathLoss size for reporting purposes
			}

			if (maxTxPower - PLd + bidirectionalPathLossJitter >= receiverSensitivity)
			{
				pathLoss[j].push_front(new PathLossElement(i, PLd - bidirectionalPathLossJitter));
				totalElements++;  //keep track of pathLoss size for reporting purposes
			}

	    }
	}

	CASTALIA_DEBUG << "\nNumber of distinct space cells: " << numOfSpaceCells << "\n";
	CASTALIA_DEBUG << "Each cell affects (and keeps state for): " << (double)totalElements/numOfSpaceCells << " other cells on the average\n";
	CASTALIA_DEBUG << "The pathLoss array of lists was allocated in " << (double)(totalElements * elementSize)/1048576 << " MBytes. \nThe larger this number, the slower your simulation. Consider increasing\nthe cell size, decreasing the field size, or if you only \nhave static nodes, decreasing the number of nodes\n\n";

	/* Allocate the currentSignalAtReceiver array */
	currentSignalAtReceiver = new list<ReceivedSignalElement *> [numOfNodes];
	if (currentSignalAtReceiver==NULL) opp_error("Could not allocate array currentSignalAtReceiver\n");

	/* Allocate and initialize the isNodeCarrierSensing array */
	isNodeCarrierSensing = new bool[numOfNodes];
	if (isNodeCarrierSensing==NULL) opp_error("Could not allocate array isNodeCarrierSensing\n");
	for (int i = 0; i < numOfNodes; i++) isNodeCarrierSensing[i] = false;


	/*********************************************************************
	 * Allocate nodesAffectedByTransmitter even for static nodes.
	 * This makes the code more compact. We also have temporal variations
	 * so the nodes that are affected are not necessarily the same.
	 *********************************************************************/
	nodesAffectedByTransmitter = new list<int> [numOfNodes];
	if (nodesAffectedByTransmitter==NULL) opp_error("Could not allocate array nodesAffectedByTransmitter\n");



	/***************************************************************************
	 * If direct assignment of link qualities is given at the omnetpp.ini file
	 * (either as RxPower or PRR) we parse the input and update pathLoss.
	 * This is only for static nodes. (onlyStaticNodes==TRUE)
	 ***************************************************************************/
        parsePathLossMap();
	if(onlyStaticNodes) parsePrrMap();

	/* Create temporal model object from parameters file (if given) */
	if (strlen(temporalModelParametersFile) > 0) 
	{
	    temporalModel = new channelTemporalModel(temporalModelParametersFile,2);
	    temporalModelDefined = true;
	}
	else 
	{
	    temporalModelDefined = false;
	}

	/**************************************************************
	 * Precompute some good and bad SNR thresholds so we alleviate
	 * the compute-intensive operation of determining PER from SNR.
	 * Useful for well-studied modulation schemes (FSK, PSK)
	 **************************************************************/
	switch (modulationType)
	{
		case MODULATION_FSK:
	      		thresholdSNRdB = 10.0 * log10(   (-2.0*(dataRate/noiseBandwidth)) * log( 2.0 * (1.0 -  pow(BAD_LINK_PROB_THRESHOLD, 1.0/(8.0*minPacketSize))) )   );
	      		goodLinkSNRdB =  10.0 * log10(   (-2.0*(dataRate/noiseBandwidth)) * log( 2.0 * (1.0 -  pow(GOOD_LINK_PROB_THRESHOLD, 1.0/(8.0*maxPacketSize))) )   );
	          	break;

		case MODULATION_PSK:
          		thresholdSNRdB = 10.0 * log10(   (dataRate/noiseBandwidth) * pow(erfcInv(2.0 * ( 1.0 - pow(BAD_LINK_PROB_THRESHOLD, 1.0/(8.0*minPacketSize)) )), 2.0)   );
	        	goodLinkSNRdB = 10.0 * log10(   (dataRate/noiseBandwidth) * pow(erfcInv(2.0 * ( 1.0 - pow(GOOD_LINK_PROB_THRESHOLD, 1.0/(8.0*maxPacketSize)) )), 2.0)   );
          		break;

		case MODULATION_CUSTOM:
			thresholdSNRdB = customModulationArray[0].SNR;   // the first element of the SNR_TO_BER array
			goodLinkSNRdB = customModulationArray[numOfCustomModulationValues-1].SNR;  // the last element of the SNR_TO_BER array
			break;

		case MODULATION_IDEAL:
			thresholdSNRdB = IDEAL_MODULATION_THRESHOLD_SNR_DB;
			goodLinkSNRdB =  IDEAL_MODULATION_THRESHOLD_SNR_DB;
			break;

		default:
			break;
	}


	/* Allocate and Initialize stats */
	stats = new TxRxStatistics_type [numOfNodes];
	if (stats==NULL) opp_error("Could not allocate array stats\n");


	CASTALIA_DEBUG << "Time for Wireless Channel module initialization: " << (double)(clock() - startTime)/CLOCKS_PER_SEC <<"secs\n\n";


	/*******************************************************
	 * Schedule a self message at the sim-time-limit so the
	 * simulation does not finish before the sim-time-limit
	 *******************************************************/
	scheduleAt(ev.config()->getAsTime("General", "sim-time-limit"), new WChannel_GenericMessage("sim-time-limit message", WC_SIMTIME_LIMIT));
}




/*****************************************************************************
 * This is where the main work is done by processing all the messages received
 *****************************************************************************/
void WirelessChannel::handleMessage(cMessage *msg)
{
	WChannel_GenericMessage* message = dynamic_cast<WChannel_GenericMessage*>(msg);

	// we get the ID of the node who sent this message
	int srcAddr = message->getSrcAddress();


	switch (message -> kind()) {

		case WC_SIMTIME_LIMIT:
		{
			CASTALIA_DEBUG << "\n[Wireless Channel]: Received self message to ensure sim-time-limit is reached @time:" << simTime();
			break;
		}

		case WC_NODE_MOVEMENT:
		{
			/*****************************************************
			 * A node notified the wireless channel that it moved
			 * to a new space cell. Update the nodeLocation and
			 * based on the new cell calculation decide if the
			 * cellOccupation array needs to be updated.
			 *****************************************************/

			if (onlyStaticNodes) opp_error("Error: Rerceived WS_NODE_MOVEMENT msg, while onlyStaticNodes is TRUE");

			int oldCell = nodeLocation[srcAddr].cell;

			WChannel_NodeMoveMessage* mobilityMsg = dynamic_cast<WChannel_NodeMoveMessage*>(msg);
			nodeLocation[srcAddr].x = mobilityMsg->getX();
			nodeLocation[srcAddr].y = mobilityMsg->getY();
			nodeLocation[srcAddr].z = mobilityMsg->getZ();
			nodeLocation[srcAddr].phi = mobilityMsg->getPhi();
			nodeLocation[srcAddr].theta = mobilityMsg->getTheta();

			if ((nodeLocation[srcAddr].x < 0.0)||(nodeLocation[srcAddr].y < 0.0)||(nodeLocation[srcAddr].z < 0.0))
				opp_error("Wireless channel received faulty WC_NODE_MOVEMENT msg. We cannot have negative node coordinates");

			int xIndex = (int)floor(nodeLocation[srcAddr].x/xFieldSize * numOfXCells);
			if (((xIndex-1)*xCellSize) >= nodeLocation[srcAddr].x) xIndex--;
			else if (xIndex >= numOfXCells )
			{
				xIndex = numOfXCells - 1;   // the maximum possible x index
				if (nodeLocation[srcAddr].x > xFieldSize)
					CASTALIA_DEBUG << "WARNING at WC_NODE_MOVEMENT: node position out of bounds in X dimension!\n";
			}
			int yIndex = (int)floor(nodeLocation[srcAddr].y/yFieldSize * numOfYCells);
			if (((yIndex-1)*yCellSize) >= nodeLocation[srcAddr].y) yIndex--;
			else if (yIndex >= numOfYCells )
			{
				yIndex = numOfYCells - 1;   // the maximum possible y index
				if (nodeLocation[srcAddr].y > yFieldSize)
					CASTALIA_DEBUG << "WARNING at WC_NODE_MOVEMENT: node position out of bounds in Y dimension!\n";
			}
			int zIndex = (int)floor(nodeLocation[srcAddr].z/zFieldSize * numOfZCells);
			if (((zIndex-1)*zCellSize) >= nodeLocation[srcAddr].z) zIndex--;
			else if (zIndex >= numOfZCells )
			{
				zIndex = numOfZCells - 1;   // the maximum possible z index
				if (nodeLocation[srcAddr].z > zFieldSize)
					CASTALIA_DEBUG << "WARNING at WC_NODE_MOVEMENT: node position out of bounds in Z dimension!\n";
			}


			int newCell = zIndex * zIndexIncrement + yIndex * yIndexIncrement + xIndex * xIndexIncrement;

			if (newCell != oldCell)
			{
				cellOccupation[oldCell].remove(srcAddr);
				cellOccupation[newCell].push_front(srcAddr);
				nodeLocation[srcAddr].cell = newCell;
			}
		}

		case WC_CARRIER_SENSE_BEGIN:
		{
			isNodeCarrierSensing[srcAddr] = true;

			/***********************************************************
			 * Check the current received signal strength for this node
			 * If the list currentSignalAtReceiver[srcAddr] is non empty
			 * then we know that the received signal is beyond the
			 * receiver sensitivity threashold so carrier is sensed.
			 ***********************************************************/

			if (!currentSignalAtReceiver[srcAddr].empty())
				sendDelayed( new cMessage("carrier signal sensed", WC_CARRIER_SENSED), CS_DETECTION_DELAY, "toNode", srcAddr);

			break;
		}

		case WC_CARRIER_SENSE_INSTANTANEOUS:
		{
			/***************************************************************************
 			 * Carrier sensing in Castalia works in an interrupt-based design (declaring
			 * interest, waiting for response). The MAC (and subsequently the radio)
 		 	 * declares an interval where we wish to have carrier sensing.
 			 * The wireless channel then replies with a carrier_sensed msg
 			 * either at the beginning of this interval or at the beginning of new
 			 * packets transmitting on the wireless channel. But what if we want
 			 * to check (carrier sense) a specific moment of a carrier sensed interval?
 			 * For example we are already CSing while listening and a packet comes
 			 * for TX which also needs CSing. That's why we created this extra msg kind
 			 * If you want to CS and there is already an active CS interval then
 			 * you use this instantaneous CS kind. The first need for this appeared with
 			 * the TMAC protocol where we CS for the whole listening period (in case
 			 * we want to extend listening) but we also need to CS before TXing.
 			 *****************************************************************************/

			if (!currentSignalAtReceiver[srcAddr].empty())
				sendDelayed( new cMessage("carrier signal sensed", WC_CARRIER_SENSED), CS_DETECTION_DELAY, "toNode", srcAddr);
			else
				sendDelayed( new cMessage("no carrier signal sensed", WC_NO_CARRIER_SENSED), CS_DETECTION_DELAY,"toNode",srcAddr);

			break;
		}


		case WC_CARRIER_SENSE_END:
		{
			isNodeCarrierSensing[srcAddr] = false;
			break;
		}



		case WC_PKT_BEGIN_TX:
		{
			/* update the stats */
			stats[srcAddr].transmissions++;

			/* Find the cell that the transmitting node resides */
			int cellTx = nodeLocation[srcAddr].cell;

			/* Iterate through the list of cells that are affected by cellTx
			 * and check if there are nodes there. If they are, are they
			 * carrier sensing? If yes send a carrier_sensed message. Update
			 * the currentSignalAtReceiver and nodesAffectedByTransmitter arrays
			 */
			list<PathLossElement *>::iterator it1;
			for ( it1=pathLoss[cellTx].begin(); it1 != pathLoss[cellTx].end(); it1++ )
			{
				/* If no nodes exist in this cell, move on.*/
				if (cellOccupation[(*it1)->cellID].empty()) continue;


				/* Otherwise there are some nodes in that cell.
				 * Calculate the signal received by these nodes
				 * It is exactly the same for all of them.
				 * The signal may be variable in time.
				 */
				float currentSignalReceived = message->getTxPower_dB() - (*it1)->avgPathLoss;
				if (temporalModelDefined)
				{
				    float timePassed_msec = (simTime() - (*it1)->lastObservationTime)*1000;
				    float timeProcessed_msec = temporalModel->runTemporalModel(timePassed_msec,&((*it1)->lastObservedDiffFromAvgPathLoss));
				    currentSignalReceived += (*it1)->lastObservedDiffFromAvgPathLoss;
				    /* Update the observation time */
				    (*it1)->lastObservationTime = simTime() - (timePassed_msec - timeProcessed_msec)/1000;
				}
				/* If the resulting current signal received is
				 * not strong enough, update the relevant stats
				 * and continue to the next cell. 
				 */
				if ((currentSignalReceived - noiseFloor < thresholdSNRdB) || (currentSignalReceived < receiverSensitivity))
				{
					stats[srcAddr].TxFailedTemporalFades += cellOccupation[(*it1)->cellID].size();
					continue;
				}
				

				/* Else go through all the nodes of that cell.
				 * Iterator it2 returns node IDs.
				 */
				list<int>::iterator it2;
				for ( it2=cellOccupation[(*it1)->cellID].begin(); it2 != cellOccupation[(*it1)->cellID].end(); it2++ )
				{
					/* Is node *it2 carrier sensing? if yes, send carrier_sensed message */
					if (isNodeCarrierSensing[*it2])
						sendDelayed( new cMessage("carrier signal sensed", WC_CARRIER_SENSED), CS_DETECTION_DELAY, "toNode", *it2);

					/* For each node in the cell update the currentSignalAtReceiver
					 * Iterator it3 returns receivedSignalElements from different
					 * active transmitters.
					 */
					list<ReceivedSignalElement *>::iterator it3;
					float totalCurrentInterference = -200.0;
					for (it3=currentSignalAtReceiver[*it2].begin(); it3 != currentSignalAtReceiver[*it2].end(); it3++)
					{
						/* calculate the new interference caused to the existing transmissions */
						float newInterference = addPower_dBm((*it3)->currentInterference, currentSignalReceived);
						if (newInterference > (*it3)->maxInterference)  (*it3)->maxInterference = newInterference;

						totalCurrentInterference = addPower_dBm(totalCurrentInterference, (*it3)->signal);
					}//for it3

					currentSignalAtReceiver[*it2].push_front(new ReceivedSignalElement(srcAddr, currentSignalReceived, totalCurrentInterference));
					nodesAffectedByTransmitter[srcAddr].push_front(*it2);

				} //for it2

			} //for it1

			break;
		} //case WC_PKT_BEGIN_TX


		case WC_PKT_END_TX:
		{
			float InterferenceForSignalTerminating, signalTerminating, SINR_dB, prob;
			bool found = false;

			/* Go through the list of nodes that were affected
			 *  by this transmission. *it1 holds the node ID
			 */
			list<int>::iterator it1;
			for (it1=nodesAffectedByTransmitter[srcAddr].begin(); it1 != nodesAffectedByTransmitter[srcAddr].end(); it1++)
			{

				/* For each node, go through its list of received signals
				 * and find the signal and interference for this ending
				 * transmission. Then delete that entry in the list.
				 */
				list<ReceivedSignalElement *>::iterator it2;
				for (it2=currentSignalAtReceiver[*it1].begin(); it2 != currentSignalAtReceiver[*it1].end(); it2++)
				{
					if ((*it2)->transmitterID == srcAddr)
					{
						signalTerminating = (*it2)->signal;
						InterferenceForSignalTerminating = (*it2)->maxInterference;

						delete (*it2);  // release the memory used by the object
						currentSignalAtReceiver[*it1].erase(it2);  // delete it from the list
						found = true;   // raise a flag for sanity checking, it should always be found
						break; // no need to search further in the loop
					}
				}

				/* Sanity check: it should always be found (put there during WC_PKT_BEGIN_TX msg handling)*/
				if (found) found = false;
				else opp_error("Signal by transmitter not found while WC_PKT_END_TX msg handling. Bug in the code!");

				/* Then for each of the remaining transmissions that affect
				 * node *it1, reduce the interference by signalTerminating.
				 */
				for (it2=currentSignalAtReceiver[*it1].begin(); it2 != currentSignalAtReceiver[*it1].end(); it2++)
				{
					(*it2)->currentInterference = subtractPower_dBm((*it2)->currentInterference, signalTerminating);
				}

				/*************************************************************************
				 * If the affected node is the transmitter itself then we do not deliver
				 * the packet. We do need to allow for all the other operations before to
				 * happen, both in the WC_PKT_END_TX and WC_PKT_BEGIN_TX message handling
				 *************************************************************************/
				if ((*it1)==srcAddr) continue;


				/* Determine the SNR based on the collision model */
				switch(collisionModel)
				{
					case NO_INTERFERENCE_NO_COLLISIONS:
						SINR_dB = signalTerminating - noiseFloor;
						InterferenceForSignalTerminating = -200.0;
						break;

					case SIMPLE_COLLISION_MODEL:
						if (InterferenceForSignalTerminating > -200.0) SINR_dB = -20.0;
						break;

					case ADDITIVE_INTERFERENCE_MODEL:
						SINR_dB = signalTerminating - addPower_dBm(InterferenceForSignalTerminating, noiseFloor);
						break;

					case COMPLEX_INTERFERENCE_MODEL:
						opp_error("The specified \"Collision model\" in the omnetpp.ini file is not yet supported/implemented.");
						break;

					default :
						opp_error("Error in specifying \"Collision model\" in the omnetpp.ini.");
				}
				
				if (SINR_dB >= thresholdSNRdB) {
					prob = calculateProb(SINR_dB, message->byteLength());
				}
				else
				{
					/* the tx failed. Certailnly because of interference*/
					stats[srcAddr].TxFailedInterference++;
					stats[*it1].RxFailedInterference++;
					continue; // go to the next affected node
				}

				if ( (prob == 1.0) || ( genk_dblrand(1) <= prob ) )
				{
					/* Send it! we cannot send msg multiple times, we have to create copies.
					 * Notice that we are using random number generator 1, not 0
					 */
					cMessage *copy = (cMessage *) msg->dup();
					WChannel_GenericMessage *wcMsg = check_and_cast<WChannel_GenericMessage *>(copy);
					wcMsg->setRssi(signalTerminating);
					send(wcMsg, "toNode", *it1);

					/* update the stats, we have an actual reception */
					if (InterferenceForSignalTerminating > -200.0)
					{
						stats[srcAddr].TxReachedInterference++;
						stats[*it1].RxReachedInterference++;
					}
					else
					{
						stats[srcAddr].TxReachedNoInterference++;
						stats[*it1].RxReachedNoInterference++;
					}
				}

				else  // reception was not successful
				{	
					/* update the stats */
					if (InterferenceForSignalTerminating > -200.0)
					{
						stats[srcAddr].TxFailedInterference++;
						stats[*it1].RxFailedInterference++;
					}
					else
					{
						stats[srcAddr].TxFailedNoInterference++;
						stats[*it1].RxFailedNoInterference++;
					}
				}

			} //for it1

			/* Now that we are done processing the msg we delete the whole list
			 * nodesAffectedByTransmitter[srcAddr], since srcAddr in not TXing anymore.
			 */
			nodesAffectedByTransmitter[srcAddr].erase(nodesAffectedByTransmitter[srcAddr].begin(),nodesAffectedByTransmitter[srcAddr].end());

			break;
		}//case WC_PKT_END_TX


		default:
		{
			CASTALIA_DEBUG << "\n[WChannel]:\n Warning: Wireless Channel received unknown message!";
			break;
		}
	}

	delete msg;
}


void WirelessChannel::finish()
{
	if(printStatistics)
	{
		/* Output aggregate transmission/reception info useful for debugging purposes */
		for(int i = 0;  i < numOfNodes; i++)
		{
			int totalTxFailed  = stats[i].TxFailedNoInterference + stats[i].TxFailedInterference;
			int totalTxReached = stats[i].TxReachedNoInterference + stats[i].TxReachedInterference;
			int totalRxFailed  = stats[i].RxFailedNoInterference + stats[i].RxFailedInterference;
			int totalRxReached = stats[i].RxReachedNoInterference + stats[i].RxReachedInterference;

			CASTALIA_DEBUG << "\n\n## Wireless Channel --> Node[" << i << "] ##";
			CASTALIA_DEBUG << "\n - Transmissions = " << stats[i].transmissions;
			CASTALIA_DEBUG << "\n      These transmissions resulted in " << totalTxFailed + totalTxReached  << " possible receptions";
			CASTALIA_DEBUG << "\n      at _other_ nodes. From these possible receptions:";
			CASTALIA_DEBUG << "\n         " << totalTxReached << " Reached. More specifically";
			CASTALIA_DEBUG << "\n           " << stats[i].TxReachedNoInterference << " without interference, " << stats[i].TxReachedInterference << " despite interference";
			CASTALIA_DEBUG << "\n         " << totalTxFailed << " Failed. More specifically";
			CASTALIA_DEBUG << "\n           " << stats[i].TxFailedInterference << " with interference, " << stats[i].TxFailedNoInterference << " despite NO interference";

			CASTALIA_DEBUG << "\n      We also had " << stats[i].TxFailedTemporalFades << " lost reception opportunities";
			CASTALIA_DEBUG << "\n      (not counted above) due to temporal fades";
			CASTALIA_DEBUG << "\n - Receptions: ";
			CASTALIA_DEBUG << "\n     " << totalRxReached << " Reached. More specifically";
			CASTALIA_DEBUG << "\n        " << stats[i].RxReachedNoInterference << " without interference, " << stats[i].RxReachedInterference << " despite interference";
			CASTALIA_DEBUG << "\n     " << totalRxFailed << " Failed. More specifically";
			CASTALIA_DEBUG << "\n        " << stats[i].RxFailedInterference << " with interference, " << stats[i].RxFailedNoInterference << " despite NO interference";

		}
	}


	/*****************************************************
	 * Delete dynamically allocated arrays. Some allocate
	 * lists of objects so they need an extra nested loop
	 * to properly deallocate all these objects
	 *****************************************************/

	/* delete pathLoss */
	for (int i = 0; i < numOfSpaceCells; i++)
	{
		list<PathLossElement *>::iterator it1;
		for (it1=pathLoss[i].begin(); it1 != pathLoss[i].end(); it1++)
		{
			delete (*it1);		// deallocate the memory occupied by the object
		}
	}
	delete[] pathLoss;   // the delete[] operator releases memory allocated with new []


	/* delete currentSignalAtReceiver */
	for (int i = 0; i < numOfNodes; i++)
	{
		list<ReceivedSignalElement *>::iterator it1;
		for (it1=currentSignalAtReceiver[i].begin(); it1 != currentSignalAtReceiver[i].end(); it1++)
		{
			delete (*it1);				// deallocate the memory occupied by the object
		}
	}
	delete[] currentSignalAtReceiver;   // the delete[] operator releases memory allocated with new []

	/* delete nodesAffectedByTransmitter */
	delete[] nodesAffectedByTransmitter;   // the delete[] operator releases memory allocated with new []

	/* delete cellOccupation */
	delete[] cellOccupation;   // the delete[] operator releases memory allocated with new []

	/* delete isNodeCarrierSensing */
	delete[] isNodeCarrierSensing;


	/* delete nodeLocation */
	delete[] nodeLocation;

	/* delete stats */
	delete[] stats;

	if (modulationType == MODULATION_CUSTOM) delete[] customModulationArray;
	if (temporalModelDefined) delete temporalModel;
	
	//close the output stream that CASTALIA_DEBUG is writing to
	DebugInfoWriter::closeStream();
}

float WirelessChannel::calculateProb(float SNR_dB, int packetByteSize)
{
        if (SNR_dB > goodLinkSNRdB) return 1.0;
	switch (modulationType){

		case MODULATION_FSK:
			return pow( 1.0 - 0.5 * exp( ((-0.5) * noiseBandwidth / dataRate)* pow(10.0, (SNR_dB/10.0))), (8.0*packetByteSize));

		case MODULATION_PSK:
          		return pow( 1.0 - 0.5 * erfc( sqrt(pow(10.0,(SNR_dB/10.0)) * noiseBandwidth / dataRate)), (8.0*packetByteSize));

		case MODULATION_CUSTOM:
		    if (goodLinkSNRdB - SNR_dB < SNR_dB - thresholdSNRdB) {
			for (int i = numOfCustomModulationValues-1; i >= 0; i--) {
			    if (customModulationArray[i].SNR > SNR_dB) continue;
			    if (customModulationArray[i].SNR - SNR_dB < SNR_dB - customModulationArray[i-1].SNR) {
				return pow( 1.0 - customModulationArray[i].BER, 8.0*packetByteSize);
			    } else {
				return pow( 1.0 - customModulationArray[i-1].BER, 8.0*packetByteSize);
			    }
			}
		    } else {
			for (int i = 0; i < numOfCustomModulationValues; i+=2) {
			    if (customModulationArray[i+1].SNR < SNR_dB) continue;
			    if (customModulationArray[i+1].SNR - SNR_dB > SNR_dB - customModulationArray[i].SNR) {
				return pow( 1.0 - customModulationArray[i].BER, 8.0*packetByteSize);
			    } else {
				return pow( 1.0 - customModulationArray[i+1].BER, 8.0*packetByteSize);
			    }
			}
		    }

		case MODULATION_IDEAL:
			return 1.0;

		default:
			return 1.0;
	}

}


void WirelessChannel::readIniFileParameters(void)
{
 	string tempDebugFname(parentModule()->par("debugInfoFilename"));
	DebugInfoWriter::setDebugFileName(tempDebugFname);

	printDebugInfo = par("printDebugInfo");

	printStatistics = par("printStatistics");

	onlyStaticNodes = par("onlyStaticNodes");

	pathLossExponent = par("pathLossExponent");

	sigma = par("sigma");

	bidirectionalSigma = par("bidirectionalSigma");

	PLd0 = par("PLd0");

	d0 = par("d0");

	pathLossMapFile = par("pathLossMapFile");

	PRRMapFile = par("PRRMapFile");

	temporalModelParametersFile = par("temporalModelParametersFile");


	numOfNodes = parentModule()->par("numNodes");
	xFieldSize = parentModule()->par("field_x");
	yFieldSize = parentModule()->par("field_y");
	zFieldSize = parentModule()->par("field_z");
	xCellSize = parentModule()->par("xCellSize");
	yCellSize = parentModule()->par("yCellSize");
	zCellSize = parentModule()->par("zCellSize");

	//find the minimum packet size
	cModule *node_0;
	if(parentModule()->findSubmodule("node", 0) != -1)
		node_0 = check_and_cast<cModule*>(parentModule()->submodule("node", 0));
	else
		opp_error("\n[Wireless Channel]:\n Error in geting a valid reference to  node[0].\n");

	if( node_0->findSubmodule("nodeApplication")  &&
		node_0->findSubmodule("networkInterface") &&
		node_0->submodule("networkInterface")->findSubmodule("MAC") &&
		node_0->submodule("networkInterface")->findSubmodule("Radio") )
	{
		int appOverhead = node_0->submodule("nodeApplication")->par("packetHeaderOverhead");
		int macOverhead = node_0->submodule("networkInterface")->submodule("MAC")->par("macFrameOverhead");
		int radioOverhead = node_0->submodule("networkInterface")->submodule("Radio")->par("phyFrameOverhead");
		maxPacketSize = node_0->submodule("networkInterface")->submodule("Radio")->par("maxPhyFrameSize");
		minPacketSize = (appOverhead + macOverhead + radioOverhead + 2);
		
		dataRate = node_0->submodule("networkInterface")->submodule("Radio")->par("dataRate");

		receiverSensitivity = node_0->submodule("networkInterface")->submodule("Radio")->par("receiverSensitivity");

		noiseBandwidth = node_0->submodule("networkInterface")->submodule("Radio")->par("noiseBandwidth");

		noiseFloor = node_0->submodule("networkInterface")->submodule("Radio")->par("noiseFloor");

		modulationTypeParam = node_0->submodule("networkInterface")->submodule("Radio")->par("modulationType");
		customModulationArray = NULL;
		
		//Modulation type is a string, either IDEAL, FSK, PSK, or CUSTOM. 
		if (strlen(modulationTypeParam) == 0 || strncmp(modulationTypeParam,"IDEAL",5) == 0) {
		    modulationType = MODULATION_IDEAL;
		} else if (strncmp(modulationTypeParam,"FSK",3) == 0) {
		    modulationType = MODULATION_FSK;
		} else if (strncmp(modulationTypeParam,"PSK",3) == 0) {
		    modulationType = MODULATION_PSK;
		} else if (strncmp(modulationTypeParam,"CUSTOM",6) == 0) {
		    modulationType = MODULATION_CUSTOM;
		    
		    /*********************************************************
		     *A custom definition of modulation is a list of comma separated pairs, 
		     *where first value is SNR and second value is BER (bit error rate) associated with the given SNR
		     *********************************************************/

		    //tokenizer will help to convert the string into an array using comma as delimiter
		    cStringTokenizer t(modulationTypeParam + 6,",");
		    vector <string> v = t.asVector();
		    
		    //it is necessary to create and fill the customModulationArray based on the result from tokenizer
		    customModulationArray = new CustomModulation_type[v.size()];
		    numOfCustomModulationValues = v.size();
		    const char *ptr;
		    for (int i = 0; i < v.size(); i++) {
			ptr = v[i].c_str();
			if (parseFloat(ptr, &(customModulationArray[i].SNR))) 
			    opp_error("\n[Wireless Channel]: Illegal syntax for CUSTOM modulation type, unable to parse float from %s\n",ptr);
			if ((i > 0) && (customModulationArray[i-1].SNR >= customModulationArray[i].SNR)) 
			    opp_error("\n[Wireless Channel]: Illegal syntax for CUSTOM modulation type: SNR values are not in increasing order\n");
			while (ptr[0] && ptr[0] != ':') ptr++;
			if (parseFloat(++ptr, &(customModulationArray[i].BER))) 
			    opp_error("\n[Wireless Channel]: Illegal syntax for CUSTOM modulation type, unable to parse float from %s\n",ptr);
			if ((customModulationArray[i].BER < 0) || (customModulationArray[i].BER > 1)) 
			    opp_error("\n[Wireless Channel]: Illegal syntax for CUSTOM modulation type: BER values have to be in range 0 to 1\n");
		    }
		} else {
		    opp_error("\n[Wireless Channel]: Illegal value of parameter \"modulationType\" in omnetpp.ini\n" );
		}

		/* Only one encoding type allowed cyrrently */
		encodingType = node_0->submodule("networkInterface")->submodule("Radio")->par("encodingType");
		switch(encodingType)
		{
			case 0:
				encodingType = ENCODING_NRZ;
				break;

			case 1:
				opp_error("\n[Wireless Channel]: Illegal value of parameter \"encodingType\" in omnetpp.ini. ENCODING_4B5B is not supported yet.\n" );
				break;

			case 2:
				opp_error("\n[Wireless Channel]: Illegal value of parameter \"encodingType\" in omnetpp.ini. ENCODING_MANCHESTER is not supported yet.\n" );
				break;

			case 3:
				opp_error("\n[Wireless Channel]: Illegal value of parameter \"encodingType\" in omnetpp.ini. ENCODING_SECDEC is not supported yet.\n" );
				break;

			default:
				opp_error("\n[Wireless Channel]: Illegal value of parameter \"encodingType\" in omnetpp.ini.\n" );
				break;
		}

		/* extract the maxTxPower from the string module parameter txPowerLevels */
		const char *str = node_0->submodule("networkInterface")->submodule("Radio")->par("txPowerLevels");
		cStringTokenizer tokenizer(str);
		const char *token;
		maxTxPower = 0.0;
		while ((token = tokenizer.nextToken())!=NULL){
			float tempTxPower = atof(token);
			if (tempTxPower > maxTxPower) maxTxPower = tempTxPower;
		}

	}
	else
		opp_error("\n[Wireless Channel]:\n Error in geting a valid reference to  node[0] Application OR NetworkInterface (MAC/Radio) module.\n");



	collisionModel = par("collisionModel");
	switch(collisionModel) {
		case 0:
			collisionModel = NO_INTERFERENCE_NO_COLLISIONS;
			break;

		case 1:
			collisionModel = SIMPLE_COLLISION_MODEL;
			break;

		case 2:
			collisionModel = ADDITIVE_INTERFERENCE_MODEL;
			break;

		case 3:
			collisionModel = COMPLEX_INTERFERENCE_MODEL;
			break;

		default:
			opp_error("\n[Wireless Channel]: Illegal value of parameter \"collisionModel\" in omnetpp.ini\n" );
			break;
	}

} // readIniParameters

//Parsing of custom pathloss map from a file, given by the parameter pathLossMapFile
void WirelessChannel::parsePathLossMap(void)
{
    if (strlen(pathLossMapFile) == 0) return;
    ifstream f(pathLossMapFile);
    if (!f.is_open()) opp_error("\n[Wireless Channel]:\n Error reading from pathLossMapFile %s\n", pathLossMapFile);

    string s; const char *ct; 
    int source, destination; float pathloss_db;
    
    //each line in a file is in the same format: SOURCE>DESTINATION:PL_DB, ... ,DESTINATION:PL_DB
    //based on these values we will update the pathloss array (using updatePathLossElement function)
    while (getline(f,s)) {
	ct = s.c_str();				//ct points to the beginning of a line
	while (ct[0] && (ct[0] == ' ' || ct[0] == '\t')) ct++;
	if (ct[0] == '#') continue;		//check for commentswhile (ct[0]
	if (parseInt(ct,&source)) opp_error("\n[Wireless Channel]:\n Bad syntax in pathLossMapFile\n");
	while (ct[0] && ct[0] != '>') ct++;	//skip untill '>' character
	if (!ct[0]) opp_error("\n[Wireless Channel]:\n Bad syntax in pathLossMapFile\n");	
	cStringTokenizer t(++ct,","); 		//divide the rest of the strig with comma delimiter
	while (ct = t.nextToken()) {
	    if (parseInt(ct,&destination)) opp_error("\n[Wireless Channel]:\n Bad syntax in pathLossMapFile\n");
	    while (ct[0] && ct[0] != ':') ct++;	//skip untill ':' character
	    if (parseFloat(++ct,&pathloss_db)) opp_error("\n[Wireless Channel]:\n Bad syntax in pathLossMapFile\n");	
	    updatePathLossElement(source,destination,pathloss_db);	
	}
    }
}

//Parsing of custom PRR map from a file, given by the parameter PRRMapFile
void WirelessChannel::parsePrrMap(void)
{
    if (strlen(PRRMapFile) == 0) return;
    ifstream f(PRRMapFile);
    if (!f.is_open()) opp_error("\n[Wireless Channel]:\n Error reading from PRRMapFile\n");

    string s; const char *ct;
    int source, destination, packetSize;
    float tmpSNR_db, PRR, sourceTxPower_db;

    //each line in a file is in the same format: SOURCE,PACKET_SIZE,TX_POWER>DESTINATION:PRR, ... ,DESTINATION:PRR
    //given the values of PACKET_SIZE and TX_POWER we are able to convert the given PRR (packet reception rate) value
    //to a pathloss value, which we can use to update the pathloss array (using updatePathLossElement function)
    while (getline(f,s)) {
	ct = s.c_str();				//ct points to the beginning of a line
	while (ct[0] && (ct[0] == ' ' || ct[0] == '\t')) ct++;
	if (ct[0] == '#') continue;		//check for comments
	if (parseInt(ct,&source)) opp_error("\n[Wireless Channel]:\n Bad syntax in PRRMapFile\n");	
	while (ct[0] && ct[0] != ',') ct++;	//skip untill ',' character
	if (parseInt(++ct,&packetSize)) opp_error("\n[Wireless Channel]:\n Bad syntax in PRRMapFile\n");	
	while (ct[0] && ct[0] != ',') ct++;	//skip untill ',' character
	if (parseFloat(++ct,&sourceTxPower_db)) opp_error("\n[Wireless Channel]:\n Bad syntax in PRRMapFile\n");	
	while (ct[0] && ct[0] != '>') ct++;	//skip untill '>' character
	if (!ct[0]) opp_error("\n[Wireless Channel]:\n Bad syntax in PRRMapFile\n");	
	cStringTokenizer t(++ct,",");		//divide the rest of the strig with comma delimiter
	while (ct = t.nextToken()) {
	    if (parseInt(ct,&destination)) opp_error("\n[Wireless Channel]:\n Bad syntax in PRRMapFile\n");
	    while (ct[0] && ct[0] != ':') ct++;	//skip untill ':' character
	    if (parseFloat(ct+1,&PRR)) opp_error("\n[Wireless Channel]:\n Bad syntax in PRRMapFile\n");	
	    if (PRR <= 0) continue;		//if PRR is 0 or below, we cannot receive packets, so skip this element
	    if (PRR >= 1) PRR = 0.9999;		//if PRR is 1, the value for SNR will be -infinity. 
						//To avoid that PRR is set to a value close to 1 but not equal to 1
    	    
    	    //Based on modulation type given PRR value can be converted to SNR value in Db
    	    if (modulationType = MODULATION_FSK) {
		tmpSNR_db = 10.0 * log10((-2.0*(dataRate/noiseBandwidth)) * log( 2.0 * (1.0 -  pow(PRR, 1.0/(8.0*packetSize)))));
	    } else if (modulationType = MODULATION_PSK) {
		tmpSNR_db = 10.0 * log10((dataRate/noiseBandwidth) * pow(erfcInv(2.0 * ( 1.0 - pow(PRR, 1.0/(8.0*packetSize)))), 2.0));
	    } else {
		opp_error("\n[Wireless Channel]:\n Error: PRRMapFile is only compatible with modulation type PSK or FSK\n");
	    }
	    //if (modulationType = MODULATION_CUSTOM) { ... } //not implemented for custom modulation
	    
	    //SNR can be converted to pathloss by using (TX_POWER - NOISE_FLOOR - SNR)
    	    updatePathLossElement(source,destination,sourceTxPower_db - noiseFloor - tmpSNR_db);	
    	}
    }
}

//This function will update a pathloss element for given source and destination cells with a given value of pathloss
//If this pair is already defined in pathloss array, the old value is replaced, otherwise a new pathloss element is created
void WirelessChannel::updatePathLossElement(int src, int dst, float pathloss_db) 
{
    list<PathLossElement *>::iterator it1;
    for ( it1=pathLoss[src].begin(); it1 != pathLoss[src].end(); it1++ )
    {
        if ((*it1)->cellID == dst) {
    	    (*it1)->avgPathLoss = pathloss_db;
    	    return;
	}	    
    }
    pathLoss[src].push_front(new PathLossElement(dst, pathloss_db));
}

//wrapper function for atoi(...) call. returns 1 on error, 0 on success
int WirelessChannel::parseInt(const char * c, int * dst) {
    while (c[0] && (c[0] == ' ' || c[0] == '\t')) c++;
    if (!c[0] || c[0] < '0' || c[0] > '9') return 1;
    *dst = atoi(c);
    return 0;
}

//wrapper function for strtof(...) call. returns 1 on error, 0 on success
int WirelessChannel::parseFloat(const char * c, float * dst) {
    char * tmp;
    *dst = strtof(c,&tmp);
    if (c == tmp) return 1;
    return 0;
}
