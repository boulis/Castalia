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



#ifndef _RADIOCONTROLIMPL

#define _RADIOCONTROLIMPL

void BridgeTest_ApplicationModule::setRadio_TXPowerLevel(int parLevel, double delay)
{
	if(parLevel < 0)
	{
		CASTALIA_DEBUG << "\n[Application_" << self <<"]:\nWarning! Application module tried to set radio transmitting at an invalid TX power level\n";
	}
	else
	{
		App_ControlMessage *appCtrlMsg;
		appCtrlMsg = new App_ControlMessage("app 2 radio (through MAC): Change TX power strobe", APPLICATION_2_MAC_SETRADIOTXPOWER);
		appCtrlMsg->setRadioTXPowerLevel(parLevel);
	
		if(delay == 0.0)
			send(appCtrlMsg, "toCommunicationModule");
		else
			sendDelayed(appCtrlMsg, delay, "toCommunicationModule");
	}
}


/*
   a) 0 for CARRIER_SENSE_NONE (Only that mode is supported by the current implementation of radio),
   b) 1 for CARRIER_SENSE_ONCE_CHECK, 
   c) 2 for CARRIER_SENSE_PERSISTENT
*/
void BridgeTest_ApplicationModule::setRadio_TXMode(int parMode, double delay)
{
	if( (parMode < 0) || (parMode > 2) )
	{
		CASTALIA_DEBUG << "\n[Application_" << self <<"]:\nWarning! Application module tried to set radio into an invalid TX mode level\n";
	}
	else
	{
		App_ControlMessage *appCtrlMsg;
		appCtrlMsg = new App_ControlMessage("app 2 radio (through MAC): Change TX mode strobe", APPLICATION_2_MAC_SETRADIOTXMODE);
		appCtrlMsg->setRadioTXMode(parMode);
	
		if(delay == 0.0)
			send(appCtrlMsg, "toCommunicationModule");
		else
			sendDelayed(appCtrlMsg, delay, "toCommunicationModule");
	}
}


void BridgeTest_ApplicationModule::setRadio_Sleep(double delay)
{
	App_ControlMessage *appCtrlMsg;
	appCtrlMsg = new App_ControlMessage("app 2 radio (through MAC): Enter sleep state", APPLICATION_2_MAC_SETRADIOSLEEP);
	
	if(delay == 0.0)
		send(appCtrlMsg, "toCommunicationModule");
	else
		sendDelayed(appCtrlMsg, delay, "toCommunicationModule");
}


void BridgeTest_ApplicationModule::setRadio_Listen(double delay)
{
	App_ControlMessage *appCtrlMsg;
	appCtrlMsg = new App_ControlMessage("app 2 radio (through MAC): Enter listening state", APPLICATION_2_MAC_SETRADIOLISTEN);
	
	if(delay == 0.0)
		send(appCtrlMsg, "toCommunicationModule");
	else
		sendDelayed(appCtrlMsg, delay, "toCommunicationModule");
}

#endif //_RADIOCONTROLIMPL
