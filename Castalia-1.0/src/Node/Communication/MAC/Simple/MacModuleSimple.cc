/************************************************************************************
 *  Copyright: National ICT Australia,  2006										*
 *  Developed at the Networks and Pervasive Computing program						*
 *  Author(s): Athanassios Boulis, Dimosthenis Pediaditakis							*
 *  This file is distributed under the terms in the attached LICENSE file.			*
 *  If you do not find this file, copies can be found by writing to:				*
 *																					*
 *      NICTA, Locked Bag 9013, Alexandria, NSW 1435, Australia						*
 *      Attention:  License Inquiry.												*
 *																					*
 ************************************************************************************/


#include "MacModuleSimple.h"

#define MAC_BUFFER_ARRAY_SIZE macBufferSize+1

#define BUFFER_IS_EMPTY  (headTxBuffer==tailTxBuffer)

#define BUFFER_IS_FULL  (getTXBufferSize() >= macBufferSize)

#define CARRIER_SENSE_INTERVAL 0.0001

#define DRIFTED_TIME(time) ((time) * cpuClockDrift)

#define EV   ev.disabled() ? (ostream&)ev : ev



Define_Module(MacModuleSimple);



void MacModuleSimple::initialize() 
{
	
	readIniFileParameters();
	
	//--------------------------------------------------------------------------------
	//------- Follows code for the initialization of the class member variables ------
	
	self = parentModule()->parentModule()->index();
	
	//get a valid reference to the object of the Radio module so that we can make direct calls to its public methods
	//instead of using extra messages & message types for tighlty couplped operations. 
	radioModule = check_and_cast<RadioModule*>(gate("toRadioModule")->toGate()->ownerModule());
	radioDataRate = radioModule->par("dataRate");
	radioDelayForValidCS = ((double) radioModule->par("delayCSValid"))/1000; //parameter given in ms in the omnetpp.ini
	
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
	setRadioState(MAC_2_RADIO_ENTER_SLEEP);
	if ((dutyCycle > 0) && (dutyCycle < 1))
		sleepInterval = listenInterval * (dutyCycle/(1-dutyCycle));
	else 
		sleepInterval = -1;
	
	macState = MAC_STATE_DEFAULT;
	EV << "\n[Mac_" << self <<"]:\n _1_ State changed to MAC_STATE_DEFAULT";
	
	rxOutMessage = NULL;
	
	selfExitCSMsg = NULL;
	
	schedTXBuffer = new MAC_GenericFrame*[MAC_BUFFER_ARRAY_SIZE];
	headTxBuffer = 0;
	tailTxBuffer = 0;
	
	maxSchedTXBufferSizeRecorded = 0;
	
	epsilon = 0.000001;
	
	disabled = 1;
	
	packetID = 0;
	
	timesBlockTX = 0;
}


void MacModuleSimple::handleMessage(cMessage *msg)
{	
	/*
	   //debug info
	   EV << "\n[Mac]:\n Message received: " << msg->name() << ", at Node: " << self << ", at time: " << simTime();
	 */
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
		 * Sent by the Application submodule in order to start/switch-on the MAC submodule.
		 *--------------------------------------------------------------------------------------------------------------*/
		case APP_NODE_STARTUP:
		{
			disabled = 0;
			
			setRadioState(MAC_2_RADIO_ENTER_LISTEN);
			
			if ((dutyCycle > 0) && (dutyCycle < 1))
				scheduleAt(simTime() + DRIFTED_TIME(listenInterval), new MAC_ControlMessage("put_radio_to_sleep", MAC_SELF_SET_RADIO_SLEEP));

			break;
		}
		
		
		/*--------------------------------------------------------------------------------------------------------------
		 * Sent by the Application submodule. We need to push it into the buffer and schedule its forwarding to the Radio buffer for TX.
		 *--------------------------------------------------------------------------------------------------------------*/
		case APP_DATA_PACKET:
		{
			if(!BUFFER_IS_FULL)
			{
				App_GenericDataPacket *rcvAppDataPacket = check_and_cast<App_GenericDataPacket*>(msg);
				//create the MACFrame from the Application Data Packet (encapsulation)
				MAC_GenericFrame *dataFrame;
				
				for( int i = 0; i< numTx; i++ )	// place at the TX buffer as many copies of the message as allowed by the parameter numTx 
				{		
					if (genk_dblrand(1) < probTx )  // for each try check with probTx, notice that we are using generator 1 not 0 
					{	
							// get the random offset for this try
							double random_offset = genk_dblrand(0) * randomTxOffset;	
							
							// create a new copy of the dataFrame
							char buff[50];
							sprintf(buff, "MAC Data frame (%f)", simTime());
							dataFrame = new MAC_GenericFrame(buff, MAC_FRAME);
							encapsulateAppPacket(rcvAppDataPacket, dataFrame);
							dataFrame->setKind(MAC_FRAME_SELF_PUSH_TX_BUFFER); //the message MUST be pointer of type MAC_GenericFrame*
							
							scheduleAt(simTime() + DRIFTED_TIME(random_offset + i*reTxInterval), dataFrame);
							
							// schedule a new TX 
							scheduleAt(simTime() + DRIFTED_TIME(random_offset + i*reTxInterval) + epsilon, new MAC_ControlMessage("initiate a TX", MAC_SELF_INITIATE_TX));
					}
				}
				
				rcvAppDataPacket = NULL;
				dataFrame = NULL;
			}
			else
				EV << "\n[Mac_" << self <<"]:\n WARNING: SchedTxBuffer FULL!!! Application data packet is discarded";
			
			break;
		}//END_OF_CASE(APP_DATA_PACKET)
		
		
		
		
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

					
					if(macState == MAC_STATE_DEFAULT)
					{	
						macState = 	MAC_STATE_TX;
						EV << "\n[Mac_" << self <<"]:\n _2_  State changed to MAC_STATE_TX";
					}
					
					
					
					// SEND THE BEACON FRAMES TO RADIO BUFFER
					double beacon_offset = 0;
					/* First, (if there is a duty cycle) we must "wake up" potential receivers
					 * by sending a train of beacons. The packet we have now is fixed so we will
					 * send so many packets to cover the (sleeping interval) * beaconIntervalFraction
					 * double beacon_offset = 0;
					 */
					if ((dutyCycle > 0) && (dutyCycle < 1)) 
					{	
						// we do not send the beacons back to back, we leave listenInterval/2 between them
						double beaconTxTime = beaconFrameSize * 8 / (1000 * radioDataRate);
						double step = beaconTxTime + listenInterval/2;
						
						int i;
						
						char beaconDescr[30];
						for( i = 1; (i-1)*step < sleepInterval * beaconIntervalFraction; i++)
						{
							sprintf(beaconDescr, "Mac_beacon_Frame_%d", packetID);
							MAC_GenericFrame *beaconFrame = new MAC_GenericFrame(beaconDescr, MAC_FRAME);
							createBeaconDataFrame(beaconFrame);
							
							// send one beacon delayed by the proper amount of time
							sendDelayed(beaconFrame, DRIFTED_TIME((i-1)*step), "toRadioModule");//send beaconFrame to the buffer of the Radio with delay (i-1)*step
							
							setRadioState(MAC_2_RADIO_ENTER_TX, DRIFTED_TIME((i-1)*step)+epsilon);
							beacon_offset = DRIFTED_TIME((i-1)*step);
							packetID++;
						}
						
					}
					
					
					
					// SEND THE DATA FRAME TO RADIO BUFFER
					MAC_GenericFrame *dataFrame = popTxBuffer();
					
					sendDelayed(dataFrame, beacon_offset, "toRadioModule");
					setRadioState(MAC_2_RADIO_ENTER_TX, beacon_offset+epsilon);
					
					double dataTXtime = dataFrame->getFrameLength() * 8 / (1000 * radioDataRate);
					//schedule a self-msg to update the node's state later
					scheduleAt(simTime()+ beacon_offset + DRIFTED_TIME(dataTXtime), new MAC_ControlMessage("check schedTXBuffer buffer", MAC_SELF_CHECK_TX_BUFFER));
					
					/*
				 	 * TODO:
				 	 * IF we need to request Carrier Sense for each packet in the buffer then replace the previous line with the following three:
				 	 * macState = MAC_STATE_DEFAULT;
				 	 * //check if empty buffer and do TX (with or without Carrier Sense )
				 	 * scheduleAt(simTime() + DRIFTED_TIME(beacon_offset+dataTXtime), new MAC_ControlMessage("initiate a TX", MAC_SELF_INITIATE_TX));
				 	 */
				 	 
				 	 dataFrame = NULL;
			 	 }
			 }
			 else //MAC buffer is empty
			 {
			 	macState = MAC_STATE_DEFAULT;
			 	EV << "\n[Mac_" << self <<"]:\n _3a_ State changed to MAC_STATE_DEFAULT";
			 	//it is not actually needed because the radio enters automatically into listen mode when the radioBuffer is empty
			 	//setRadioState(MAC_2_RADIO_ENTER_LISTEN);
				
				if ((dutyCycle > 0) && (dutyCycle < 1)) 
					scheduleAt(simTime() + DRIFTED_TIME(listenInterval), new MAC_ControlMessage("put_radio_to_sleep", MAC_SELF_SET_RADIO_SLEEP));
			 }

			break;
		}
		
		
		
		/*--------------------------------------------------------------------------------------------------------------
		 * Control message sent by the MAC (ourself) to try to set the state of the Radio to RADIO_STATE_SLEEP.
		 *--------------------------------------------------------------------------------------------------------------*/
		case MAC_SELF_SET_RADIO_SLEEP:
		{	
			if(macState == MAC_STATE_DEFAULT) //not in MAC_STATE_TX   OR   MAC_STATE_CARRIER_SENSING    OR    MAC_STATE_EXPECTING_RX
			{
				setRadioState(MAC_2_RADIO_ENTER_SLEEP);
				
				scheduleAt(simTime() + DRIFTED_TIME(sleepInterval), new MAC_ControlMessage("wake_up_radio", MAC_SELF_WAKEUP_RADIO));
			}
			
			break;
		}
		
		
		
		/*--------------------------------------------------------------------------------------------------------------
		 * Control message sent by the MAC (ourself) to try to wake up the radio.
		 *--------------------------------------------------------------------------------------------------------------*/
		case MAC_SELF_WAKEUP_RADIO:
		{	
			if(macState == MAC_STATE_DEFAULT) //not in MAC_STATE_TX   OR   MAC_STATE_CARRIER_SENSING    OR    MAC_STATE_EXPECTING_RX
			{
				setRadioState(MAC_2_RADIO_ENTER_LISTEN);
				
				//scheduleAt(simTime() + DRIFTED_TIME(radioDelayForValidCS), new MAC_ControlMessage("initiate a TX", MAC_SELF_INITIATE_TX));
			
				scheduleAt(simTime() + DRIFTED_TIME(listenInterval), new MAC_ControlMessage("put_radio_to_sleep", MAC_SELF_SET_RADIO_SLEEP));
			}
			break;
		}
		
		
		
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
						scheduleAt(simTime(), new MAC_ControlMessage("check schedTXBuffer buffer", MAC_SELF_CHECK_TX_BUFFER));
					}
				 }
				 else // the Mac buffer is empty
				 {	
				 	EV << "\n[Mac_" << self << "]:\n Received message type MAC_SELF_INITIATE_TX but Mac Buffer is empty.";
				 	
				 	macState = MAC_STATE_DEFAULT;
				 	EV << "\n[Mac_" << self <<"]:\n _3b_ State changed to MAC_STATE_DEFAULT";
				 	
					if ((dutyCycle > 0) && (dutyCycle < 1)) 
						scheduleAt(simTime() + DRIFTED_TIME(listenInterval), new MAC_ControlMessage("put_radio_to_sleep", MAC_SELF_SET_RADIO_SLEEP));
				 }
			}
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
				EV << "\n[Mac]:\n WARNING: SchedTxBuffer FULL!!! Application data packet is discarded";
			
			break;
		}
		
		
		
		/*--------------------------------------------------------------------------------------------------------------
		 * Data Frame Received from the Radio submodule (the data frame can be a Data packet or a beacon packet)
		 *--------------------------------------------------------------------------------------------------------------*/
		case MAC_FRAME:
		{
			/*
			 * TODO: what if MAC is in TX mode and receives a MAC frame from the radio module?
			 * Currently we discard it. 
			 */
			if( (macState == MAC_STATE_DEFAULT) || (macState == MAC_STATE_EXPECTING_RX) || (macState == MAC_STATE_CARRIER_SENSING) )   
			{
				MAC_GenericFrame *rcvFrame;
				rcvFrame = check_and_cast<MAC_GenericFrame*>(msg);
				
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
						EV << "\n[Mac_" << self <<"]:\n _4_ State changed to MAC_STATE_EXPECTING_RX";
						
						cancelAndDelete(rxOutMessage);  //just in case
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
									// just create a new message with a different description and  schedule it
									if(macState == MAC_STATE_DEFAULT)
									{	
										macState = MAC_STATE_EXPECTING_RX;
										EV << "\n[Mac_" << self <<"]:\n _5_ State changed to MAC_STATE_EXPECTING_RX";
									}
									rxOutMessage = new cMessage("get out of RADIO_RX, data received", MAC_SELF_EXIT_EXPECTING_RX);
									scheduleAt(simTime(), rxOutMessage);
						}
					}
						
						
					//DELIVER THE DATA PACKET TO APPLICATION
					App_GenericDataPacket *appDataPacket; 		  //passed as argument in decapsulateMacFrame() to hold the returned application packet
					appDataPacket = new App_GenericDataPacket("Application Packet MAC->Application", APP_DATA_PACKET);
					decapsulateMacFrame(rcvFrame, appDataPacket); //decaspulate the received MAC frame and create a valid App_GenericDataPacket
					
					//send the App_GenericDataPacket message to the Application module
					send(appDataPacket, "toCommunicationModule");
					
				}
			}
			break;
		}
		
		
		
		/*--------------------------------------------------------------------------------------------------------------
		 * Received by ourselves. It was sent DRIFTED_TIME(sleepInterval) before, when we received the first beacon, so that we
		 * won't stay trapped in the MAC_STATE_EXPECTING_RX waiting for the Data Frame to come.
		 *--------------------------------------------------------------------------------------------------------------*/
		case MAC_SELF_EXIT_EXPECTING_RX:
		{
			if (macState != MAC_STATE_EXPECTING_RX)
			{
				EV << "\n[Mac]:\n WARNING: received MAC_SELF_EXIT_EXPECTING_RX message while not in MAC_STATE_EXPECTING_RX state. Node: " << self << "  at time: " << simTime() << ", message description: " << msg->name();
			}
			else
			{
				macState = MAC_STATE_DEFAULT;
				EV << "\n[Mac_" << self <<"]:\n _6_ State changed to MAC_STATE_DEFAULT";
				setRadioState(MAC_2_RADIO_ENTER_LISTEN); //radio should be already in LISTEN mode. I use this line just in case 
				
				//check if empty buffer and do TX (with or without Carrier Sense )
				scheduleAt(simTime(), new MAC_ControlMessage("initiate a TX", MAC_SELF_INITIATE_TX)); 

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
					
					MAC_ControlMessage *selfExitCSMsg;
					selfExitCSMsg = new MAC_ControlMessage("Exit carrier sense state MAC->MAC", MAC_SELF_EXIT_CARRIER_SENSE);
					scheduleAt(simTime() + CARRIER_SENSE_INTERVAL + epsilon, selfExitCSMsg);
					
					macState = MAC_STATE_CARRIER_SENSING;
					EV << "\n[Mac_" << self <<"]:\n _7_ State changed to MAC_STATE_CARRIER_SENSING";
				}
				else //carrier sense indication of Radio is NOT Valid and reasonNonValid holds the cause for the non valid carrier sense indication
				{
					switch(isCarrierSenseValid_ReturnCode)
					{
						case RADIO_IN_TX_MODE:
						{
							/*TODO: alternatively we may send the packet without beacons and without sending the MAC_SELF_CHECK_TX_BUFFER self-message DIRECTLY
							 *      to the RadioBuffer WITHOUT ADDING BEACONS as follows:
							 * 
							 * if(!(BUFFER_IS_EMPTY))
							 * {
							 * 		macState = MAC_STATE_TX;
							 * 		MAC_GenericFrame *dataFrame = check_and_cast<MAC_GenericFrame*>(schedTXBuffer->pop());
							 * 		if(dataFrame != NULL)
							 * 		{
							 * 			sendPacketToRadio(dataFrame);
							 * 			setRadioState(MAC_2_RADIO_ENTER_TX, epsilon);
							 * 			double dataTXtime = dataFrame->getFrameLength() * 8 / (1000 * radioDataRate);
							 * 			scheduleAt(simTime()+dataTXtime, new MAC_ControlMessage("check schedTXBuffer buffer", MAC_SELF_CHECK_TX_BUFFER));
							 * 		}
							 * }
							 * 
							 * 
							 *MOREOVER IF we need to request Carrier Sense for each packet in the buffer then replace the following line with the following three:
							 * 
							 * macState = MAC_STATE_DEFAULT;
							 * //check if empty buffer and do TX (with or without Carrier Sense )
							 * scheduleAt(simTime() + DRIFTED_TIME(beacon_offset+dataTXtime), new MAC_ControlMessage("initiate a TX", MAC_SELF_INITIATE_TX));
							 */
							 
							//send the packet (+ the precending beacons) to the Radio Buffer  by sending a message to ourselves
							scheduleAt(simTime(), new MAC_ControlMessage("check schedTXBuffer buffer", MAC_SELF_CHECK_TX_BUFFER));
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
							EV << "\n[Mac]:\n WARNING: In MAC module, radioModule->isCarrierSenseValid(reasonNonValid) return invalid reasonNonValid";
							break;
						}
					}//end_switch
				}
			}
			/*TODO: DELETEME  ???????????
			else  //MAC Module is not in the MAC_STATE_DEFAULT
			{
				if(macState == MAC_STATE_TX)
				{
					// schedule a message that will check the Tx buffer for transmission NOW
					scheduleAt(simTime(), new MAC_ControlMessage("check schedTXBuffer buffer", MAC_SELF_CHECK_TX_BUFFER));
				}
				else
				if(macState == MAC_STATE_CARRIER_SENSING)
				{
					double random_offset = genk_dblrand(0) * randomTxOffset;
					//delayed carrier sense request (delay = random_offset + i * reTxInterval + epsilon)
					scheduleAt(simTime()+random_offset + reTxInterval + epsilon, new MAC_ControlMessage("Enter carrier sense state MAC->MAC", MAC_SELF_PERFORM_CARRIER_SENSE););
				}
				else //macState == MAC_STATE_EXPECTING_RX)
				{
					//do nothing --> the packet that we pushed in the MAC TX buffer will be forwarded to radio in the next 
					//change to state MAC_STATE_TX (in wich we send all the conents of the buffer)
				}
			}*/
			
			break;
		}
		
		
		
		/*--------------------------------------------------------------------------------------------------------------
		 * Sent by the MAC submodule (ourself) to exit the MAC_STATE_CARRIER_SENSING (carrier is free)
		 *--------------------------------------------------------------------------------------------------------------*/
		case MAC_SELF_EXIT_CARRIER_SENSE:
		{
			if(macState == MAC_STATE_CARRIER_SENSING)
			{
				macState = MAC_STATE_TX;
				// schedule a message that will check the Tx buffer for transmission
				scheduleAt(simTime(), new MAC_ControlMessage("check schedTXBuffer buffer", MAC_SELF_CHECK_TX_BUFFER));
			}
			
			selfExitCSMsg = NULL;
			
			break;
		}
		

		/*--------------------------------------------------------------------------------------------------------------
		 * Sent by the Radio submodule as a response for our (MAC's) previously sent directive to perform CS (carrier is not clear)
		 *--------------------------------------------------------------------------------------------------------------*/
		case RADIO_2_MAC_SENSED_CARRIER:
		{
			/*
			 * TODO:  SOSOSOSOS  !!! ADD RANDOM BACKOFF (e.g. random exponential) AND DON'T USE always the sleepInterval
			 */
			if(macState == MAC_STATE_CARRIER_SENSING)
			{
				macState = MAC_STATE_DEFAULT;
				EV << "\n[Mac_" << self <<"]:\n _8_ State changed to MAC_STATE_DEFAULT";
				
				// Go to sleep. When it wakes up it automatically 'll check again the buffer and initiate a new Carrier Sense + TX
				scheduleAt(simTime()+ DRIFTED_TIME(listenInterval), new MAC_ControlMessage("put_radio_to_sleep", MAC_SELF_SET_RADIO_SLEEP));
				
				//the carrier is busy so the pending(if there is one) MAC_SELF_EXIT_CARRIER_SENSE message is useless(and erroneous).Delete it
				if (( selfExitCSMsg != NULL ) && (  selfExitCSMsg->isScheduled() ))	
				{	
					cancelAndDelete(selfExitCSMsg);
					selfExitCSMsg = NULL;
				}
			}
			break;
		}
		
		
		
		case RADIO_2_MAC_FULL_BUFFER:
		{
			/* 
			 * TODO: add your code here to manage the situation of a full Radio buffer.
			 * Apparently we 'll have to stop sending messages and enter into listen or sleep mode (depending on the MAC protocol that we implement).
			 */
			 EV << "\nMac module received RADIO_2_MAC_FULL_BUFFER because the Radio buffer is full.\n";
			 break;
		}
		
		
		
		/*--------------------------------------------------------------------------------------------------------------
		 * 
		 *--------------------------------------------------------------------------------------------------------------*/
		case RESOURCE_MGR_OUT_OF_ENERGY:
		{
			disabled = 1;
			break;
		}
		
		
		default:
		{
			EV << "\\n[Mac]:\n WARNING: received packet of unknown type";
			break;
		}
		
	}//end_of_switch
	
	delete msg;
	msg = NULL;		// safeguard
}



void MacModuleSimple::finish()
{
	MAC_GenericFrame *msg;
	while(!BUFFER_IS_EMPTY)
	{	
		msg = popTxBuffer();//check_and_cast<MAC_GenericFrame*>(schedTXBuffer.pop_back());
		EV << "\n--Value(Mac Cleanup): " << msg->getData() << "\n";
		cancelAndDelete(msg);
	  	msg = NULL;
	}
	delete[] schedTXBuffer;
	recordScalar("Max Mac size recorded", maxSchedTXBufferSizeRecorded);
	recordScalar("Times Blocked", timesBlockTX);
	//schedTXBuffer.clear();
	//delete schedTXBuffer;
}


void MacModuleSimple::readIniFileParameters(void)
{
	dutyCycle = par("dutyCycle");
	
	listenInterval = ((double)par("listenInterval"))/1000;				// just convert msecs to secs
	
	beaconIntervalFraction = par("beaconIntervalFraction");
	
	probTx = par("probTx");
	
	numTx = par("numTx");
	
	randomTxOffset = ((double)par("randomTxOffset"))/1000;		// just convert msecs to secs
	
	reTxInterval = ((double)par("reTxInterval"))/1000;			// just convert msecs to secs
	
	maxMACFrameSize = par("maxMACFrameSize");
	
	macFrameOverhead = par("macFrameOverhead");
	
	beaconFrameSize = par("beaconFrameSize");
	
	ACKFrameSize = par("ACKFrameSize");
	
	macBufferSize = par("macBufferSize");
	
	carrierSense = par("carrierSense");
}




void MacModuleSimple::setRadioState(MAC_ContorlMessageType typeID, double delay)
{
	if( (typeID != MAC_2_RADIO_ENTER_SLEEP) && (typeID != MAC_2_RADIO_ENTER_LISTEN) && (typeID != MAC_2_RADIO_ENTER_TX) )
		opp_error("MAC attempt to set Radio into an unknown state. ERROR commandID");
	
	MAC_ControlMessage * ctrlMsg = new MAC_ControlMessage("state command strobe MAC->radio", typeID);	
	
	sendDelayed(ctrlMsg, delay, "toRadioModule");
}




void MacModuleSimple::setRadioTxMode(Radio_TxMode txTypeID, double delay)
{
	if( (txTypeID != CARRIER_SENSE_NONE)&&(txTypeID != CARRIER_SENSE_ONCE_CHECK)&&(txTypeID != CARRIER_SENSE_PERSISTENT) )
		opp_error("MAC attempt to set Radio CarrierSense into an unknown type. ERROR commandID");
	
	MAC_ControlMessage * ctrlMsg = new MAC_ControlMessage("Change Radio TX mode command strobe MAC->radio", MAC_2_RADIO_CHANGE_TX_MODE);
	ctrlMsg->setRadioTxMode(txTypeID);
	
	sendDelayed(ctrlMsg, delay, "toRadioModule");
}




void MacModuleSimple::setRadioPowerLevel(int powLevel, double delay)
{	
	if( (powLevel >= 0) && (powLevel < radioModule->getTotalTxPowerLevels()) )
	{
		MAC_ControlMessage * ctrlMsg = new MAC_ControlMessage("Set power level command strobe MAC->radio", MAC_2_RADIO_CHANGE_POWER_LEVEL);
		ctrlMsg->setPowerLevel(powLevel);
		sendDelayed(ctrlMsg, delay, "toRadioModule");
	}
	else
		EV << "\n[Mac]:\n WARNING: in function setRadioPowerLevel() of Mac module, parameter powLevel has invalid value.";
}


void MacModuleSimple::createBeaconDataFrame(MAC_GenericFrame *retFrame)
{
	retFrame->setData(-1);//-1 data for beacons
	retFrame->setSrcID(self);
	//retFrame->setDestID(????);
	//TODO:ADD code here to fill the macHeaderInfo of the MAC data frame
	//inside it we can set the dest address to be unicast or broadcast
	retFrame->getHeader().frameType = MAC_PROTO_BEACON_FRAME;
	retFrame->setFrameLength(beaconFrameSize);
}


int MacModuleSimple::isBeacon(MAC_GenericFrame *theFrame)
{
	return(theFrame->getHeader().frameType == MAC_PROTO_BEACON_FRAME);
}


void MacModuleSimple::encapsulateAppPacket(App_GenericDataPacket *appPacket, MAC_GenericFrame *retFrame)
{
	retFrame->setSrcID(self);
	retFrame->setDestID(appPacket->getDestID());
	//TODO:ADD code here to fill the macHeaderInfo of the MAC data frame
	//inside it we can set the dest address to be unicast or broadcast
	retFrame->getHeader().frameType = MAC_PROTO_DATA_FRAME;
	retFrame->setFrameLength(appPacket->getFrameLength()+macFrameOverhead);
	retFrame->setData(appPacket->getData());
}



void MacModuleSimple::decapsulateMacFrame(MAC_GenericFrame *receivedMsg, App_GenericDataPacket *appPacket)
{
	appPacket->setSrcID(receivedMsg->getSrcID());
	appPacket->setDestID(self); //???? maybe we have to keep the destID of the receivedMsg
	appPacket->setFrameLength(receivedMsg->getFrameLength()-macFrameOverhead);
	appPacket->setData(receivedMsg->getData());
}


int MacModuleSimple::pushBuffer(MAC_GenericFrame *theFrame)
{
	if(theFrame == NULL)
	{
		EV << "\n[Mac]:\n WARNING: Trying to push  NULL MAC_GenericFrame to the MAC_Buffer!!";
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
		EV << "\n[Mac]:\n WARNING: SchedTxBuffer FULL!!! value to be Tx not added to buffer";
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


MAC_GenericFrame* MacModuleSimple::popTxBuffer()
{

	if (tailTxBuffer == headTxBuffer) {
		ev << "\nTrying to pop  EMPTY TxBuffer!!";
		tailTxBuffer--;  // reset tail pointer
		return NULL;
	}

	MAC_GenericFrame *pop_message = schedTXBuffer[headTxBuffer];
	
	/*headTxBuffer++;
	if (headTxBuffer == MAC_BUFFER_ARRAY_SIZE) 
		headTxBuffer = 0;*/
	headTxBuffer = (++headTxBuffer)%(MAC_BUFFER_ARRAY_SIZE);
	
	return pop_message;
}

 
int MacModuleSimple::getTXBufferSize(void)
{
	int size = tailTxBuffer - headTxBuffer;
	if ( size < 0 )
		size += MAC_BUFFER_ARRAY_SIZE;
	
	return size;
}
