//***************************************************************************************
//*  Copyright: National ICT Australia,  2007, 2008					*
//*  Developed at the Networks and Pervasive Computing program				*
//*  Author(s): Athanassios Boulis, Dimosthenis Pediaditakis				*
//*  This file is distributed under the terms in the attached LICENSE file.		*
//*  If you do not find this file, copies can be found by writing to:			*
//*											*
//*      NICTA, Locked Bag 9013, Alexandria, NSW 1435, Australia			*
//*      Attention:  License Inquiry.							*
//*											*
//***************************************************************************************



#ifndef _TUNABLEMACCONTROLIMPL

#define _TUNABLEMACCONTROLIMPL


void valueReporting_ApplicationModule::setMac_DutyCycle(double parDutyCycle, double delay)
{	
	if( (parDutyCycle <= 0.0) || (parDutyCycle > 1.0) )
	{
		CASTALIA_DEBUG << "\n[Application_" << self <<"]:\nWarning! Application module tried to set an invalid duty cycle to the MAC module\n";
	}
	else
	{
		App_ControlMessage *appCtrlMsg;
		appCtrlMsg = new App_ControlMessage("app 2 MAC: Set duty cycle strobe", APPLICATION_2_MAC_SETDUTYCYCLE);
		appCtrlMsg->setMacDutyCycle(parDutyCycle);
	
		if(delay == 0.0)
			send(appCtrlMsg, "toCommunicationModule");
		else
			sendDelayed(appCtrlMsg, delay, "toCommunicationModule");
	}
}


void valueReporting_ApplicationModule::setMac_ListenInterval(double parInterval /* in msec */, double delay)
{
	if(parInterval < 0)
	{
		CASTALIA_DEBUG << "\n[Application_" << self <<"]:\nWarning! Application module tried to set an invalid ListenInterval to the MAC module\n";
	}
	else
	{
		App_ControlMessage *appCtrlMsg;
		appCtrlMsg = new App_ControlMessage("app 2 MAC: Set ListenInterval strobe", APPLICATION_2_MAC_SETLISTENINTERVAL);
		appCtrlMsg->setMacListenInterval(parInterval);
	
		if(delay == 0.0)
			send(appCtrlMsg, "toCommunicationModule");
		else
			sendDelayed(appCtrlMsg, delay, "toCommunicationModule");
	}
}


void valueReporting_ApplicationModule::setMac_BeaconIntervalFraction(double parInterval, double delay)
{
	if( (parInterval < 0.0) || (parInterval > 1.0) )
	{
		CASTALIA_DEBUG << "\n[Application_" << self <<"]:\nWarning! Application module tried to set an invalid BeaconInterval to the MAC module\n";
	}
	else
	{
		App_ControlMessage *appCtrlMsg;
		appCtrlMsg = new App_ControlMessage("app 2 MAC: Set BeaconInterval strobe", APPLICATION_2_MAC_SETBEACONINTERVALFRACTION);
		appCtrlMsg->setMacBeaconIntervalFraction(parInterval);
	
		if(delay == 0.0)
			send(appCtrlMsg, "toCommunicationModule");
		else
			sendDelayed(appCtrlMsg, delay, "toCommunicationModule");
	}
}


void valueReporting_ApplicationModule::setMac_ProbTX(double parProb, double delay)
{
	if( (parProb < 0.0) || (parProb > 1.0) )
	{
		CASTALIA_DEBUG << "\n[Application_" << self <<"]:\nWarning! Application module tried to set an invalid ProbTX to the MAC module\n";
	}
	else
	{
		App_ControlMessage *appCtrlMsg;
		appCtrlMsg = new App_ControlMessage("app 2 MAC: Set ProbTX strobe", APPLICATION_2_MAC_SETPROBTX);
		appCtrlMsg->setMacProbTX(parProb);
	
		if(delay == 0.0)
			send(appCtrlMsg, "toCommunicationModule");
		else
			sendDelayed(appCtrlMsg, delay, "toCommunicationModule");
	}
}


void valueReporting_ApplicationModule::setMac_NumTX(int parNum, double delay)
{
	if(parNum < 0)
	{
		CASTALIA_DEBUG << "\n[Application_" << self <<"]:\nWarning! Application module tried to set an invalid NumTX to the MAC module\n";
	}
	else
	{
		App_ControlMessage *appCtrlMsg;
		appCtrlMsg = new App_ControlMessage("app 2 MAC: Set NumTX strobe", APPLICATION_2_MAC_SETNUMTX);
		appCtrlMsg->setMacNumTX(parNum);
	
		if(delay == 0.0)
			send(appCtrlMsg, "toCommunicationModule");
		else
			sendDelayed(appCtrlMsg, delay, "toCommunicationModule");
	}
}


void valueReporting_ApplicationModule::setMac_RndTXOffset(double parOffset /* in msec */, double delay)
{
	if(parOffset <= 0.0)
	{
		CASTALIA_DEBUG << "\n[Application_" << self <<"]:\nWarning! Application module tried to set an invalid randomTxOffset to the MAC module\n";
	}
	else
	{
		App_ControlMessage *appCtrlMsg;
		appCtrlMsg = new App_ControlMessage("app 2 MAC: Set randomTxOffset strobe", APPLICATION_2_MAC_SETRNDTXOFFSET);
		appCtrlMsg->setMacRndTXOffset(parOffset);
	
		if(delay == 0.0)
			send(appCtrlMsg, "toCommunicationModule");
		else
			sendDelayed(appCtrlMsg, delay, "toCommunicationModule");
	}
}


void valueReporting_ApplicationModule::setMac_ReTXInterval(double parInterval /* in msec */, double delay)
{
	if(parInterval <= 0.0)
	{
		CASTALIA_DEBUG << "\n[Application_" << self <<"]:\nWarning! Application module tried to set an invalid reTxInterval to the MAC module\n";
	}
	else
	{
		App_ControlMessage *appCtrlMsg;
		appCtrlMsg = new App_ControlMessage("app 2 MAC: Set reTxInterval strobe", APPLICATION_2_MAC_SETRETXINTERVAL);
		appCtrlMsg->setMacReTXInterval(parInterval);
	
		if(delay == 0.0)
			send(appCtrlMsg, "toCommunicationModule");
		else
			sendDelayed(appCtrlMsg, delay, "toCommunicationModule");
	}
}


/*
 * Values for parType:
 * (0) --> BACKOFF_SLEEP_INT
 * (1) --> BACKOFF_CONSTANT
 * (2) --> BACKOFF_MULTIPLYING
 * (3) --> BACKOFF_EXPONENTIAL
 */
void valueReporting_ApplicationModule::setMac_BackoffType(int parType, double delay)
{
	if( (parType != 0) && (parType != 1) && (parType != 2) && (parType != 3) )
	{
		CASTALIA_DEBUG << "\n[Application_" << self <<"]:\nWarning! Application module tried to set an invalid backoffType to the MAC module\n";
	}
	else
	{
		App_ControlMessage *appCtrlMsg;
		appCtrlMsg = new App_ControlMessage("app 2 MAC: Set backoffType strobe", APPLICATION_2_MAC_SETBACKOFFTYPE);
		appCtrlMsg->setMacBackoffType(parType);
	
		if(delay == 0.0)
			send(appCtrlMsg, "toCommunicationModule");
		else
			sendDelayed(appCtrlMsg, delay, "toCommunicationModule");
	}
}


void valueReporting_ApplicationModule::setMac_BackoffBaseValue(double parValue /* in msec */, double delay)
{
	if(parValue <= 0)
	{
		CASTALIA_DEBUG << "\n[Application_" << self <<"]:\nWarning! Application module tried to set an invalid backoffBaseValue to the MAC module\n";
	}
	else
	{
		App_ControlMessage *appCtrlMsg;
		appCtrlMsg = new App_ControlMessage("app 2 MAC: Set backoffBaseValue strobe", APPLICATION_2_MAC_SETBACKOFFBASEVALUE);
		appCtrlMsg->setMacBackoffBaseValue(parValue);
	
		if(delay == 0.0)
			send(appCtrlMsg, "toCommunicationModule");
		else
			sendDelayed(appCtrlMsg, delay, "toCommunicationModule");
	}
}


void valueReporting_ApplicationModule::setMac_UsingCarrierSense(bool parValue, double delay)
{
	App_ControlMessage *appCtrlMsg;
	appCtrlMsg = new App_ControlMessage("app 2 MAC: Set usingCarrierSense  strobe", APPLICATION_2_MAC_SETCARRIERSENSE);
	appCtrlMsg->setMacUsingCarrierSense(parValue);

	if(delay == 0.0)
		send(appCtrlMsg, "toCommunicationModule");
	else
		sendDelayed(appCtrlMsg, delay, "toCommunicationModule");
}


#endif //_TUNABLEMACCONTROLIMPL
