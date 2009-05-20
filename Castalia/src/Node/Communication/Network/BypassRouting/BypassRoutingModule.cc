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



#include "BypassRoutingModule.h"

#define NET_BUFFER_ARRAY_SIZE netBufferSize+1

#define BUFFER_IS_EMPTY  (headTxBuffer==tailTxBuffer)

#define BUFFER_IS_FULL  (getTXBufferSize() >= netBufferSize)

#define DRIFTED_TIME(time) ((time) * cpuClockDrift)

#define EV   ev.disabled() ? (ostream&)ev : ev

#define CASTALIA_DEBUG (!printDebugInfo)?(ostream&)DebugInfoWriter::getStream():DebugInfoWriter::getStream()



Define_Module(BypassRoutingModule);



void BypassRoutingModule::initialize()
{
	readIniFileParameters();

	//--------------------------------------------------------------------------------
	//------- Follows code for the initialization of the class member variables ------

	self = parentModule()->parentModule()->index();

	//get a valid reference to the object of the Radio module so that we can make direct calls to its public methods
	//instead of using extra messages & message types for tighlty couplped operations.
	radioModule = check_and_cast<RadioModule*>(gate("toMacModule")->toGate()->ownerModule()->gate("toRadioModule")->toGate()->ownerModule());
	radioDataRate = (double) radioModule->par("dataRate");

	macFrameOverhead = gate("toMacModule")->toGate()->ownerModule()->par("macFrameOverhead");

	//get a valid reference to the object of the Resources Manager module so that we can make direct calls to its public methods
	//instead of using extra messages & message types for tighlty couplped operations.
	cModule *parentParent = parentModule()->parentModule();
	if(parentParent->findSubmodule("nodeResourceMgr") != -1)
	{
		resMgrModule = check_and_cast<ResourceGenericManager*>(parentParent->submodule("nodeResourceMgr"));
	}
	else
		opp_error("\n[Network]:\n Error in geting a valid reference to  nodeResourceMgr for direct method calls.");
	cpuClockDrift = resMgrModule->getCPUClockDrift();
	
	schedTXBuffer = new Network_GenericFrame*[NET_BUFFER_ARRAY_SIZE];
	headTxBuffer = 0;
	tailTxBuffer = 0;

	maxSchedTXBufferSizeRecorded = 0;
	
	epsilon = 0.000001;
	
	disabled = 1;
	
	char buff[256];
	sprintf(buff, "%i", self);
	strSelfID.assign(buff);
}


void BypassRoutingModule::handleMessage(cMessage *msg)
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
		 * Sent by the Application submodule in order to start/switch-on the Network submodule.
		 *--------------------------------------------------------------------------------------------------------------*/
		case APP_NODE_STARTUP:
		{
			disabled = 0;
			
			send(new App_ControlMessage("Network --> Mac startup message", APP_NODE_STARTUP), "toMacModule");

			break;
		}


		/*--------------------------------------------------------------------------------------------------------------
		 * Sent by the Application submodule. We need to push it into the buffer and schedule its forwarding to the MAC buffer for TX.
		 *--------------------------------------------------------------------------------------------------------------*/
		case APP_DATA_PACKET:
		{			
			if(!BUFFER_IS_FULL)
			{
				App_GenericDataPacket *rcvAppDataPacket = check_and_cast<App_GenericDataPacket*>(msg);
				
				char buff[50];
				sprintf(buff, "Network Data frame (%f)", simTime());
				Network_GenericFrame *newDataFrame = new Network_GenericFrame(buff, NETWORK_FRAME);
				
				//create the NetworkFrame from the Application Data Packet (encapsulation)
				if(encapsulateAppPacket(rcvAppDataPacket, newDataFrame))
				{
					// Forward the packet based on the destination field of the application module's frame
					string appPktDestination(rcvAppDataPacket->getHeader().destination.c_str());
					if( (appPktDestination.compare(BROADCAST) == 0) ||
					    (appPktDestination.compare(SINK) ==  0) ||
					    (appPktDestination.compare(PARENT_LEVEL) == 0) )
							newDataFrame->getHeader().nextHop = BROADCAST;
					else
						newDataFrame->getHeader().nextHop = appPktDestination.c_str();
					
					
					pushBuffer(newDataFrame);
					
						
					scheduleAt(simTime() + epsilon, new Network_ControlMessage("initiate a TX", NETWORK_SELF_CHECK_TX_BUFFER));
				}
				else
				{
					cancelAndDelete(newDataFrame);
					newDataFrame = NULL;
					CASTALIA_DEBUG << "\n[Network_" << self <<"] t= " << simTime() << ": WARNING: Application sent to Network an oversized packet...packet dropped!!\n";
				}

				rcvAppDataPacket = NULL;
				//newDataFrame = NULL;
			}
			else
			{
				Network_ControlMessage *fullBuffMsg = new Network_ControlMessage("Network buffer is full Network->Application", NETWORK_2_APP_FULL_BUFFER);

				send(fullBuffMsg, "toCommunicationModule");
				
				CASTALIA_DEBUG << "\n[Network_" << self <<"] t= " << simTime() << ": WARNING: SchedTxBuffer FULL!!! Application data packet is discarded";
			}

			break;
		}//END_OF_CASE(APP_DATA_PACKET)



		/*--------------------------------------------------------------------------------------------------------------
		 * Sent by the Network submodule itself to send the next Frame to Radio
		 *--------------------------------------------------------------------------------------------------------------*/
		case NETWORK_SELF_CHECK_TX_BUFFER:
		{	
			if(!(BUFFER_IS_EMPTY))
			{
				
				// SEND THE DATA FRAME TO MAC BUFFER
				Network_GenericFrame *dataFrame = popTxBuffer();

				send(dataFrame, "toMacModule");
				
				/*if(!(BUFFER_IS_EMPTY))
				{
					double dataTXtime = ((double)(dataFrame->byteLength()+netFrameOverhead) * 8.0 / (1000.0 * radioDataRate));
					
					scheduleAt(simTime() + dataTXtime + epsilon, new Network_ControlMessage("check schedTXBuffer buffer", NETWORK_SELF_CHECK_TX_BUFFER));
				}*/
				
				dataFrame = NULL;
			}

			break;
		}



		/*--------------------------------------------------------------------------------------------------------------
		 * Data Frame Received from the Radio submodule (the data frame can be a Data packet or a beacon packet)
		 *--------------------------------------------------------------------------------------------------------------*/
		case NETWORK_FRAME:
		{			
			Network_GenericFrame *rcvFrame;
			rcvFrame = check_and_cast<Network_GenericFrame*>(msg);
			
			//TODO: here we can add code to control the destination of the message and drop packets accordingly
			//(int)rcvFrame->getHeader().applicationID
			
			if(((int) rcvFrame->getHeader().frameType) == NET_PROTOCOL_DATA_FRAME)
			{
				//DELIVER THE DATA PACKET TO APPLICATION
				App_GenericDataPacket *appDataPacket;
					
				//No need to create a new message because of the decapsulation: appDataPacket = new App_GenericDataPacket("Application Packet Network->Application", APP_DATA_PACKET);
				//decaspulate the received Network frame and create a valid App_GenericDataPacket
				appDataPacket = check_and_cast<App_GenericDataPacket *>(rcvFrame->decapsulate());
				
				appDataPacket->setRssi(rcvFrame->getRssi());
				appDataPacket->setCurrentPathFromSource(rcvFrame->getCurrentPathFromSource());
				
				//send the App_GenericDataPacket message to the Application module
				send(appDataPacket, "toCommunicationModule");
			}

			break;
		}


		case MAC_2_NETWORK_FULL_BUFFER:
		{
			/*
			 * TODO:
			 * add your code here to manage the situation of a full MAC buffer.
			 * Apparently we 'll have to stop sending messages and enter into listen or sleep mode (depending on the Network protocol that we implement).
			 */
			 
			 Network_ControlMessage *fullBuffMsg = new Network_ControlMessage("Network buffer is full Mac->Network->Application", NETWORK_2_APP_FULL_BUFFER);
			 
			 send(fullBuffMsg, "toCommunicationModule");
			 
			 CASTALIA_DEBUG << "\n[Network_"<< self <<"] t= " << simTime() << ": WARNING: MAC_2_NET_FULL_BUFFER received because the MAC buffer is full.\n";
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
		
		
		case APPLICATION_2_MAC_SETRADIOTXPOWER: 	case APPLICATION_2_MAC_SETRADIOTXMODE:
		case APPLICATION_2_MAC_SETRADIOSLEEP: 		case APPLICATION_2_MAC_SETRADIOLISTEN:
		case APPLICATION_2_MAC_SETDUTYCYCLE: 		case APPLICATION_2_MAC_SETLISTENINTERVAL:
		case APPLICATION_2_MAC_SETBEACONINTERVALFRACTION: case APPLICATION_2_MAC_SETPROBTX:
		case APPLICATION_2_MAC_SETNUMTX: 		case APPLICATION_2_MAC_SETRNDTXOFFSET:
		case APPLICATION_2_MAC_SETRETXINTERVAL: 	case APPLICATION_2_MAC_SETBACKOFFTYPE:
		case APPLICATION_2_MAC_SETBACKOFFBASEVALUE: case APPLICATION_2_MAC_SETCARRIERSENSE:
		{
			App_ControlMessage *appCtrlMsg = check_and_cast<App_ControlMessage*>(msg);
			App_ControlMessage *dupAppCtrlMsg = (App_ControlMessage *) appCtrlMsg->dup();
			send(dupAppCtrlMsg, "toMacModule");
			break;
		}
		
		default:
		{
			CASTALIA_DEBUG << "\n[Network_"<< self <<"] t= " << simTime() << ": WARNING: received packet of unknown type\n";
			break;
		}

	}//end_of_switch

	delete msg;
	
	msg = NULL;		// safeguard
}



void BypassRoutingModule::finish()
{	
	Network_GenericFrame *netMsg;

	while(!BUFFER_IS_EMPTY)
	{
		netMsg = popTxBuffer();

		cancelAndDelete(netMsg);

	  	netMsg = NULL;
	}
	
	delete [] schedTXBuffer;
}


void BypassRoutingModule::readIniFileParameters(void)
{
	printDebugInfo = par("printDebugInfo");

	maxNetFrameSize = par("maxNetFrameSize");

	netDataFrameOverhead = par("netDataFrameOverhead");

	netBufferSize = par("netBufferSize");
}



int BypassRoutingModule::encapsulateAppPacket(App_GenericDataPacket *appPacket, Network_GenericFrame *retFrame)
{
	// Set the ByteLength of the frame 
	int totalMsgLen = appPacket->byteLength() + netDataFrameOverhead; // the byte-size overhead for a Data packet is fixed (always netDataFrameOverhead)
	if(totalMsgLen > maxNetFrameSize)
		return 0;
	retFrame->setByteLength(netDataFrameOverhead); // extra bytes will be added after the encapsulation
	App_GenericDataPacket *dupAppPacket = check_and_cast<App_GenericDataPacket *>(appPacket->dup());
	retFrame->encapsulate(dupAppPacket);
	
	
	// --- Set the Simulation-Specific fields of Network_GenericFrame 
	retFrame->setRssi(0.0);
	retFrame->setCurrentPathFromSource("");
	
	
	// --- Set the Network_GenericFrame -> header  fields 
	retFrame->getHeader().frameType = (unsigned short)NET_PROTOCOL_DATA_FRAME; // Set the frameType field
	retFrame->getHeader().source = appPacket->getHeader().source;
	retFrame->getHeader().destinationCtrl = appPacket->getHeader().destination; // simply copy the destination of the App_GenericDataPacket
	retFrame->getHeader().lastHop = strSelfID.c_str();
	retFrame->getHeader().nextHop = ""; // next-hop is handled outside this function
	// No ttl is used (retFrame->getHeader().ttl = ??? )

	return 1;
}



int BypassRoutingModule::pushBuffer(Network_GenericFrame *theFrame)
{
	if(theFrame == NULL)
	{
		CASTALIA_DEBUG << "\n[Network_" << self << "] t= " << simTime() << ": WARNING: Trying to push NULL Network_GenericFrame to the Net_Buffer!!\n";
		return 0;
	}

	tailTxBuffer = (++tailTxBuffer)%(NET_BUFFER_ARRAY_SIZE); //increment the tailTxBuffer pointer. If reached the end of array, then start from position [0] of the array

	if (tailTxBuffer == headTxBuffer)
	{
		// reset tail pointer
		if(tailTxBuffer == 0)
			tailTxBuffer = NET_BUFFER_ARRAY_SIZE-1;
		else
			tailTxBuffer--;
		CASTALIA_DEBUG << "\n[Network_" << self << "] t= " << simTime() << ": WARNING: SchedTxBuffer FULL!!! value to be Tx not added to buffer\n";
		return 0;
	}

	theFrame->setKind(NETWORK_FRAME);

	if (tailTxBuffer==0)
		schedTXBuffer[NET_BUFFER_ARRAY_SIZE-1] = theFrame;
	else
		schedTXBuffer[tailTxBuffer-1] = theFrame;


	int currLen = getTXBufferSize();
	if (currLen > maxSchedTXBufferSizeRecorded)
		maxSchedTXBufferSizeRecorded = currLen;

	return 1;
}


Network_GenericFrame* BypassRoutingModule::popTxBuffer(void)
{

	if (tailTxBuffer == headTxBuffer) {
		CASTALIA_DEBUG << "\n[Network_" << self << "] \nTrying to pop  EMPTY TxBuffer!!";
		tailTxBuffer--;  // reset tail pointer
		return NULL;
	}

	Network_GenericFrame *pop_message = schedTXBuffer[headTxBuffer];

	headTxBuffer = (++headTxBuffer)%(NET_BUFFER_ARRAY_SIZE);

	return pop_message;
}


int BypassRoutingModule::getTXBufferSize(void)
{
	int size = tailTxBuffer - headTxBuffer;
	if ( size < 0 )
		size += NET_BUFFER_ARRAY_SIZE;

	return size;
}
