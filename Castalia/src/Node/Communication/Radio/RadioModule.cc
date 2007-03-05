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
 
 

#include "RadioModule.h"

#define RADIO_BUFFER_ARRAY_SIZE bufferSize+1

#define BUFFER_IS_EMPTY  (headTxBuffer==tailTxBuffer)

#define BUFFER_IS_FULL  (getRadioBufferSize() >= bufferSize)

#define EV   ev.disabled() ? (ostream&)ev : ev

#define CONSUME_ENERGY(a) resMgrModule->consumeEnergy(a)



Define_Module(RadioModule);



void RadioModule::initialize() {
	
	readIniFileParameters();
	
	self = parentModule()->parentModule()->index();
	
	radioState = RADIO_STATE_LISTEN;
	EV << "\n[Radio_" << self <<"]: _1_ State changed to RADIO_STATE_LISTEN";
	startListeningTime = simTime();
	isCSValid = 0;
	scheduleAt(simTime()+delayCSValid, new Radio_ControlMessage("Radio self set carrier sense indication as valid NOW", RADIO_SELF_VALIDATE_CS));
	
	//radioBuffer = new cQueue("radioBuffer");
	radioBuffer = new MAC_GenericFrame*[RADIO_BUFFER_ARRAY_SIZE];
	headTxBuffer = 0;
	tailTxBuffer = 0;
	
	txPowerLevels.clear();
	const char *tmpLevelsStr, *token;
	tmpLevelsStr =  par("txPowerLevels");
	cStringTokenizer pwrLevelsTokenizer(tmpLevelsStr);
	while ( (token = pwrLevelsTokenizer.nextToken()) != NULL )
    	txPowerLevels.push_back((double)atof(token));
	
	if(txPowerConsumptionPerLevel.size() != txPowerLevels.size())
		opp_error("\n[Radio]:\n Initialization error in RadioModule: parameters txPowerLevels=%d (of Radio) and txPowerLevels=%d (of WChannel) have not the same number of elements.", txPowerLevels.size(), txPowerConsumptionPerLevel.size());
	
	maxBufferSizeRecorded = 0;
	
	startSleepingTime = 0;
	
	//get a valid reference to the object of the Resources Manager module so that we can make direct calls to its public methods
	//instead of using extra messages & message types for tighlty couplped operations.
	cModule *parentParent = parentModule()->parentModule();
	if(parentParent->findSubmodule("nodeResourceMgr") != -1)
	{	
		resMgrModule = check_and_cast<ResourceGenericManager*>(parentParent->submodule("nodeResourceMgr"));
	}
	else
		opp_error("\n[Radio]:\n Error in getting a valid reference to  nodeResourceMgr for direct method calls.");
	
	
	changingState = 0;
	
	nextState = -1;
	//framesToNextState = -1;			//the number of frames that have to be transmitted before the radio state changes.
	
	nextTxMode = -1;
	//framesToNextTxMode = -1;
	
	nextPowerLevel = -1;
	//framesToNextPowerLevel = -1;
	
	disabled = 0;
	
	valuesVector.setName("Radio Vector");
	
	timesBlockTX = 0;
	timesBlockTX2 = 0;
	
	totalTimeTX = 0;
	totalTimeListening = 0;
	totalTimeRX = 0;
	totalTimeSleeping = 0;
	totalTimeTransitions = 0;
	
	beginT = simTime();
	
	droppedDataFrames = 0;
	droppedBeacons = 0;
}



void RadioModule::handleMessage(cMessage *msg)
{	
	/*
	   //debug info
	   EV << "\n[Radio]:\n Message received: " << msg->name() << ", at Node: " << self << ", at time: " << simTime();
	 */
	if(disabled)
	{
		delete msg;
		msg = NULL;		// safeguard	
		return;
	}
	
	switch (msg->kind()) 
	{
		/*--------------------------------------------------------------------------------------------------------------
		 * Message received from the MAC module. The Radio module has to enter in SLEEP state
		 *--------------------------------------------------------------------------------------------------------------*/
		case MAC_2_RADIO_ENTER_SLEEP:
		{
			if(!changingState)
			{
				if(radioState == RADIO_STATE_TX)
				{
					/*
					 * There is no need to send self message to manage state transition etc.
					 * Those things are performed by the RADIO_SELF_CHECK_BUFFER when the buffer gets empty
					 * That means the radio follows this strategy: 
					 * 		keep sending until your buffer gets empty and then go to the state that "nextState" indicates.
					 */
					 nextState = RADIO_STATE_SLEEP;
				}
				else
				if(radioState == RADIO_STATE_LISTEN) 
				{
					/*
					 * It might interrupt a receiving message in the middle. How we know that we are receiving???
					 * The difficulty in knowing that is that the channel sends only the WC_PKT_END_TX message to us so
					 * it is not possible know when we start receiving. We only now when the TX of the packet ended.
					 * CURRENT APPROACH: The radio discards/drops any message if it gets a change state control
					 * 					 message from te MAC module while it is receiving data. 
					*/
					changingState = 1; //lock the MAC from changing Radio state while Radio switched state
					double currentListeningDuration;
					currentListeningDuration = simTime() - startListeningTime;
					
					//calculate accurately: the consumed power while in listening mode + the power of switching state [max(listenPower, sleepPower) * delayListen2Sleep] 
					double totalPower;
					totalPower = (listenPower * currentListeningDuration) + ((listenPower > sleepPower)?listenPower:sleepPower) * delayListen2Sleep;
					CONSUME_ENERGY(totalPower);
					totalTimeListening += currentListeningDuration;
					scheduleAt(simTime()+delayListen2Sleep, new Radio_ControlMessage("Radio self go to sleep NOW", RADIO_SELF_ENTER_SLEEP_NOW));
					
					totalTimeTransitions += delayListen2Sleep;
				}
			}
			
			break;
		}
		
		
		/*--------------------------------------------------------------------------------------------------------------
		 * 
		 *--------------------------------------------------------------------------------------------------------------*/
		case MAC_2_RADIO_ENTER_LISTEN:
		{	
			if(!changingState)
			{
				if(radioState == RADIO_STATE_TX)
				{	
					/*
					 * There is no need to send self message to manage state transition etc.
					 * Those things are performed by the RADIO_SELF_CHECK_BUFFER when the buffer gets empty
					 * That means the radio follows this strategy: 
					 * 		keep sending until your buffer gets empty and then go to the state that "nextState" indicates.
					 */
					nextState = RADIO_STATE_LISTEN;
				}
				else
				if(radioState == RADIO_STATE_SLEEP) 
				{	
					changingState = 1; //lock the MAC from changing Radio state while Radio switched state
					double currentSleepingDuration;
					currentSleepingDuration = simTime() - startSleepingTime;
					
					//calculate accurately: the consumed power while in sleeping mode + the power of switching state [max(listenPower, sleepPower) * delaySleep2Listen]
					double totalPower;
					totalPower = (sleepPower * currentSleepingDuration) + ((listenPower > sleepPower)?listenPower:sleepPower) * delaySleep2Listen;
					CONSUME_ENERGY(totalPower);
					totalTimeSleeping += currentSleepingDuration;
					scheduleAt(simTime()+delaySleep2Listen, new Radio_ControlMessage("Radio self wake up NOW", RADIO_SELF_ENTER_LISTEN_NOW));
					
					totalTimeTransitions += delaySleep2Listen;
				}
			}
			
			break;
		}
		
		
		/*--------------------------------------------------------------------------------------------------------------
		 * 
		 *--------------------------------------------------------------------------------------------------------------*/
		case MAC_2_RADIO_ENTER_TX:
		{
			if(!changingState)
			{
				if(radioState == RADIO_STATE_LISTEN)
				{
					changingState = 1; //lock the MAC from changing Radio state while Radio switched state
					
					double currentListeningDuration;
					currentListeningDuration = simTime() - startListeningTime;
					
					//calculate accurately: the consumed power while in listening mode + the power of switching state [max(listenPower, txPowerConsumptionPerLevel[txPowerLevelUsed]) * delayListen2Tx] 
					double totalPower;
					totalPower = (listenPower * currentListeningDuration) + ((listenPower > txPowerConsumptionPerLevel[txPowerLevelUsed])?listenPower:txPowerConsumptionPerLevel[txPowerLevelUsed]) * delayListen2Tx;
					CONSUME_ENERGY(totalPower);
					totalTimeListening += currentListeningDuration;
					scheduleAt(simTime()+delayListen2Tx, new Radio_ControlMessage("Radio self go to sleep NOW", RADIO_SELF_ENTER_TX_NOW));
					
					totalTimeTransitions += delayListen2Tx;
				}
				else
				if(radioState == RADIO_STATE_SLEEP) 
				{
					changingState = 1; //lock the MAC from changing Radio state while Radio switched state
					
					double currentSleepingDuration;
					currentSleepingDuration = simTime() - startSleepingTime;
					
					//calculate accurately: the consumed power while in sleeping mode + the power of switching state [max(sleepPower, txPowerConsumptionPerLevel[txPowerLevelUsed]) * delaySleep2Tx]
					double totalPower;
					totalPower = (sleepPower * currentSleepingDuration) + ((sleepPower > txPowerConsumptionPerLevel[txPowerLevelUsed])?sleepPower:txPowerConsumptionPerLevel[txPowerLevelUsed]) * delaySleep2Tx;
					CONSUME_ENERGY(totalPower);
					totalTimeSleeping += currentSleepingDuration;
					scheduleAt(simTime()+delaySleep2Tx, new Radio_ControlMessage("Radio self wake up and enter TX NOW", RADIO_SELF_ENTER_TX_NOW));
					
					totalTimeTransitions += delaySleep2Tx;
				}
				
			}
			
			break;
		}
		
		
		
		/*--------------------------------------------------------------------------------------------------------------
		 * 
		 *--------------------------------------------------------------------------------------------------------------*/
		case MAC_2_RADIO_CHANGE_TX_MODE:
		{
			MAC_ControlMessage *macCtrlMsg;
			macCtrlMsg = check_and_cast<MAC_ControlMessage*>(msg);
			int theTxMode = macCtrlMsg->getRadioTxMode();
			
			if( (theTxMode == CARRIER_SENSE_NONE) || (theTxMode == CARRIER_SENSE_ONCE_CHECK) || (theTxMode == CARRIER_SENSE_PERSISTENT) )
			{
				if(radioState != RADIO_STATE_TX) //&& (!changingState) is not causing problems to change TX_MODE while changing state
				{
					txModeUsed = theTxMode;
					nextTxMode = -1;   //this line is useless--- it can be deleted , is just a safeguard
				}
				else
					nextTxMode = theTxMode;
			}
			else
				EV << "\n[Radio_" << self << "] WARNING: The MAC Module attempted to change the TxMode of Radio with an unknown TXMode type.";
			
			macCtrlMsg = NULL;
			
			break;
		}
		
		
		/*--------------------------------------------------------------------------------------------------------------
		 * 
		 *--------------------------------------------------------------------------------------------------------------*/
		case MAC_2_RADIO_CHANGE_POWER_LEVEL:
		{
			MAC_ControlMessage *macCtrlMsg;
			macCtrlMsg = check_and_cast<MAC_ControlMessage*>(msg);
			int pwrLevelIndex = macCtrlMsg->getPowerLevel();
			
			if( (pwrLevelIndex >= 0) && (pwrLevelIndex < txPowerLevels.size()) )
			{
				if(radioState != RADIO_STATE_TX) //&& (!changingState) is not causing problems to change TX power level while changing state
				{	
					txPowerLevelUsed = pwrLevelIndex;
					nextPowerLevel = -1; //this line is useless--- it can be deleted , is just a safeguard
				}
				else
					nextPowerLevel = pwrLevelIndex; //while the radio is in TX don't change the TX_MODE until the radio buffer gets empty
			}
			else
				EV << "\n[Radio]:\n WARNING: The MAC Module attempted to change the TX power Level of Radio with an invalid pwr level index.";
			
			macCtrlMsg = NULL;
			
			break;
		}
		
		
		/*--------------------------------------------------------------------------------------------------------------
		 * 
		 *--------------------------------------------------------------------------------------------------------------*/
		case MAC_2_RADIO_SENSE_CARRIER:
		{	
			if(radioState == RADIO_STATE_TX) //it is impossible to receive from the MAC a Carrier Sense strobe while radio is in TX because in this radio state the isCarrierSenseValid() returns false
			{
				scheduleAt(simTime(), new Radio_ControlMessage("Radio2Mac carrier sensed", RADIO_2_MAC_SENSED_CARRIER));
				EV << "\n[Radio_" << self << "] WARNING: Radio received from MAC sense carrier command while in TX state!";
			}
			else
			if(radioState == RADIO_STATE_LISTEN)
			{
				MAC_ControlMessage *csMsg;
				csMsg = check_and_cast<MAC_ControlMessage*>(msg);
				
				double theInterval;
				theInterval = csMsg->getSense_carrier_interval();
				
				senseCarrier(theInterval);
				
				csMsg = NULL;
			}
			else //radioState == RADIO_STATE_SLEEP
			{
				EV << "\n[Radio_" << self << "] WARNING: Radio received from MAC sense carrier command while in SLEEP state!";
				//it is impossible to receive from the MAC a Carrier Sense strobe while radio is in SLEEP state because while the radio is sleeping isCarrierSenseValid() returns false
			}
			
			break;
		}
		
		
		/*--------------------------------------------------------------------------------------------------------------
		 * 
		 *--------------------------------------------------------------------------------------------------------------*/
		case MAC_FRAME:
		{
			if(!BUFFER_IS_FULL)
			{
				MAC_GenericFrame *dataFrame = check_and_cast<MAC_GenericFrame*>(msg);
				
				// create a new copy of the message because dataFrame will be deleted outside the switch statement
				MAC_GenericFrame *duplMsg = (MAC_GenericFrame *)dataFrame->dup(); //because theFrame will be deleted after the main switch in the handleMessage()
				
				if(!pushBuffer(duplMsg))
				{
					MAC_ControlMessage *fullBuffMsg = new MAC_ControlMessage("Radio buffer is full Radio->Mac", RADIO_2_MAC_FULL_BUFFER);
					
					send(fullBuffMsg, "toMacModule");
					
					EV << "\n[Radio_" << self << "] WARNING: The buffer of the Radio is Full!";
				}
			}
			else
				EV << "\n[Radio_" << self << "] WARNING: Radio Buffer is FULL!!! MAC data frame is dropped";
			break;
		}
		
		
		
		/*--------------------------------------------------------------------------------------------------------------
		 * 
		 *--------------------------------------------------------------------------------------------------------------*/
		case RADIO_SELF_ENTER_SLEEP_NOW:
		{
			isCSValid = 0;
			changingState = 0; //unlock the MAC from disabled changing Radio state
			nextState = -1;
			//framesToNextState = 0;
			startSleepingTime = simTime();
			radioState = RADIO_STATE_SLEEP;
			EV << "\n[Radio_" << self << "] _2_ State changed to RADIO_STATE_SLEEP";
			break;
		}
		
		
		
		/*--------------------------------------------------------------------------------------------------------------
		 * 
		 *--------------------------------------------------------------------------------------------------------------*/
		case RADIO_SELF_ENTER_LISTEN_NOW:
		{
			isCSValid = 0;		//not actually needed because it is set in "case RADIO_SELF_ENTER_SLEEP_NOW" as well
			scheduleAt(simTime()+delayCSValid, new Radio_ControlMessage("Radio self set carrier sense indication as valid NOW", RADIO_SELF_VALIDATE_CS));
			changingState = 0;	//unlock the MAC from disabled changing Radio state
			nextState = -1;
			startListeningTime = simTime();
			radioState = RADIO_STATE_LISTEN;
			EV << "\n[Radio_" << self << "] _3_ State changed to RADIO_STATE_LISTEN";
			break;
		}
		
		
		
		/*--------------------------------------------------------------------------------------------------------------
		 * 
		 *--------------------------------------------------------------------------------------------------------------*/
		case RADIO_SELF_ENTER_TX_NOW:
		{
			switch(txModeUsed)
			{
				case CARRIER_SENSE_NONE:
				{
					isCSValid = 0;
					changingState = 0; //unlock the MAC from disabled changing Radio state
					nextState = -1;
					startTxTime = simTime();
					radioState = RADIO_STATE_TX;
					EV << "\n[Radio_" << self << "] _4_ State changed to RADIO_STATE_TX";
			
					scheduleAt(simTime(), new Radio_ControlMessage("Radio self send to Channel NOW", RADIO_SELF_CHECK_BUFFER));
					break;
				}
				
				case CARRIER_SENSE_ONCE_CHECK:
				{
					/*
					 * TODO: Add code here for the implementation of Carrier Sense of the Radio 
					 */
					break;
				}
				
				case CARRIER_SENSE_PERSISTENT:
				{
					/*
					 * TODO: Add code here for the implementation of Persistent Carrier Sense of the Radio 
					 */
					break;
				}
			}//switch
			
			break;
		}
		
		
		
		case RADIO_SELF_CHECK_BUFFER:
		{
			if(radioState == RADIO_STATE_TX)
			{
				if(!BUFFER_IS_EMPTY)
				{	

					double txTime = popAndSendToChannel();
					
					// schedule a self-msg to check again the buffer after txTime
					scheduleAt(simTime()+ txTime, new Radio_ControlMessage("Radio self send to Channel NOW", RADIO_SELF_CHECK_BUFFER));
					
					if(!txTime)
						EV << "\n[Radio_" << self << "] Failed to pop the buffer and send to wireless channel.";
				}
				else //radio buffer is empty
				{
					isCSValid = 0;
					
					double currentTxDuration;
					currentTxDuration = simTime()- startTxTime;
					//calculate accurately: the consumed power while in TX mode + the power consumed while switching state after TX (TX->Listen or TX->Sleep)
					double totalPower;
					
					//schedule the self messages that 'll perform the state change & calculate the consumed power while in TX + switch time 
					if(nextState == RADIO_STATE_SLEEP)
					{
						//radioState = RADIO_STATE_SLEEP;
						//EV << "\n[Radio_" << self <<"]:\n _5_ State changed to RADIO_STATE_SLEEP";
						totalPower = txPowerConsumptionPerLevel[txPowerLevelUsed] * (currentTxDuration + delayTx2Sleep); //TX consumes always more power than Listen or Sleep
						scheduleAt(simTime()+delayTx2Sleep, new Radio_ControlMessage("Radio self go to sleep NOW", RADIO_SELF_ENTER_SLEEP_NOW));
						
						totalTimeTransitions += delayTx2Sleep;
					}
					else
					{
						//radioState = RADIO_STATE_LISTEN;
						//V << "\n[Radio_" << self <<"]:\n _6_ State changed to RADIO_STATE_LISTEN";
						totalPower = (txPowerConsumptionPerLevel[txPowerLevelUsed] * currentTxDuration) + ((listenPower > txPowerConsumptionPerLevel[txPowerLevelUsed])?listenPower:txPowerConsumptionPerLevel[txPowerLevelUsed]) * delayTx2Listen;
						scheduleAt(simTime()+delayTx2Listen, new Radio_ControlMessage("Radio self enter listen NOW", RADIO_SELF_ENTER_LISTEN_NOW));
						
						totalTimeTransitions += delayTx2Listen;
					}
					
					CONSUME_ENERGY(totalPower);
					totalTimeTX += currentTxDuration;
					nextState = -1;
					changingState = 1; //lock the MAC from changing Radio state while Radio switched state
					
					
					//Perform changes in the TxMode of the Radio if there were requests from the MAC while Radio was in TX mode
					if(nextTxMode != -1)
					{
						txModeUsed = nextTxMode;
						nextTxMode = -1;
					}
					
					
					//Perform changes in the power Level of TX of the Radio if there were requests from the MAC while Radio was in TX mode
					if(nextPowerLevel != -1)
					{	
						txPowerLevelUsed = nextPowerLevel;  //we don't have to worry about the validity of the nextPowerLevel, it is checked in case MAC_2_RADIO_CHANGE_POWER_LEVEL
						nextPowerLevel = -1;
					}
				}
			}//if(radioState == RADIO_STATE_TX)
			
			break;
		}
		
		
		
		/*--------------------------------------------------------------------------------------------------------------
		 * 
		 *--------------------------------------------------------------------------------------------------------------*/
		case RADIO_SELF_VALIDATE_CS:
		{	
			if(!isCSValid)
				isCSValid = 1;
			
			break;
		}
	
		
		
		/*--------------------------------------------------------------------------------------------------------------
		 * 
		 *--------------------------------------------------------------------------------------------------------------*/
		case WC_PKT_END_TX:
		{
			WChannel_GenericMessage *rcvFrame = check_and_cast<WChannel_GenericMessage*>(msg);
			
			int frameLen;
			frameLen = rcvFrame->byteLength();
			
			double currentRxDuration;
			currentRxDuration = frameLen * 8 / (1000 * dataRate);
			
			MAC_GenericFrame *macFrame; // = new MAC_GenericFrame("Frame to be sent to the Mac module", MAC_FRAME);
			macFrame = (check_and_cast<MAC_GenericFrame *>(rcvFrame->decapsulate()));			
			
			if( (radioState == RADIO_STATE_LISTEN) && ((simTime() - startListeningTime) > currentRxDuration) )
			{
				send(macFrame, "toMacModule");
				
				double totalPowerRX = (rxPower-listenPower) * (currentRxDuration);
				
				CONSUME_ENERGY(totalPowerRX);
				
				totalTimeListening -= currentRxDuration;
				
				totalTimeRX += currentRxDuration;
			}
			else
			{
				if(printStatistics)
				{	
					if(isBeacon(macFrame))
						droppedBeacons++;
					else
						droppedDataFrames++;
				}
				
				cancelAndDelete(macFrame);
				macFrame = NULL;
			}
			
			rcvFrame = NULL;
			
			break;
		}
		
		
		/*--------------------------------------------------------------------------------------------------------------
		 * 
		 *--------------------------------------------------------------------------------------------------------------*/
		case WC_CARRIER_SENSED:
		{
			MAC_ControlMessage *csMsg = new MAC_ControlMessage("carrier sensed Radio->Mac", RADIO_2_MAC_SENSED_CARRIER);
			send(csMsg, "toMacModule");
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
			EV << "\n[Radio_" << self << "] WARNING: received packet of unknown type";
			break;
		}
		
	}
	 
	delete msg;
	msg = NULL;		// safeguard
}



void RadioModule::finish() 
{
	MAC_GenericFrame *macMsg;
	//App_GenericDataPacket *appMsg;
	
	while(!BUFFER_IS_EMPTY)
	{	
	  	macMsg = popBuffer();
		//appMsg = check_and_cast<App_GenericDataPacket *>(macMsg->decapsulate());
		//EV << "\n--Value(Radio Cleanup): " << macMsg->getHeader().srcID << "\n";
		cancelAndDelete(macMsg);
		//cancelAndDelete(appMsg);
	  	macMsg = NULL;
	  	//appMsg = NULL;
	}
	
	simtime_t now = simTime();
	
	double totalPower;
	
	if(radioState == RADIO_STATE_TX)
	{
		totalTimeTX += now - startTxTime;
		totalPower = (now - startTxTime) * txPowerConsumptionPerLevel[txPowerLevelUsed];
	}
	else
	if(radioState == RADIO_STATE_LISTEN)
	{
		totalTimeListening += now - startListeningTime;
		totalPower = (now - startListeningTime) * listenPower;
	}
	else
	if(radioState == RADIO_STATE_SLEEP) //this if statement can be ommited, the "else" by its own is enough
	{
		totalTimeSleeping += now - startSleepingTime;
		totalPower = (now - startSleepingTime) * sleepPower;
	}
	
	CONSUME_ENERGY(totalPower);
	
	endT = now;
	recordScalar("Total Simulation time", endT - beginT);
	
	//recordScalar("Times Validated CS", timesBlockTX);
	//recordScalar("Times attempt to set to listen",timesBlockTX2);
	recordScalar("Total time in TX state", totalTimeTX);
	recordScalar("Total time in RX state", totalTimeRX);
	recordScalar("Total time in Listening state", totalTimeListening);
	recordScalar("Total time in Sleeping state", totalTimeSleeping);
	recordScalar("Total time of transitions", totalTimeTransitions);
	recordScalar("Max Radio-buffer size recorded", maxBufferSizeRecorded);
	
	if(printStatistics)
	{
		EV << "\n\nNode[" << self << "] DROPPED (not successfully received) packets breakdown\n(example causes: radio not listening for the whole or part of the duration of a packet's transmission)";
		EV << "\nNode[" << self << "]   *  Radio --> number of dropped Packets at Radio module: \t(" << droppedBeacons << ") beacons   and   (" << droppedDataFrames << ") data frames";
	}
		
	delete radioBuffer;
}


void RadioModule::readIniFileParameters(void)
{
	printStatistics = par("printDroppedStatistics");
	
	dataRate = par("dataRate");
	
	rxPower = (double)par("rxPower") / 1000;
	
	listenPower = (double)par("listenPower") / 1000;
	
	sleepPower = (double)par("sleepPower") / 1000;
	
	const char *parameterStr, *token;
	txPowerConsumptionPerLevel.clear();
	parameterStr = par("txPowerConsumptionPerLevel");
	cStringTokenizer pwrTokenizer(parameterStr);
	while ( (token = pwrTokenizer.nextToken()) != NULL )
    	txPowerConsumptionPerLevel.push_back((double)(atof(token)/1000));
	
	txPowerLevelUsed = par("txPowerLevelUsed");
	
	bufferSize = par("bufferSize");
	
	maxPhyFrameSize = par("maxPhyFrameSize");
	
	PhyFrameOverhead = par("phyFrameOverhead");
	
	delaySleep2Listen = ((double)par("delaySleep2Listen")) / 1000;
	
	delayListen2Sleep = ((double)par("delayListen2Sleep")) / 1000;
	
	delayTx2Sleep = ((double)par("delayTx2Sleep")) / 1000;
	
	delaySleep2Tx = ((double)par("delaySleep2Tx")) / 1000;
	
	delayListen2Tx = ((double)par("delayListen2Tx")) / 1000;
	
	delayTx2Listen = ((double)par("delayTx2Listen")) / 1000;
	
	int tmpDefMode = par("txModeUsed");
	switch(tmpDefMode)
	{
		case 0:
		{
			txModeUsed = CARRIER_SENSE_NONE;
			break;
		}
		
		case 1:
		{
			txModeUsed = CARRIER_SENSE_ONCE_CHECK;
			break;
		}
		
		case 2:
		{
			txModeUsed = CARRIER_SENSE_PERSISTENT;
			break;
		}
		
		default:
		{
			opp_error("Error initialization value of parameter \"defaultTxMode\" of node %d in the omnet.ini file.", self);
			break; //this line is unreachable
		}
	}
	
	delayCSValid = ((double)par("delayCSValid")) / 1000;
	
}



void RadioModule::senseCarrier(double interval) {
	// send the delayed messages to the wireless channel
	WChannel_GenericMessage *message;
	
	message = new WChannel_GenericMessage("begin carrier sense", WC_CARRIER_SENSE_BEGIN);
	message->setSrcAddress(self);
	send( message, "toCommunicationModule");
	
	message = new WChannel_GenericMessage("end carrier sense", WC_CARRIER_SENSE_END);
	message->setSrcAddress(self);
	sendDelayed( message, interval, "toCommunicationModule");
}


int RadioModule::pushBuffer(MAC_GenericFrame *theFrame)
{
	if(theFrame == NULL)
	{
		EV << "\n[Radio_" << self << "] WARNING: Trying to push  NULL MAC_GenericFrame to the Radio_Buffer!!";
		return 0;
	}
	
	/*tailTxBuffer++;
	if (tailTxBuffer == bufferSize)
		tailTxBuffer = 0;*/
	tailTxBuffer = (++tailTxBuffer)%(RADIO_BUFFER_ARRAY_SIZE);

	if (tailTxBuffer == headTxBuffer)
	{
		// reset tail pointer
		if(tailTxBuffer == 0)
			tailTxBuffer = RADIO_BUFFER_ARRAY_SIZE-1;
		else
			tailTxBuffer--;
		EV << "\n[Radio_" << self << "] WARNING: radioBuffer FULL!!! value to be Tx not added to buffer";
		return 0;
	}
	
	if (tailTxBuffer==0) 
		radioBuffer[RADIO_BUFFER_ARRAY_SIZE-1] = theFrame;
	else 
		radioBuffer[tailTxBuffer-1] = theFrame;
		
	
	int currLen = getRadioBufferSize(); //(int) schedTXBuffer.size();//length();
	if (currLen > maxBufferSizeRecorded) 
		maxBufferSizeRecorded = currLen;

	return 1;
}


MAC_GenericFrame* RadioModule::popBuffer()
{
	if (tailTxBuffer == headTxBuffer) {
		EV << "\n[Radio_" << self << "] Trying to pop  EMPTY Radio Buffer!!";
		tailTxBuffer--;  // reset tail pointer
		return NULL;
	}

	MAC_GenericFrame *pop_message = radioBuffer[headTxBuffer];
	radioBuffer[headTxBuffer] = NULL;
	
	/*headTxBuffer++;
	if (headTxBuffer == bufferSize) 
		headTxBuffer = 0;*/
	
	headTxBuffer = (++headTxBuffer)%(RADIO_BUFFER_ARRAY_SIZE);

	return pop_message;
}


// SOS: it must be called if we previously have checked the radioBuffer to be NON EMPTY!!!!!
double RadioModule::popAndSendToChannel()
{
	//this message 'll hold the popped frame from the buffer
	double txTime = 0;
	
	MAC_GenericFrame *poppedMacFrame = popBuffer();
	
	if(poppedMacFrame != NULL)
	{	
		WChannel_GenericMessage *end = new WChannel_GenericMessage("Frame to be sent to the Wireless Channel");
		if(encapsulateMacFrame(poppedMacFrame, end))  //the "end" message holds the encapsulated Mac-frame
		{
			//Generate and send "packet begin message"
			WChannel_GenericMessage *begin = new WChannel_GenericMessage("Radio begins Tx", WC_PKT_BEGIN_TX);
			begin->setSrcAddress(self);
			begin->setDestAddress(end->getDestAddress());
			begin->setPowerLevel(txPowerLevelUsed);
			send(begin, "toCommunicationModule");
	
			//send "packet end message"
			end->setKind(WC_PKT_END_TX);
			txTime = end->byteLength() * 8 / (1000 * dataRate);  //calculate the TX time for the Length of frame2send frame
			sendDelayed(end, txTime, "toCommunicationModule");
			
			// Don't delete the popped frames because we perform true encapsulation
			//cancelAndDelete(poppedMacFrame);
			//poppedMacFrame = NULL;
		}
		else
		{
			EV << "\n[Radio_" << self << "] WARNING: Mac sent to Radio an oversized packet...packet dropped!!";
			cancelAndDelete(end);
			cancelAndDelete(poppedMacFrame);
			end = NULL;
			poppedMacFrame = NULL;
		}
	}
	return txTime;
}



int RadioModule::encapsulateMacFrame(MAC_GenericFrame *macFrame, WChannel_GenericMessage *retWcFrame)
{
	int totalMsgLen = macFrame->byteLength()+PhyFrameOverhead;
	if(totalMsgLen > maxPhyFrameSize)
		return 0;
	retWcFrame->setByteLength(PhyFrameOverhead); //macFrame->byteLength() extra bytes will be added after the encapsulation
	
	retWcFrame->setSrcAddress(macFrame->getHeader().srcID); //simulation-specific information
	retWcFrame->setDestAddress(macFrame->getHeader().destID); //simulation-specific information
	retWcFrame->setPowerLevel(txPowerLevelUsed); //simulation-specific information
	
	//no need for dup() becasue the macFrame was stored in the MacBuffer and will not be deleted after the main "switch" statement
	retWcFrame->encapsulate(macFrame);
	return 1;
}



/*
 * Returns :
 * 0 if it is not valid  + assigns a value in "reason" that indicates why it is not valid
 * 1 if it is valid
 */
int RadioModule::isCarrierSenseValid()
{
	Enter_Method("isCarrierSenseValid(int & reason)"); //because this method is public and can be called from other modules
	
	int reason; //holds the reason for non valid carrier sense
	
	if(isCSValid)
		return 1;
	else
	{
		if(radioState == RADIO_STATE_TX)
			reason = RADIO_IN_TX_MODE;
		else
		if(radioState == RADIO_STATE_LISTEN)
			reason = RADIO_NON_READY;
		else
		if(radioState == RADIO_STATE_SLEEP)
			reason = RADIO_SLEEPING;
		else
			EV << "\n[Radio_" << self << "] WARNING: in function isCarrierSenseValid(), non valid radioState.";
	}
	
	return reason;
}


int RadioModule::getTotalTxPowerLevels(void)
{
	Enter_Method("getTotalTxPowerLevels(void)"); //because this method is public and can be called from other modules
	
	return (int)txPowerLevels.size();
}

 
int RadioModule::getRadioBufferSize(void)
{
	int size = tailTxBuffer - headTxBuffer;
	if ( size < 0 )
		size += RADIO_BUFFER_ARRAY_SIZE;
		
	return size;
}

int RadioModule::isBeacon(MAC_GenericFrame *theFrame)
{
	return(theFrame->getHeader().frameType == MAC_PROTO_BEACON_FRAME);
}

