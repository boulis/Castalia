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



#include "TunableMacModule.h"

#define MAC_BUFFER_ARRAY_SIZE macBufferSize+1

#define BUFFER_IS_EMPTY  (headTxBuffer==tailTxBuffer)

#define BUFFER_IS_FULL  (getTXBufferSize() >= macBufferSize)

#define CARRIER_SENSE_INTERVAL 0.0001

#define DRIFTED_TIME(time) ((time) * cpuClockDrift)

#define EV   ev.disabled() ? (ostream&)ev : ev

#define CASTALIA_DEBUG (!printDebugInfo)?(ostream&)DebugInfoWriter::getStream():DebugInfoWriter::getStream()



Define_Module(TunableMacModule);



void TunableMacModule::initialize()
{
	readIniFileParameters();

	//--------------------------------------------------------------------------------
	//------- Follows code for the initialization of the class member variables ------

	self = parentModule()->parentModule()->index();

	//get a valid reference to the object of the Radio module so that we can make direct calls to its public methods
	//instead of using extra messages & message types for tighlty couplped operations.
	radioModule = check_and_cast<RadioModule*>(gate("toRadioModule")->toGate()->ownerModule());
	radioDataRate = (double) radioModule->par("dataRate");
	radioDelayForValidCS = ((double) radioModule->par("delayCSValid"))/1000.0; //parameter given in ms in the omnetpp.ini

	phyLayerOverhead = radioModule->par("phyFrameOverhead");

	//get a valid reference to the object of the Resources Manager module so that we can make direct calls to its public methods
	//instead of using extra messages & message types for tighlty couplped operations.
	cModule *parentParent = parentModule()->parentModule();
	if(parentParent->findSubmodule("nodeResourceMgr") != -1)
	{
		resMgrModule = check_and_cast<ResourceGenericManager*>(parentParent->submodule("nodeResourceMgr"));
	}
	else
		opp_error("\n[Mac]:\n Error in geting a valid reference to  nodeResourceMgr for direct method calls.");
	cpuClockDrift = resMgrModule->getCPUClockDrift();


	// If a valid duty cycle is defined, calculate sleepInterval and
	// start the periodic sleep/listen cycle using a self message
	// In order not to have synchronized nodes we indroduce a random delay
	if ((dutyCycle > 0.0) && (dutyCycle < 1.0))
	{
		setRadioState(MAC_2_RADIO_ENTER_SLEEP);
		sleepInterval = listenInterval * ((1.0-dutyCycle)/dutyCycle);
	}
	else
		sleepInterval = -1.0;

	macState = MAC_STATE_DEFAULT;
	if(printStateTransitions)
		CASTALIA_DEBUG << "\n[Mac_" << self <<"] t= " << simTime() << ": State changed to MAC_STATE_DEFAULT (initializing)";


	schedTXBuffer = new MAC_GenericFrame*[MAC_BUFFER_ARRAY_SIZE];
	headTxBuffer = 0;
	tailTxBuffer = 0;

	maxSchedTXBufferSizeRecorded = 0;

	epsilon = 0.000001;

	disabled = 1;

	beaconSN = 0;

	backoffTimes = 0;

	//duty cycle references
	dutyCycleSleepMsg = NULL;
	dutyCycleWakeupMsg = NULL;
	
	selfCheckTXBufferMsg = NULL;
	
	selfExitCSMsg = NULL;
	
	rxOutMessage = NULL;

	potentiallyDroppedBeacons = 0;
	potentiallyDroppedDataFrames = 0;
}


void TunableMacModule::handleMessage(cMessage *msg)
{
	int msgKind = msg->kind();


	if((disabled) && (msgKind != APP_NODE_STARTUP))
	{
		delete msg;
		msg = NULL;		// safeguard
		return;
	}


	switch (msgKind)
	{

		/*--------------------------------------------------------------------------------------------------------------
		 * Sent by the Network submodule (which received that from Application) in order to start/switch-on the MAC submodule.
		 *--------------------------------------------------------------------------------------------------------------*/
		case APP_NODE_STARTUP:
		{
			disabled = 0;

			setRadioState(MAC_2_RADIO_ENTER_LISTEN);

			if ((dutyCycle > 0.0) && (dutyCycle < 1.0))
			{
				if ( (dutyCycleSleepMsg != NULL) && (dutyCycleSleepMsg->isScheduled())  )
						cancelAndDelete(dutyCycleSleepMsg);
	
				dutyCycleSleepMsg = new MAC_ControlMessage("put_radio_to_sleep", MAC_SELF_SET_RADIO_SLEEP);
	
				scheduleAt(simTime() + DRIFTED_TIME(listenInterval), dutyCycleSleepMsg);
			}

			break;
		}


		/*--------------------------------------------------------------------------------------------------------------
		 * Sent by the Network module. We need to push it into the buffer and schedule its forwarding to the Radio buffer for TX.
		 *--------------------------------------------------------------------------------------------------------------*/
		case NETWORK_FRAME:
		{
			if(!BUFFER_IS_FULL)
			{
				Network_GenericFrame *rcvNetDataFrame = check_and_cast<Network_GenericFrame*>(msg);
				//create the MACFrame from the Network Data Packet (encapsulation)
				MAC_GenericFrame *dataFrame;

				for( int i = 0; i< numTx; i++ )	// place at the TX buffer as many copies of the message as allowed by the parameter numTx
				{
					if (genk_dblrand(0) < probTx )  // for each try check with probTx, notice that we are using generator 1 not 0
					{
							// get the random offset for this try
							double random_offset = genk_dblrand(1) * randomTxOffset;
							
							// create a new copy of the dataFrame
							char buff[50];
							sprintf(buff, "MAC Data frame (%f)", simTime());
							dataFrame = new MAC_GenericFrame(buff, MAC_FRAME);
							if(encapsulateNetworkFrame(rcvNetDataFrame, dataFrame))
							{
								dataFrame->setKind(MAC_FRAME_SELF_PUSH_TX_BUFFER); //the dataFrame is a pointer of type MAC_GenericFrame*
								scheduleAt(simTime() + DRIFTED_TIME(random_offset + i*reTxInterval), dataFrame);

								// schedule a new TX
								scheduleAt(simTime() + DRIFTED_TIME(random_offset + i*reTxInterval) + epsilon, new MAC_ControlMessage("initiate a TX after receiving a NETWORK_FRAME", MAC_SELF_INITIATE_TX));

							}
							else
							{
								cancelAndDelete(dataFrame);
								dataFrame = NULL;
								CASTALIA_DEBUG << "\n[Mac_" << self <<"] t= " << simTime() << ": WARNING: Network module sent to MAC an oversized packet...packet dropped!!\n";
							}

					}
				}

				rcvNetDataFrame = NULL;
				dataFrame = NULL;
			}
			else
			{
				MAC_ControlMessage *fullBuffMsg = new MAC_ControlMessage("MAC buffer is full Radio->Mac", MAC_2_NETWORK_FULL_BUFFER);

				send(fullBuffMsg, "toNetworkModule");
				
				CASTALIA_DEBUG << "\n[Mac_" << self <<"] t= " << simTime() << ": WARNING: SchedTxBuffer FULL!!! Network frame is discarded";
			}

			break;
		}//END_OF_CASE(NETWORK_FRAME)
		
		
		
		
		/*--------------------------------------------------------------------------------------------------------------
		 * Control message sent by the MAC (ourself) to initiate a TX either by first requesting Carrier Sense (if it is
		 * enabled from the omnet.ini parameter "carrierSense") or directly by sending a MAC_SELF_CHECK_TX_BUFFER self message
		 *--------------------------------------------------------------------------------------------------------------*/
		case MAC_SELF_INITIATE_TX:
		{
			if(macState == MAC_STATE_DEFAULT) // there is no point to do the following things when the Mac is already in TX mode
			{								  // and moreover if it is in MAC_STATE_EXPECTING_RX or MAC_STATE_CARRIER_SENSING then we should't start a new TX
				if(!(BUFFER_IS_EMPTY))
				{
					if(carrierSense)  //with MAC-triggered carrier sense
					{
						//delayed carrier sense request (delay = random_offset + i * reTxInterval + epsilon)
						scheduleAt(simTime(), new MAC_ControlMessage("Enter carrier sense state MAC->MAC", MAC_SELF_PERFORM_CARRIER_SENSE));

						/*
						 * TODO: it is not mandatory to check our state as both message types :
						 * MAC_SELF_CHECK_TX_BUFFER  and  MAC_SELF_PERFORM_CARRIER_SENSE
						 * when are processed they take into consideration the current state of the MAC module.
						 *
						 * if(macState == MAC_STATE_TX)
						 * 		scheduleAt(simTime(), new MAC_ControlMessage("check schedTXBuffer buffer", MAC_SELF_CHECK_TX_BUFFER)); //we m9ight not need to call MAC_SELF_CHECK_TX_BUFFER again because MAC will TX everything until it the Buffer get empty
						 * else
						 * if(macState == MAC_STATE_DEFAULT)
						 *      scheduleAt(simTime() + epsilon, new MAC_ControlMessage("Enter carrier sense state MAC->MAC", MAC_SELF_PERFORM_CARRIER_SENSE));
						 */
					}
					else  //without MAC-triggered carrier sense
					{
						/*
						 * TODO: OR BETTER
						 *
						 * if(macState != MAC_STATE_CARRIER_SENSING  && MAC_STATE_EXPECTING_RX)
						 *    scheduleAt(simTime() + epsilon, new MAC_ControlMessage("check schedTXBuffer buffer", MAC_SELF_CHECK_TX_BUFFER));
						 */

						// schedule a message that will check the Tx buffer for transmission
						if( (selfCheckTXBufferMsg == NULL) || (!selfCheckTXBufferMsg->isScheduled()) )
						{
							selfCheckTXBufferMsg = new MAC_ControlMessage("check schedTXBuffer buffer", MAC_SELF_CHECK_TX_BUFFER);
							scheduleAt(simTime(), selfCheckTXBufferMsg);
						}
					}
				 }
				 else // the Mac buffer is empty
				 {
					CASTALIA_DEBUG << "\n[Mac_" << self << "] t= " << simTime() << ": WARNING: MAC_SELF_INITIATE_TX received but Mac Buffer is empty, message description: " << msg->name() << "\n";


				 	macState = MAC_STATE_DEFAULT;
				 	if(printStateTransitions)
						CASTALIA_DEBUG << "\n[Mac_" << self <<"] t= " << simTime() << ": State changed to MAC_STATE_DEFAULT (MAC_SELF_INITIATE_TX received and buffer is empty)";


					if ((dutyCycle > 0.0) && (dutyCycle < 1.0))
					{
						if ( (dutyCycleSleepMsg != NULL) && (dutyCycleSleepMsg->isScheduled())  )
								cancelAndDelete(dutyCycleSleepMsg);
			
						dutyCycleSleepMsg = new MAC_ControlMessage("put_radio_to_sleep", MAC_SELF_SET_RADIO_SLEEP);
			
						scheduleAt(simTime() + DRIFTED_TIME(listenInterval), dutyCycleSleepMsg);
					}
				 }
			}
			
			break;
		}
		
		



		/*--------------------------------------------------------------------------------------------------------------
		 * Sent by the MAC submodule itself to send the next Frame to Radio
		 *--------------------------------------------------------------------------------------------------------------*/
		case MAC_SELF_CHECK_TX_BUFFER:
		{
			 if(!(BUFFER_IS_EMPTY))
			 {
			 	 // if there is something to Tx AND we are not expecting Rx AND we are not in MAC_STATE_CARRIER_SENSING
			 	 if ( (macState == MAC_STATE_DEFAULT) || (macState == MAC_STATE_TX) )
			 	 {
			 	 	/*
			 	 	 * **Commented out because we only use only the default power level & TxMode**
			 	 	 *
			 	 	 * setRadioTxMode(CARRIER_SENSE_NONE); //only MAC is performing Carrier Sense if carrierSense==TRUE else no CS at all
			 	 	 * setRadioPowerLevel(0);
			 	 	 */


					// SEND THE BEACON FRAMES TO RADIO BUFFER
					double beacon_offset = 0.0;
					/* First, (if there is a duty cycle) we must "wake up" potential receivers
					 * by sending a train of beacons. The packet we have now is fixed so we will
					 * send so many packets to cover the (sleeping interval) * beaconIntervalFraction
					 * double beacon_offset = 0;
					 */
					if ((dutyCycle > 0.0) && (dutyCycle < 1.0))
					{
						// we do not send the beacons back to back, we leave listenInterval/2 between them
						double beaconTxTime = ((double)(beaconFrameSize+phyLayerOverhead)) * 8.0 / (1000.0 * radioDataRate);

						double step = beaconTxTime + listenInterval/2.0;

						int i;

						char beaconDescr[30];
						for( i = 1; (i-1)*step < sleepInterval * beaconIntervalFraction; i++)
						{
							sprintf(beaconDescr, "Mac_beacon_Frame_%d", beaconSN);
							MAC_GenericFrame *beaconFrame = new MAC_GenericFrame(beaconDescr, MAC_FRAME);
							createBeaconDataFrame(beaconFrame);

							// send one beacon delayed by the proper amount of time
							sendDelayed(beaconFrame, DRIFTED_TIME((i-1)*step), "toRadioModule");//send beaconFrame to the buffer of the Radio with delay (i-1)*step

							setRadioState(MAC_2_RADIO_ENTER_TX, DRIFTED_TIME((i-1)*step)+epsilon);
							beacon_offset = DRIFTED_TIME((i)*step);
							beaconSN++;
						}

					}



					// SEND THE DATA FRAME TO RADIO BUFFER
					MAC_GenericFrame *dataFrame = popTxBuffer();

					sendDelayed(dataFrame, beacon_offset, "toRadioModule");
					setRadioState(MAC_2_RADIO_ENTER_TX, beacon_offset+epsilon);
					
					double dataTXtime = ((double)(dataFrame->byteLength()+phyLayerOverhead) * 8.0 / (1000.0 * radioDataRate));
					
					//schedule the selfCheckTXBufferMsg but firstly cancel any previous scheduling
					selfCheckTXBufferMsg = new MAC_ControlMessage("check schedTXBuffer buffer", MAC_SELF_CHECK_TX_BUFFER);
					scheduleAt(simTime()+ beacon_offset + dataTXtime + epsilon, selfCheckTXBufferMsg);

					dataFrame = NULL;
					
					if(macState == MAC_STATE_DEFAULT)
						macState = MAC_STATE_TX;
			 	 }
			 }
			 else //MAC buffer is empty
			 {
			 	if(macState == MAC_STATE_CARRIER_SENSING)
			 	{
			 		if (( selfExitCSMsg != NULL ) && (  selfExitCSMsg->isScheduled() ))
			 		{
				 		cancelAndDelete(selfExitCSMsg);
						selfExitCSMsg = NULL;
			 		}
			 	}
			 	else
			 	if(macState == MAC_STATE_EXPECTING_RX)
			 	{
			 		if (( rxOutMessage != NULL ) && (  rxOutMessage->isScheduled() ))
			 		{
				 		cancelAndDelete(rxOutMessage);
						rxOutMessage = NULL;
			 		}
			 	}
			 		 
			 	macState = MAC_STATE_DEFAULT;
			 	
			 	if(printStateTransitions)
					CASTALIA_DEBUG << "\n[Mac_" << self <<"] t= " << simTime() << ": State changed to MAC_STATE_DEFAULT (MAC_SELF_CHECK_TX_BUFFER received and buffer is empty)";

							
				if ((dutyCycle > 0.0) && (dutyCycle < 1.0))
				{
					if ( (dutyCycleSleepMsg != NULL) && (dutyCycleSleepMsg->isScheduled())  )
							cancelAndDelete(dutyCycleSleepMsg);
		
					dutyCycleSleepMsg = new MAC_ControlMessage("put_radio_to_sleep", MAC_SELF_SET_RADIO_SLEEP);
		
					scheduleAt(simTime() + DRIFTED_TIME(listenInterval), dutyCycleSleepMsg);
				}
				
				selfCheckTXBufferMsg = NULL;
			 }

			break;
		}


		case MAC_SELF_SET_RADIO_SLEEP:
		{
			if ((dutyCycle > 0.0) && (dutyCycle < 1.0))
			{
				if(macState == MAC_STATE_DEFAULT) //not in MAC_STATE_TX   OR   MAC_STATE_CARRIER_SENSING    OR    MAC_STATE_EXPECTING_RX
				{
					setRadioState(MAC_2_RADIO_ENTER_SLEEP);

					if ( (dutyCycleWakeupMsg != NULL) && (dutyCycleWakeupMsg->isScheduled()) )
						cancelAndDelete(dutyCycleWakeupMsg);
	
					dutyCycleWakeupMsg = new MAC_ControlMessage("wake_up_radio", MAC_SELF_WAKEUP_RADIO);
	
					scheduleAt(simTime() + DRIFTED_TIME(sleepInterval), dutyCycleWakeupMsg);
				}
				
			}
			else
			{
				if ( (dutyCycleWakeupMsg != NULL) && (dutyCycleWakeupMsg->isScheduled()) )
						cancelAndDelete(dutyCycleWakeupMsg);
				dutyCycleWakeupMsg = NULL;
					
				if ( (dutyCycleSleepMsg != NULL) && (dutyCycleSleepMsg->isScheduled())  )
					cancelAndDelete(dutyCycleSleepMsg);
			}
			
			dutyCycleSleepMsg = NULL;
			
			break;
		}


		/*--------------------------------------------------------------------------------------------------------------
		 * Control message sent by the MAC (ourself) to try to wake up the radio.
		 *--------------------------------------------------------------------------------------------------------------*/
		case MAC_SELF_WAKEUP_RADIO:
		{
			if ((dutyCycle > 0.0) && (dutyCycle < 1.0))
			{
				if(macState == MAC_STATE_DEFAULT) //not in MAC_STATE_TX   OR   MAC_STATE_CARRIER_SENSING    OR    MAC_STATE_EXPECTING_RX
				{
					setRadioState(MAC_2_RADIO_ENTER_LISTEN);
	
					if ( (dutyCycleSleepMsg != NULL) && (dutyCycleSleepMsg->isScheduled())  )
						cancelAndDelete(dutyCycleSleepMsg);
	
					dutyCycleSleepMsg = new MAC_ControlMessage("put_radio_to_sleep", MAC_SELF_SET_RADIO_SLEEP);
	
					scheduleAt(simTime() + DRIFTED_TIME(listenInterval), dutyCycleSleepMsg);
				}
			}
			else
			{
				setRadioState(MAC_2_RADIO_ENTER_LISTEN);
				
				if ( (dutyCycleWakeupMsg != NULL) && (dutyCycleWakeupMsg->isScheduled()) )
						cancelAndDelete(dutyCycleWakeupMsg);
					
				if ( (dutyCycleSleepMsg != NULL) && (dutyCycleSleepMsg->isScheduled())  )
					cancelAndDelete(dutyCycleSleepMsg);
				dutyCycleSleepMsg = NULL;
			}
			
			dutyCycleWakeupMsg = NULL;
			
			break;
		}
		


		/*--------------------------------------------------------------------------------------------------------------
		 * Data Frame sent by the MAC module itself in order to be pushed/inserted into the MAC TX buffer and to be forwarded afterwards to the Radio module for TX
		 *--------------------------------------------------------------------------------------------------------------*/
		case MAC_FRAME_SELF_PUSH_TX_BUFFER:
		{
			if(!(BUFFER_IS_FULL))
			{
				MAC_GenericFrame *dataFrame = check_and_cast<MAC_GenericFrame*>(msg);

				// create a new copy of the message because dataFrame will be deleted outside the switch statement
				MAC_GenericFrame *duplMsg = (MAC_GenericFrame *)dataFrame->dup(); //because theFrame will be deleted after the main switch in the handleMessage()
				pushBuffer(duplMsg);
			}
			else
			{
				MAC_ControlMessage *fullBuffMsg = new MAC_ControlMessage("MAC buffer is full Radio->Mac", MAC_2_NETWORK_FULL_BUFFER);

				send(fullBuffMsg, "toNetworkModule");
				
				CASTALIA_DEBUG  << "\n[Mac_"<< self <<"] t= " << simTime() << ": WARNING: SchedTxBuffer FULL!!! Network frame is discarded.\n";
			}

			break;
		}



		/*--------------------------------------------------------------------------------------------------------------
		 * Data Frame Received from the Radio submodule (the data frame can be a Data packet or a beacon packet)
		 *--------------------------------------------------------------------------------------------------------------*/
		case MAC_FRAME:
		{
			MAC_GenericFrame *rcvFrame;
			rcvFrame = check_and_cast<MAC_GenericFrame*>(msg);

			if(macState != MAC_STATE_TX)
			{
				if(isBeacon(rcvFrame)) //the received message is a Beacon
				{

					/*
					 * If this is the first beacon we should enter state MAC_STATE_EXPECTING_RX.
					 * In this state we cannot go to sleep or Tx.
					 * Also schedule a self event to get out of it after sleepInterval.
					 * We might get out of MAC_STATE_EXPECTING_RX earlier if we receive a data packet.
					 */
					 
					if (macState != MAC_STATE_EXPECTING_RX)
					{
						if ( (macState == MAC_STATE_CARRIER_SENSING) && (selfExitCSMsg != NULL) && (selfExitCSMsg->isScheduled()) )
						{
								cancelAndDelete(selfExitCSMsg);
								selfExitCSMsg = NULL;
						}

						macState = MAC_STATE_EXPECTING_RX;
						
						if(printStateTransitions)
							CASTALIA_DEBUG << "\n[Mac_" << self <<"] t= " << simTime() << ": State changed to MAC_STATE_EXPECTING_RX (MAC_FRAME [beacon] received from Radio)";
						
						if (( rxOutMessage != NULL ) && (  rxOutMessage->isScheduled() ))
						{
							cancelAndDelete(rxOutMessage);  //just in case
						}
						
						rxOutMessage = new MAC_ControlMessage("get out of RADIO_RX, timer expired", MAC_SELF_EXIT_EXPECTING_RX);
						scheduleAt(simTime() + DRIFTED_TIME(sleepInterval), rxOutMessage);						
					}

				}
				else  //the received message is a Data Frame
				{
					if(macState != MAC_STATE_CARRIER_SENSING)
					{
						/*
						 * If there was a preceding train of beacons get out of RADIO_RX now.
						 * This means if there is a MAC_SELF_EXIT_EXPECTING_RX message in the queue (sign of a beacon) just cancel and reschedule that message.
						 *
						 */
						 
						 if (( rxOutMessage != NULL ) && (  rxOutMessage->isScheduled() ))
						 {
					 		cancelAndDelete(rxOutMessage);
							rxOutMessage = NULL;
						 }
						 
						 if(macState == MAC_STATE_EXPECTING_RX)
						 {
						 	rxOutMessage = new MAC_ControlMessage("get out of RADIO_RX, data received", MAC_SELF_EXIT_EXPECTING_RX);
						 	scheduleAt(simTime(), rxOutMessage);
						 }
						 else
						 {
						 	setRadioState(MAC_2_RADIO_ENTER_LISTEN); //radio should be already in LISTEN mode.
						 	
						 	// if there is a duty cycle, make sure that the node will not go to sleep right after this reception of the data frame  
						 	// because other frames may follow this frame.
						 	if ((dutyCycle > 0.0) && (dutyCycle < 1.0))
							{
								if ( (dutyCycleSleepMsg != NULL) && (dutyCycleSleepMsg->isScheduled())  )
										cancelAndDelete(dutyCycleSleepMsg);
					
								dutyCycleSleepMsg = new MAC_ControlMessage("put_radio_to_sleep", MAC_SELF_SET_RADIO_SLEEP);
					
								scheduleAt(simTime() + DRIFTED_TIME(listenInterval), dutyCycleSleepMsg);
							}
						 }
					}
				

					//control the destination of the message and drop packets accordingly
					int destinationID = rcvFrame->getHeader().destID;
					if( (destinationID == self) || (destinationID == MAC_BROADCAST_ADDR) )
					{
						//DELIVER THE DATA PACKET TO Network MODULE
						Network_GenericFrame *netDataFrame;
						//No need to create a new message because of the decapsulation: netDataFrame = new Network_GenericFrame("Network frame MAC->Network", NETWORK_FRAME);
		
						//decaspulate the received MAC frame and create a valid Network_GenericFrame
						netDataFrame = check_and_cast<Network_GenericFrame *>(rcvFrame->decapsulate());
	
						netDataFrame->setRssi(rcvFrame->getRssi());
		
						//send the App_GenericDataPacket message to the Application module
						send(netDataFrame, "toNetworkModule");
					}
				}

			}
			else //(macState == MAC_STATE_TX)
			{
				/*
				 * What if MAC is in TX mode and receives a MAC frame from the radio module?
				 * Currently we discard it.
				 */
				
				if(printPotentiallyDropped)
				{
					if(isBeacon(rcvFrame))
						potentiallyDroppedBeacons++;
					else
						potentiallyDroppedDataFrames++;
				}
			}

			break;
		}
		
		
		
		
		/*--------------------------------------------------------------------------------------------------------------
		 * Sent by the Radio module in order for the MAC module to update its state to MAC_STATE_TX
		 *--------------------------------------------------------------------------------------------------------------*/
		case RADIO_2_MAC_STARTED_TX:
		{
			if(macState == MAC_STATE_CARRIER_SENSING)
			{
				//the carrier sense operation is canceled so the pending MAC_SELF_EXIT_CARRIER_SENSE message must be cancelled.
		 		if (( selfExitCSMsg != NULL ) && (  selfExitCSMsg->isScheduled() ))
		 		{
			 		cancelAndDelete(selfExitCSMsg);
					selfExitCSMsg = NULL;
		 		}
			 	
				macState = MAC_STATE_DEFAULT;
				
				if(printStateTransitions)
					CASTALIA_DEBUG << "\n[Mac_" << self <<"] t= " << simTime() << ": State changed to MAC_STATE_DEFAULT (RADIO_2_MAC_STARTED_TX received when MAC_STATE_CARRIER_SENSING)";

				if( (selfCheckTXBufferMsg == NULL) || (!selfCheckTXBufferMsg->isScheduled()) )
				{
					selfCheckTXBufferMsg = new MAC_ControlMessage("check schedTXBuffer buffer", MAC_SELF_CHECK_TX_BUFFER);
					scheduleAt(simTime(), selfCheckTXBufferMsg);
				}
			}
			else
			if(macState == MAC_STATE_DEFAULT)
			{
				if( (selfCheckTXBufferMsg == NULL) || (!selfCheckTXBufferMsg->isScheduled()) )
				{
					selfCheckTXBufferMsg = new MAC_ControlMessage("check schedTXBuffer buffer", MAC_SELF_CHECK_TX_BUFFER);
					scheduleAt(simTime(), selfCheckTXBufferMsg);
				}
			}
			else
			{
				// do nothing:
				//  a) if (macState == MAC_STATE_EXPECTING_RX)  --->  keep waiting for the data packet    OR
				//  b) if (macState == MAC_STATE_TX)  --->  just keep the current state in TX
			}
			
			break;
		}



		/*--------------------------------------------------------------------------------------------------------------
		 * Sent by the Radio module in order the MAC module to exit the MAC_STATE_TX state
		 *--------------------------------------------------------------------------------------------------------------*/
		case RADIO_2_MAC_STOPPED_TX:
		{
			/*if( (macState == MAC_STATE_TX) || (macState = MAC_STATE_DEFAULT) )
			{
				if( (selfCheckTXBufferMsg == NULL) || (!selfCheckTXBufferMsg->isScheduled()) )
				{
					selfCheckTXBufferMsg = new MAC_ControlMessage("check schedTXBuffer buffer", MAC_SELF_CHECK_TX_BUFFER);
					scheduleAt(simTime(), selfCheckTXBufferMsg);
				}
			}*/
			
			break;
		}
		



		/*--------------------------------------------------------------------------------------------------------------
		 * Received by ourselves. It was scheduled DRIFTED_TIME(sleepInterval) before, when we received the first beacon, so that we
		 * won't stay trapped in the MAC_STATE_EXPECTING_RX waiting for the Data Frame to come.
		 *--------------------------------------------------------------------------------------------------------------*/
		case MAC_SELF_EXIT_EXPECTING_RX:
		{
			if (macState != MAC_STATE_EXPECTING_RX)
			{
				CASTALIA_DEBUG << "\n[Mac_"<< self <<"] t= " << simTime() << ": WARNING: received MAC_SELF_EXIT_EXPECTING_RX message while not in MAC_STATE_EXPECTING_RX state. Node: " << self << "  at time: " << simTime() << ", message description: " << msg->name() << "\n";
			}
			else
			{
				macState = MAC_STATE_DEFAULT;
				
				if(printStateTransitions)
					CASTALIA_DEBUG  << "\n[Mac_" << self <<"] t= " << simTime() << ": State changed to MAC_STATE_DEFAULT (MAC_SELF_EXIT_EXPECTING_RX received)";
				
				setRadioState(MAC_2_RADIO_ENTER_LISTEN); //radio should be already in LISTEN mode. I use this line just in case

				//check if empty buffer and do TX (with or without Carrier Sense )
				scheduleAt(simTime(), new MAC_ControlMessage("initiate a TX after exiting MAC_STATE_EXPECTING_RX state", MAC_SELF_INITIATE_TX));

			}

			rxOutMessage = NULL;		// since we received the message, reset the pointer

			break;
		}



		/*--------------------------------------------------------------------------------------------------------------
		 * Sent by the MAC submodule itself to start carrier sensing
		 *--------------------------------------------------------------------------------------------------------------*/
		case MAC_SELF_PERFORM_CARRIER_SENSE:
		{
			if(macState == MAC_STATE_DEFAULT)
			{
				int isCarrierSenseValid_ReturnCode; //during this procedure we check if the carrier sense indication of the Radio is valid.
				isCarrierSenseValid_ReturnCode = radioModule->isCarrierSenseValid();

				if(isCarrierSenseValid_ReturnCode == 1) //carrier sense indication of Radio is Valid
				{
					// send the delayed message with the command to perform Carrier Sense to the radio
					MAC_ControlMessage *csMsg = new MAC_ControlMessage("carrier sense strobe MAC->radio", MAC_2_RADIO_SENSE_CARRIER);
					csMsg->setSense_carrier_interval(CARRIER_SENSE_INTERVAL);
					send(csMsg, "toRadioModule");

					if( (selfExitCSMsg != NULL) && (selfExitCSMsg->isScheduled()) )
						cancelAndDelete(selfExitCSMsg);
					selfExitCSMsg = new MAC_ControlMessage("Exit carrier sense state MAC->MAC", MAC_SELF_EXIT_CARRIER_SENSE);
					scheduleAt(simTime() + CARRIER_SENSE_INTERVAL + epsilon, selfExitCSMsg);

					macState = MAC_STATE_CARRIER_SENSING;
					if(printStateTransitions)
						CASTALIA_DEBUG << "\n[Mac_" << self <<"] t= " << simTime() << ": State changed to MAC_STATE_CARRIER_SENSING (MAC_SELF_PERFORM_CARRIER_SENSE received)";
				}
				else //carrier sense indication of Radio is NOT Valid and isCarrierSenseValid_ReturnCode holds the cause for the non valid carrier sense indication
				{
					switch(isCarrierSenseValid_ReturnCode)
					{
						case RADIO_IN_TX_MODE:
						{
							//send the packet (+ the precending beacons) to the Radio Buffer  by sending a message to ourselves
							if( (selfCheckTXBufferMsg == NULL) || (!selfCheckTXBufferMsg->isScheduled()) )
							{
								selfCheckTXBufferMsg = new MAC_ControlMessage("check schedTXBuffer buffer", MAC_SELF_CHECK_TX_BUFFER);
								scheduleAt(simTime(), selfCheckTXBufferMsg);
							}
							
							break;
						}

						case RADIO_SLEEPING:
						{
							//wake up the radio
							setRadioState(MAC_2_RADIO_ENTER_LISTEN);
							//send to ourselves a MAC_SELF_PERFORM_CARRIER_SENSE with delay equal to the time needed by the radio to have a valid CS indication after switching to LISTENING state
							scheduleAt(simTime() + DRIFTED_TIME(radioDelayForValidCS) + epsilon, new MAC_ControlMessage("Enter carrier sense state MAC->MAC", MAC_SELF_PERFORM_CARRIER_SENSE));
							break;
						}

						case RADIO_NON_READY:
						{
							//send to ourselves a MAC_SELF_PERFORM_CARRIER_SENSE with delay equal to the time needed by the radio to have a valid CS indication after switching to LISTENING state
							scheduleAt(simTime() + DRIFTED_TIME(radioDelayForValidCS), new MAC_ControlMessage("Enter carrier sense state MAC->MAC", MAC_SELF_PERFORM_CARRIER_SENSE));

							break;
						}

						default:
						{
							CASTALIA_DEBUG << "\n[Mac_"<< self <<"] t= " << simTime() << ": WARNING: In MAC module, radioModule->isCarrierSenseValid(reasonNonValid) return invalid reasonNonValid.\n";
							break;
						}
					}//end_switch
				}
			}
			
			break;
		}



		/*--------------------------------------------------------------------------------------------------------------
		 * Sent by the MAC submodule (ourself) to exit the MAC_STATE_CARRIER_SENSING (carrier is free)
		 *--------------------------------------------------------------------------------------------------------------*/
		case MAC_SELF_EXIT_CARRIER_SENSE:
		{			
			if(macState == MAC_STATE_CARRIER_SENSING)
			{
				macState = MAC_STATE_DEFAULT;
				if(printStateTransitions)
						CASTALIA_DEBUG << "\n[Mac_" << self <<"] t= " << simTime() << ": State changed to MAC_STATE_DEFAULT (MAC_SELF_EXIT_CARRIER_SENSE received when MAC_STATE_CARRIER_SENSING)";
				
				// schedule a message that will check the Tx buffer for transmission
				if( (selfCheckTXBufferMsg == NULL) || (!selfCheckTXBufferMsg->isScheduled()) )
				{
					selfCheckTXBufferMsg = new MAC_ControlMessage("check schedTXBuffer buffer", MAC_SELF_CHECK_TX_BUFFER);
					scheduleAt(simTime(), selfCheckTXBufferMsg);
				}
				
			}

			backoffTimes = 0;	// the channel is clear, so we reset the backoffTimes counter.

			selfExitCSMsg = NULL;

			break;
		}


		/*--------------------------------------------------------------------------------------------------------------
		 * Sent by the Radio submodule as a response for our (MAC's) previously sent directive to perform CS (carrier is not clear)
		 *--------------------------------------------------------------------------------------------------------------*/
		case RADIO_2_MAC_SENSED_CARRIER:
		{
			if(macState == MAC_STATE_CARRIER_SENSING)
			{
				macState = MAC_STATE_DEFAULT;
				if(printStateTransitions)
					CASTALIA_DEBUG << "\n[Mac_" << self <<"] t= " << simTime() << ": State changed to MAC_STATE_DEFAULT (RADIO_2_MAC_SENSED_CARRIER received when MAC_STATE_CARRIER_SENSING)";

				double backoffTimer = 0;
				backoffTimes++;

				switch(backoffType)
				{
					case BACKOFF_SLEEP_INT:{
							backoffTimer = sleepInterval;
							break;
							}

					case BACKOFF_CONSTANT:{
							backoffTimer = backoffBaseValue;
							break;
							}

					case BACKOFF_MULTIPLYING:{
							backoffTimer = (double) (backoffBaseValue * backoffTimes);
							break;
							}

					case BACKOFF_EXPONENTIAL:{
							backoffTimer = backoffBaseValue * pow(2.0, (double)(((backoffTimes-1) < 0)?0:backoffTimes-1));
							break;
							}
				}

				/***
				 * Go directly to sleep. One could say "wait for listenInterval in the case we receive something". This is highly
				 * improbable though as most of the cases we start the process of carrier sensing from a sleep state (so most
				 * likely we have missed the start of the packet). In the case we are in listen mode, if someone else
				 * is transmitting then we would have entered EXPECTING_RX mode and the carrier sense (and TX) would be postponed.
				 ***/

				if(randomBackoff)
					backoffTimer = genk_dblrand(1)*backoffTimer;

				if ((dutyCycle > 0.0) && (dutyCycle < 1.0))
				 {
					if(backoffTimer != 0.0)
					{
						if ( (dutyCycleSleepMsg != NULL) && (dutyCycleSleepMsg->isScheduled())  )
							cancelAndDelete(dutyCycleSleepMsg);
						dutyCycleSleepMsg = new MAC_ControlMessage("put_radio_to_sleep", MAC_SELF_SET_RADIO_SLEEP);
						scheduleAt(simTime() + DRIFTED_TIME(listenInterval), dutyCycleSleepMsg);
						
						scheduleAt(simTime() + DRIFTED_TIME(backoffTimer+listenInterval), new MAC_ControlMessage("try transmitting after backing off", MAC_SELF_INITIATE_TX));
					}
					else
						scheduleAt(simTime() + DRIFTED_TIME(epsilon), new MAC_ControlMessage("try transmitting after backing off", MAC_SELF_INITIATE_TX));
				 }
				 else
				 {					 
				 	if(backoffTimer != 0.0)
				 		scheduleAt(simTime() + DRIFTED_TIME(backoffTimer), new MAC_ControlMessage("try transmitting after backing off", MAC_SELF_INITIATE_TX));
				 	else
				 		scheduleAt(simTime() + DRIFTED_TIME(epsilon), new MAC_ControlMessage("try transmitting after backing off", MAC_SELF_INITIATE_TX));
				 }

			}
			
			//the carrier is sensed so the pending MAC_SELF_EXIT_CARRIER_SENSE message must be cancelled (so as not to give the
			//false impression that the channel is clear).
			if (( selfExitCSMsg != NULL ) && (  selfExitCSMsg->isScheduled() ))
			{
				cancelAndDelete(selfExitCSMsg);
				selfExitCSMsg = NULL;
			}
			
			break;
		}



		case RADIO_2_MAC_FULL_BUFFER:
		{
			/*
			 * TODO:
			 * add your code here to manage the situation of a full Radio buffer.
			 * Apparently we 'll have to stop sending messages and enter into listen or sleep mode (depending on the MAC protocol that we implement).
			 */
			 CASTALIA_DEBUG << "\n[Mac_"<< self <<"] t= " << simTime() << ": WARNING: RADIO_2_MAC_FULL_BUFFER received because the Radio buffer is full.\n";
			 
			 break;
		}



		/*--------------------------------------------------------------------------------------------------------------
		 * Message sent by the Resource Manager when battery is out of energy.
		 * Node has to shut down.
		 *--------------------------------------------------------------------------------------------------------------*/
		case RESOURCE_MGR_OUT_OF_ENERGY:
		{
			disabled = 1;
			
			break;
		}
		
		
		
		/*--------------------------------------------------------------------------------------------------------------
		 * Message received by the ResourceManager module. It commands the module to stop its operation.
		 *--------------------------------------------------------------------------------------------------------------*/
		case RESOURCE_MGR_DESTROY_NODE:
		{
			disabled = 1;
			
			break;
		}
		

		
		case APPLICATION_2_MAC_SETRADIOTXPOWER:
		{
			App_ControlMessage *ctrlFrame;
			ctrlFrame = check_and_cast<App_ControlMessage*>(msg);
			
			int msgTXPowerLevel = ctrlFrame->getRadioTXPowerLevel();
			setRadioPowerLevel(msgTXPowerLevel);
			
			ctrlFrame = NULL;
			
			break;
		}
		
		case APPLICATION_2_MAC_SETRADIOTXMODE:
		{
			App_ControlMessage *ctrlFrame;
			ctrlFrame = check_and_cast<App_ControlMessage*>(msg);
			
			int msgTXMode = ctrlFrame->getRadioTXMode();

   			switch (msgTXMode)
   			{
   				case 0:{
   						 setRadioTxMode(CARRIER_SENSE_NONE);
   						 break;
   						}
   						
   				case 1:{
   						 setRadioTxMode(CARRIER_SENSE_ONCE_CHECK);
   						 break;
   						}
   						
   				case 2:{
   						 setRadioTxMode(CARRIER_SENSE_PERSISTENT);
   						 break;
   						}
   						
   				default:{
   						CASTALIA_DEBUG << "\n[Mac_" << self <<"]: Application module sent an invalid SETRADIOTXMODE command strobe.";
   						break;
   						}
   			}
			
			ctrlFrame = NULL;
			
			break;
		}
		
		case APPLICATION_2_MAC_SETRADIOSLEEP:
		{
			setRadioState(MAC_2_RADIO_ENTER_SLEEP);

			break;
		}
		
		
		case APPLICATION_2_MAC_SETRADIOLISTEN:
		{
			setRadioState(MAC_2_RADIO_ENTER_LISTEN);
			
			break;
		}
		
		
		case APPLICATION_2_MAC_SETDUTYCYCLE:
		{
			App_ControlMessage *ctrlFrame;
			ctrlFrame = check_and_cast<App_ControlMessage*>(msg);
			
			bool hadDutyCycle = false;
			// check if there was already a non zero valid duty cycle
			if ((dutyCycle > 0.0) && (dutyCycle < 1.0))
				hadDutyCycle = true;
			
			dutyCycle = ctrlFrame->getMacDutyCycle();
			
			// If a valid duty cycle is defined, calculate sleepInterval and
			// start the periodic sleep/listen cycle using a self message
			// In order not to have synchronized nodes we indroduce a random delay
			if ((dutyCycle > 0.0) && (dutyCycle < 1.0))
			{
				sleepInterval = listenInterval * ((1.0-dutyCycle)/dutyCycle);
				
				if(!hadDutyCycle)
				{
					// cancel and delete any possibly scheduled Sleep/Wakeup message
					// however it is not likely to be scheduled if there was no duty cycle enabled before
					// cancel and delete them just in case..
					if ( (dutyCycleSleepMsg != NULL) && (dutyCycleSleepMsg->isScheduled())  )
						cancelAndDelete(dutyCycleSleepMsg);
					if ( (dutyCycleWakeupMsg != NULL) && (dutyCycleWakeupMsg->isScheduled()) )
						cancelAndDelete(dutyCycleWakeupMsg);
					
					
					if ( (dutyCycleSleepMsg != NULL) && (dutyCycleSleepMsg->isScheduled())  )
							cancelAndDelete(dutyCycleSleepMsg);
					dutyCycleSleepMsg = new MAC_ControlMessage("put_radio_to_sleep", MAC_SELF_SET_RADIO_SLEEP);
					scheduleAt(simTime() + DRIFTED_TIME(listenInterval), dutyCycleSleepMsg);
				}	
			}
			else // no duty cycle
			{		
				sleepInterval = -1.0;
			}
								
			ctrlFrame = NULL;
			
			break;
		}
		
		
		
		case APPLICATION_2_MAC_SETLISTENINTERVAL:
		{
			App_ControlMessage *ctrlFrame;
			ctrlFrame = check_and_cast<App_ControlMessage*>(msg);
			
			double tmpValue = ctrlFrame->getMacListenInterval()/1000.0;
			if(tmpValue < 0.0)
			{
				CASTALIA_DEBUG << "\n[Mac__" << self <<"]:\nWarning! Application module tried to set an invalid ListenInterval to the MAC module\n";
			}
			else
			{
				listenInterval = tmpValue;
				
				if ((dutyCycle > 0.0) && (dutyCycle < 1.0))
				{
					sleepInterval = listenInterval * ((1.0-dutyCycle)/dutyCycle);
				}
				else
					sleepInterval = -1.0;
			}
			
			ctrlFrame = NULL;
			
			break;
		}
		
		case APPLICATION_2_MAC_SETBEACONINTERVALFRACTION:
		{
			App_ControlMessage *ctrlFrame;
			ctrlFrame = check_and_cast<App_ControlMessage*>(msg);
			
			double tmpValue = ctrlFrame->getMacBeaconIntervalFraction();
			
			if( (tmpValue < 0.0) || (tmpValue > 1.0) )
			{
				CASTALIA_DEBUG << "\n[Mac_" << self <<"]:\nWarning! Application module tried to set an invalid BeaconIntervalFraction to the MAC module\n";
			}
			else
				beaconIntervalFraction = tmpValue;
			
			ctrlFrame = NULL;
		
			break;
		}
		
		
		
		case APPLICATION_2_MAC_SETPROBTX:
		{
			App_ControlMessage *ctrlFrame;
			ctrlFrame = check_and_cast<App_ControlMessage*>(msg);
			
			double tmpValue = ctrlFrame->getMacProbTX();
			
			if( (tmpValue < 0.0) || (tmpValue > 1.0) )
			{
				CASTALIA_DEBUG << "\n[Mac_" << self <<"]:\nWarning! Application module tried to set an invalid ProbTX to the MAC module\n";
			}
			else
			{
				probTx = tmpValue;
			}
			
			ctrlFrame = NULL;
			
			break;
		}
		
		case APPLICATION_2_MAC_SETNUMTX:
		{
			App_ControlMessage *ctrlFrame;
			ctrlFrame = check_and_cast<App_ControlMessage*>(msg);
			
			int tmpValue = ctrlFrame->getMacNumTX();
			
			if(tmpValue < 0)
			{
				CASTALIA_DEBUG << "\n[Mac_" << self <<"]:\nWarning! Application module tried to set an invalid NumTX to the MAC module\n";
			}
			else
			{
				numTx = tmpValue;
			}
			
			ctrlFrame = NULL;
			
			break;
		}
		
		case APPLICATION_2_MAC_SETRNDTXOFFSET:
		{
			App_ControlMessage *ctrlFrame;
			ctrlFrame = check_and_cast<App_ControlMessage*>(msg);
			
			double tmpValue = ctrlFrame->getMacRndTXOffset()/1000.0;
			
			if(tmpValue <= 0.0)
			{
				CASTALIA_DEBUG << "\n[Mac_" << self <<"]:\nWarning! Application module tried to set an invalid randomTxOffset to the MAC module\n";
			}
			else
			{
				randomTxOffset = tmpValue;
			}
			
			ctrlFrame = NULL;
			
			break;
		}
		
		case APPLICATION_2_MAC_SETRETXINTERVAL:
		{
			App_ControlMessage *ctrlFrame;
			ctrlFrame = check_and_cast<App_ControlMessage*>(msg);
			
			double tmpValue = ctrlFrame->getMacReTXInterval()/1000.0;
			
			if(tmpValue <= 0.0)
			{
				CASTALIA_DEBUG << "\n[Mac_" << self <<"]:\nWarning! Application module tried to set an invalid reTxInterval to the MAC module\n";
			}
			else
			{
				reTxInterval = tmpValue;
			}
			
			ctrlFrame = NULL;
			
			break;
		}
		
		case APPLICATION_2_MAC_SETBACKOFFTYPE:
		{
			App_ControlMessage *ctrlFrame;
			ctrlFrame = check_and_cast<App_ControlMessage*>(msg);
			
			int tmpValue = ctrlFrame->getMacBackoffType();
			
			switch(tmpValue)
			{
				case 0:
						{
							if ((dutyCycle > 0.0) && (dutyCycle < 1.0))
								backoffType = BACKOFF_SLEEP_INT;
							else
								CASTALIA_DEBUG << "\n[Mac_" << self <<"]:\nWarning! Application module tried to set an invalid backoffType to the MAC module.\n Backoff timer = sleeping interval, but sleeping interval is not defined because duty cycle is zero, one, or invalid.\n";

							break;
						}
				
				case 1:
						{
							backoffType = BACKOFF_CONSTANT;
							if(backoffBaseValue <= 0.0)
								CASTALIA_DEBUG << "\n[Mac_" << self <<"]:\nWarning! Application sent a SETBACKOFFTYPE strobe to Mac module but Mac module is set with an invalid backoffBaseValue.\n";
							break;
						}
						
				case 2:
						{
							backoffType = BACKOFF_MULTIPLYING;
							if(backoffBaseValue <= 0.0)
								CASTALIA_DEBUG << "\n[Mac_" << self <<"]:\nWarning! Application sent a SETBACKOFFTYPE strobe to Mac module but Mac module is set with an invalid backoffBaseValue.\n";
							break;
						}
						
				case 3:
						{
							backoffType = BACKOFF_EXPONENTIAL;
							if(backoffBaseValue <= 0.0)
								CASTALIA_DEBUG << "\n[Mac_" << self <<"]:\nWarning! Application sent a SETBACKOFFTYPE strobe to Mac module but Mac module is set with an invalid backoffBaseValue.\n";
							break;
						}
						
				default:
				        {
				        	CASTALIA_DEBUG << "\n[Mac_" << self <<"]:\nWarning! Application module tried to set an invalid backoffType to the MAC module\n";
				        	break;
				        }
			}
			
			ctrlFrame = NULL;
			
			break;
		}
		
		
		case APPLICATION_2_MAC_SETBACKOFFBASEVALUE:
		{
			App_ControlMessage *ctrlFrame;
			ctrlFrame = check_and_cast<App_ControlMessage*>(msg);
			
			double tmpValue = ctrlFrame->getMacBackoffBaseValue();
			
			if(tmpValue > 0)
			{
				backoffBaseValue = tmpValue/1000.0; //convertion to sec
			}
			else
				CASTALIA_DEBUG << "\n[Mac_" << self <<"]:\nWarning! Application module tried to set an invalid backoffBaseValue to the MAC module\n";
				
			ctrlFrame = NULL;
			
			break;
		}
		
		
		case APPLICATION_2_MAC_SETCARRIERSENSE:
		{
			App_ControlMessage *ctrlFrame;
			ctrlFrame = check_and_cast<App_ControlMessage*>(msg);
			
			bool tmpValue = ctrlFrame->getMacUsingCarrierSense();
			
			// if carrier sense is not enabled and application turns it on
			// OR
			// if carrier sense is enabled and application turns it off
			if( (carrierSense && !tmpValue) || (!carrierSense && tmpValue) )
			{
				// Reset the backoff counter
				backoffTimes = 0;
			}
			
			carrierSense = tmpValue;
			
			
			ctrlFrame = NULL;
			
			break;
		}


		default:
		{
			CASTALIA_DEBUG << "\n[Mac_"<< self <<"] t= " << simTime() << ": WARNING: received packet of unknown type\n";
			
			break;
		}

	}//end_of_switch

	delete msg;
	msg = NULL;		// safeguard
}



void TunableMacModule::finish()
{
	MAC_GenericFrame *macMsg;

	while(!BUFFER_IS_EMPTY)
	{
		macMsg = popTxBuffer();

		cancelAndDelete(macMsg);

	  	macMsg = NULL;
	}
	delete[] schedTXBuffer;

	if(printPotentiallyDropped)
		CASTALIA_DEBUG << "\n[Mac" << self << "]  Dropped Packets at MAC module while receiving:\t (" << potentiallyDroppedBeacons << ") beacons   and   (" << potentiallyDroppedDataFrames << ") data frames";

}


void TunableMacModule::readIniFileParameters(void)
{
	printPotentiallyDropped = par("printPotentiallyDroppedPacketsStatistics");

	printDebugInfo = par("printDebugInfo");

	printStateTransitions = par("printStateTransitions");

	dutyCycle = par("dutyCycle");

	listenInterval = ((double)par("listenInterval"))/1000.0;				// just convert msecs to secs

	beaconIntervalFraction = par("beaconIntervalFraction");

	probTx = par("probTx");

	numTx = par("numTx");

	randomTxOffset = ((double)par("randomTxOffset"))/1000.0;		// just convert msecs to secs

	reTxInterval = ((double)par("reTxInterval"))/1000.0;			// just convert msecs to secs

	maxMACFrameSize = par("maxMACFrameSize");

	macFrameOverhead = par("macFrameOverhead");

	beaconFrameSize = par("beaconFrameSize");

	ACKFrameSize = par("ACKFrameSize");

	macBufferSize = par("macBufferSize");

	carrierSense = par("carrierSense");


	backoffType = par("backoffType");
	switch(backoffType)
	{
		case 0:{
			if ((dutyCycle > 0.0) && (dutyCycle < 1.0))
				backoffType = BACKOFF_SLEEP_INT;
			else
				CASTALIA_DEBUG << "\n[Mac]: Illegal value of parameter \"backoffType\" in omnetpp.ini.\n    Backoff timer = sleeping interval, but sleeping interval is not defined because duty cycle is zero, one, or invalid. Will use backOffBaseValue instead";
				backoffType = BACKOFF_CONSTANT;
			break;
			}
		case 1:{backoffType = BACKOFF_CONSTANT; break;}
		case 2:{backoffType = BACKOFF_MULTIPLYING; break;}
		case 3:{backoffType = BACKOFF_EXPONENTIAL; break;}
		default:{
			opp_error("\n[Mac]:\n Illegal value of parameter \"backoffType\" in omnetpp.ini.");
			break;}
	}

	backoffBaseValue = ((double)par("backoffBaseValue"))/1000.0;			// just convert msecs to secs

	randomBackoff = par("randomBackoff");
}




void TunableMacModule::setRadioState(MAC_ControlMessageType typeID, double delay)
{
	if( (typeID != MAC_2_RADIO_ENTER_SLEEP) && (typeID != MAC_2_RADIO_ENTER_LISTEN) && (typeID != MAC_2_RADIO_ENTER_TX) )
		opp_error("MAC attempt to set Radio into an unknown state. ERROR commandID");

	MAC_ControlMessage * ctrlMsg = new MAC_ControlMessage("state command strobe MAC->radio", typeID);

	sendDelayed(ctrlMsg, delay, "toRadioModule");
}




void TunableMacModule::setRadioTxMode(Radio_TxMode txTypeID, double delay)
{
	if( (txTypeID != CARRIER_SENSE_NONE)&&(txTypeID != CARRIER_SENSE_ONCE_CHECK)&&(txTypeID != CARRIER_SENSE_PERSISTENT) )
		opp_error("MAC attempt to set Radio CarrierSense into an unknown type. ERROR commandID");

	MAC_ControlMessage * ctrlMsg = new MAC_ControlMessage("Change Radio TX mode command strobe MAC->radio", MAC_2_RADIO_CHANGE_TX_MODE);
	ctrlMsg->setRadioTxMode(txTypeID);

	sendDelayed(ctrlMsg, delay, "toRadioModule");
}




void TunableMacModule::setRadioPowerLevel(int powLevel, double delay)
{
	if( (powLevel >= 0) && (powLevel < radioModule->getTotalTxPowerLevels()) )
	{
		MAC_ControlMessage * ctrlMsg = new MAC_ControlMessage("Set power level command strobe MAC->radio", MAC_2_RADIO_CHANGE_POWER_LEVEL);
		ctrlMsg->setPowerLevel(powLevel);
		sendDelayed(ctrlMsg, delay, "toRadioModule");
	}
	else
		CASTALIA_DEBUG << "\n[Mac_" << self << "] t= " << simTime() << ": WARNING: in function setRadioPowerLevel() of Mac module, parameter powLevel has invalid value.\n";
}


void TunableMacModule::createBeaconDataFrame(MAC_GenericFrame *retFrame)
{
	retFrame->getHeader().srcID = self;
	retFrame->getHeader().destID = MAC_BROADCAST_ADDR;
	retFrame->getHeader().frameType = (unsigned short)MAC_PROTO_BEACON_FRAME;

	//TODO: Add code here to fill the macHeaderInfo of the MAC data frame
	//inside it we can set the dest address to be unicast or broadcast
	  //retFrame->getHeader().srcMacAddr = {'0', '0', '0', '0'};
	  //retFrame->getHeader().destMacAddr = {'0', '0', '0', '0'};

	retFrame->setByteLength(beaconFrameSize);
}


int TunableMacModule::isBeacon(MAC_GenericFrame *theFrame)
{
	return(((int)theFrame->getHeader().frameType) == MAC_PROTO_BEACON_FRAME);
}


int TunableMacModule::encapsulateNetworkFrame(Network_GenericFrame *networkFrame, MAC_GenericFrame *retFrame)
{
	int totalMsgLen = networkFrame->byteLength() + macFrameOverhead;
	if(totalMsgLen > maxMACFrameSize)
		return 0;
	retFrame->setByteLength(macFrameOverhead); //networkFrame->byteLength() extra bytes will be added after the encapsulation

	retFrame->getHeader().srcID = self;
	
	retFrame->getHeader().destID = deliver2NextHop(networkFrame->getHeader().nextHop.c_str());
	
	retFrame->getHeader().frameType = (unsigned short)MAC_PROTO_DATA_FRAME;

	Network_GenericFrame *dupNetworkFrame = check_and_cast<Network_GenericFrame *>(networkFrame->dup());
	retFrame->encapsulate(dupNetworkFrame);

	return 1;
}



int TunableMacModule::pushBuffer(MAC_GenericFrame *theFrame)
{
	if(theFrame == NULL)
	{
		CASTALIA_DEBUG << "\n[Mac_" << self << "] t= " << simTime() << ": WARNING: Trying to push NULL MAC_GenericFrame to the MAC_Buffer!!\n";
		return 0;
	}

	tailTxBuffer = (++tailTxBuffer)%(MAC_BUFFER_ARRAY_SIZE); //increment the tailTxBuffer pointer. If reached the end of array, then start from position [0] of the array

	if (tailTxBuffer == headTxBuffer)
	{
		// reset tail pointer
		if(tailTxBuffer == 0)
			tailTxBuffer = MAC_BUFFER_ARRAY_SIZE-1;
		else
			tailTxBuffer--;
		CASTALIA_DEBUG << "\n[Mac_" << self << "] t= " << simTime() << ": WARNING: SchedTxBuffer FULL!!! value to be Tx not added to buffer\n";
		return 0;
	}

	theFrame->setKind(MAC_FRAME);

	if (tailTxBuffer==0)
		schedTXBuffer[MAC_BUFFER_ARRAY_SIZE-1] = theFrame;
	else
		schedTXBuffer[tailTxBuffer-1] = theFrame;


	int currLen = getTXBufferSize();
	if (currLen > maxSchedTXBufferSizeRecorded)
		maxSchedTXBufferSizeRecorded = currLen;

	return 1;
}


MAC_GenericFrame* TunableMacModule::popTxBuffer()
{

	if (tailTxBuffer == headTxBuffer) {
		ev << "\nTrying to pop  EMPTY TxBuffer!!";
		tailTxBuffer--;  // reset tail pointer
		return NULL;
	}

	MAC_GenericFrame *pop_message = schedTXBuffer[headTxBuffer];

	headTxBuffer = (++headTxBuffer)%(MAC_BUFFER_ARRAY_SIZE);

	return pop_message;
}



int TunableMacModule::getTXBufferSize(void)
{
	int size = tailTxBuffer - headTxBuffer;
	if ( size < 0 )
		size += MAC_BUFFER_ARRAY_SIZE;

	return size;
}



int TunableMacModule::deliver2NextHop(const char *nextHop)
{
	string strNextHop(nextHop);
	
	// Resolv Network-layer IDs to MAC-layer IDs
	if(strNextHop.compare(BROADCAST) == 0)
	{
		return MAC_BROADCAST_ADDR;
	}
	else
	{
		return atoi(strNextHop.c_str());
	}
	
	// moreover if strNextHop is a list of delimited nodeIDs 
	// then again BROADCAST
}
