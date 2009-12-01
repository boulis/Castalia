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



#include "BypassMacModule.h"

#define DRIFTED_TIME(time) ((time) * cpuClockDrift)

//#define EV   ev.isDisabled() ? (ostream&)ev : ev ==> EV is now part of <omnetpp.h>

#define CASTALIA_DEBUG (!printDebugInfo)?(ostream&)DebugInfoWriter::getStream():DebugInfoWriter::getStream()



Define_Module(BypassMacModule);



void BypassMacModule::initialize()
{
	readIniFileParameters();

	//--------------------------------------------------------------------------------
	//------- Follows code for the initialization of the class member variables ------

	self = getParentModule()->getParentModule()->getIndex();

	//get a valid reference to the object of the Radio module so that we can make direct calls to its public methods
	//instead of using extra messages & message types for tighlty couplped operations.
	radioModule = check_and_cast<RadioModule*>(gate("toRadioModule")->getNextGate()->getOwnerModule());
	radioDataRate = (double) radioModule->par("dataRate");
	radioDelayForValidCS = ((double) radioModule->par("delayCSValid"))/1000.0; //parameter given in ms in the omnetpp.ini

	phyLayerOverhead = radioModule->par("phyFrameOverhead");

	//get a valid reference to the object of the Resources Manager module so that we can make direct calls to its public methods
	//instead of using extra messages & message types for tighlty couplped operations.
	cModule *parentParent = getParentModule()->getParentModule();
	if(parentParent->findSubmodule("nodeResourceMgr") != -1)
	{
		resMgrModule = check_and_cast<ResourceGenericManager*>(parentParent->getSubmodule("nodeResourceMgr"));
	}
	else
		opp_error("\n[Mac]:\n Error in geting a valid reference to  nodeResourceMgr for direct method calls.");
	cpuClockDrift = resMgrModule->getCPUClockDrift();

	epsilon = 0.000001;

	disabled = 1;
}


void BypassMacModule::handleMessage(cMessage *msg)
{
	int msgKind = msg->getKind();


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

			break;
		}


		/*--------------------------------------------------------------------------------------------------------------
		 * Sent by the Network module. We need to push it into the buffer and schedule its forwarding to the Radio buffer for TX.
		 *--------------------------------------------------------------------------------------------------------------*/
		case NETWORK_FRAME:
		{			
			if((int)TXBuffer.size() < macBufferSize)
			{
				Network_GenericFrame *rcvNetDataFrame = check_and_cast<Network_GenericFrame*>(msg);
				
				char buff[50];
				sprintf(buff, "MAC Data frame (%f)", SIMTIME_DBL(simTime()));
				MAC_GenericFrame *newDataFrame = new MAC_GenericFrame(buff, MAC_FRAME);
				
				//create the MACFrame from the Network Data Packet (encapsulation)
				if(encapsulateNetworkFrame(rcvNetDataFrame, newDataFrame))
				{
					newDataFrame->setKind(MAC_FRAME);
					TXBuffer.push(newDataFrame);
					// schedule a new TX
					scheduleAt(simTime() + epsilon, new MAC_ControlMessage("initiate a TX", MAC_SELF_CHECK_TX_BUFFER));
				}
				else
				{
					cancelAndDelete(newDataFrame);
					newDataFrame = NULL;
					CASTALIA_DEBUG << "\n[Mac_" << self <<"] t= " << simTime() << ": WARNING: Network module sent to MAC an oversized packet...packet dropped!!\n";
				}

				rcvNetDataFrame = NULL;
				//newDataFrame = NULL;
			}
			else
			{
				MAC_ControlMessage *fullBuffMsg = new MAC_ControlMessage("MAC buffer is full Radio->Mac", MAC_2_NETWORK_FULL_BUFFER);
				send(fullBuffMsg, "toNetworkModule");
				CASTALIA_DEBUG  << "\n[Mac_"<< self <<"] t= " << simTime() << ": WARNING: SchedTxBuffer FULL!!! Network frame is discarded.\n";
			}

			break;
		}//END_OF_CASE(NETWORK_FRAME)



		/*--------------------------------------------------------------------------------------------------------------
		 * Sent by the Radio module in order for the MAC module to update its state to MAC_STATE_TX
		 *--------------------------------------------------------------------------------------------------------------*/
		case RADIO_2_MAC_STARTED_TX:
		{
			//scheduleAt(simTime(), new MAC_ControlMessage("check schedTXBuffer buffer", MAC_SELF_CHECK_TX_BUFFER));
			break;
		}



		/*--------------------------------------------------------------------------------------------------------------
		 * Sent by the Radio module in order the MAC module to exit the MAC_STATE_TX state
		 *--------------------------------------------------------------------------------------------------------------*/
		case RADIO_2_MAC_STOPPED_TX:
		{
			//scheduleAt(simTime(), new MAC_ControlMessage("check schedTXBuffer buffer", MAC_SELF_CHECK_TX_BUFFER));
			break;
		}



		/*--------------------------------------------------------------------------------------------------------------
		 * Sent by the MAC submodule itself to send the next Frame to Radio
		 *--------------------------------------------------------------------------------------------------------------*/
		case MAC_SELF_CHECK_TX_BUFFER:
		{	
			if(!TXBuffer.empty())
			{
				
				// SEND THE DATA FRAME TO RADIO BUFFER
				MAC_GenericFrame *dataFrame = TXBuffer.front(); 
				TXBuffer.pop();

				send(dataFrame, "toRadioModule");
				
				setRadioState(MAC_2_RADIO_ENTER_TX, epsilon);
				
				/*if(!(BUFFER_IS_EMPTY))
				{
					double dataTXtime = ((double)(dataFrame->getByteLength()+phyLayerOverhead) * 8.0 / (1000.0 * radioDataRate));
					
					scheduleAt(simTime() + dataTXtime + epsilon, new MAC_ControlMessage("check schedTXBuffer buffer", MAC_SELF_CHECK_TX_BUFFER));
				}*/
				
				dataFrame = NULL;
			}

			break;
		}



		/*--------------------------------------------------------------------------------------------------------------
		 * Data Frame Received from the Radio getSubmodule(the data frame can be a Data packet or a beacon packet)
		 *--------------------------------------------------------------------------------------------------------------*/
		case MAC_FRAME:
		{
			MAC_GenericFrame *rcvFrame;
			rcvFrame = check_and_cast<MAC_GenericFrame*>(msg);
			
			//control the destination of the message and drop packets accordingly
			int destinationID = rcvFrame->getHeader().destID;
			
			if( (destinationID == self) || (destinationID == MAC_BROADCAST_ADDR) )
			{
				// we can just forward it to the Application or to Routing/Network module so that we decide whether or not to keep the packet
		
				//DELIVER THE DATA PACKET TO Network MODULE
				Network_GenericFrame *netDataFrame;
				//No need to create a new message because of the decapsulation: netDataFrame = new Network_GenericFrame("Network Frame MAC->Network", NETWORK_FRAME);
		
				//decaspulate the received MAC frame and create a valid Network_GenericFrame
				netDataFrame = check_and_cast<Network_GenericFrame *>(rcvFrame->decapsulate());
				
				netDataFrame->setRssi(rcvFrame->getRssi());

				//send the Network_GenericFrame message to the Network module
				send(netDataFrame, "toNetworkModule");
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
			 
			 MAC_ControlMessage *fullBuffMsg = new MAC_ControlMessage("MAC buffer is full Radio->Mac->Network", MAC_2_NETWORK_FULL_BUFFER);
			 
			 send(fullBuffMsg, "toNetworkModule");
			 
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
		 * Message received by the Resource Manager module. It commands the module to stop its operation.
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
		


		default:
		{
			CASTALIA_DEBUG << "\n[Mac_"<< self <<"] t= " << simTime() << ": WARNING: received packet of unknown type\n";
			break;
		}

	}//end_of_switch

	delete msg;
	msg = NULL;		// safeguard
}

void BypassMacModule::finish()
{	
	MAC_GenericFrame *macMsg;

	while(!TXBuffer.empty())
	{
		macMsg = TXBuffer.front();
		TXBuffer.pop();
		cancelAndDelete(macMsg);
	  	macMsg = NULL;
	}
}


void BypassMacModule::readIniFileParameters(void)
{
	printDebugInfo = par("printDebugInfo");
	maxMACFrameSize = par("maxMACFrameSize");
	macFrameOverhead = par("macFrameOverhead");
	macBufferSize = par("macBufferSize");
}

void BypassMacModule::setRadioState(MAC_ControlMessageType typeID, double delay)
{
	if( (typeID != MAC_2_RADIO_ENTER_SLEEP) && (typeID != MAC_2_RADIO_ENTER_LISTEN) && (typeID != MAC_2_RADIO_ENTER_TX) )
		opp_error("MAC attempt to set Radio into an unknown state. ERROR commandID");

	MAC_ControlMessage * ctrlMsg = new MAC_ControlMessage("state command strobe MAC->radio", typeID);

	sendDelayed(ctrlMsg, delay, "toRadioModule");
}

void BypassMacModule::setRadioTxMode(Radio_TxMode txTypeID, double delay)
{
	if( (txTypeID != CARRIER_SENSE_NONE)&&(txTypeID != CARRIER_SENSE_ONCE_CHECK)&&(txTypeID != CARRIER_SENSE_PERSISTENT) )
		opp_error("MAC attempt to set Radio CarrierSense into an unknown type. ERROR commandID");

	MAC_ControlMessage * ctrlMsg = new MAC_ControlMessage("Change Radio TX mode command strobe MAC->radio", MAC_2_RADIO_CHANGE_TX_MODE);
	ctrlMsg->setRadioTxMode(txTypeID);

	sendDelayed(ctrlMsg, delay, "toRadioModule");
}

void BypassMacModule::setRadioPowerLevel(int powLevel, double delay)
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

int BypassMacModule::encapsulateNetworkFrame(Network_GenericFrame *networkFrame, MAC_GenericFrame *retFrame)
{
	int totalMsgLen = networkFrame->getByteLength() + macFrameOverhead;
	if(totalMsgLen > maxMACFrameSize) return 0;
	retFrame->setByteLength(macFrameOverhead); //networkFrame->getByteLength() extra bytes will be added after the encapsulation
	retFrame->getHeader().srcID = self;
	retFrame->getHeader().destID = deliver2NextHop(networkFrame->getHeader().nextHop.c_str());
	retFrame->getHeader().frameType = (unsigned short)MAC_PROTO_DATA_FRAME;
	Network_GenericFrame *dupNetworkFrame = check_and_cast<Network_GenericFrame *>(networkFrame->dup());
	retFrame->encapsulate(dupNetworkFrame);

	return 1;
}

int BypassMacModule::deliver2NextHop(const char *nextHop)
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
