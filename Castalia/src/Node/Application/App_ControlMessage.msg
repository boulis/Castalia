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



enum App_ControlMessageType
{
	// Message types used by the Application module in order to change 
	// the behaviour of MAC or Radio module
	APPLICATION_2_MAC_SETRADIOTXPOWER = 1001;
	APPLICATION_2_MAC_SETRADIOTXMODE = 1002;
	APPLICATION_2_MAC_SETRADIOSLEEP = 1003;
	APPLICATION_2_MAC_SETRADIOLISTEN = 1004;
	
	APPLICATION_2_MAC_SETDUTYCYCLE = 1005;
	APPLICATION_2_MAC_SETLISTENINTERVAL = 1006;
	APPLICATION_2_MAC_SETBEACONINTERVALFRACTION = 1007;
	APPLICATION_2_MAC_SETPROBTX = 1008;
	APPLICATION_2_MAC_SETNUMTX = 1009;
	APPLICATION_2_MAC_SETRNDTXOFFSET = 1010;
	APPLICATION_2_MAC_SETRETXINTERVAL = 1011;
	APPLICATION_2_MAC_SETBACKOFFTYPE = 1012;
	APPLICATION_2_MAC_SETBACKOFFBASEVALUE = 1013;
	APPLICATION_2_MAC_SETCARRIERSENSE = 1014;

	APP_NODE_STARTUP = 1015;
	APP_SELF_REQUEST_SAMPLE = 1016;

	APP_TIMER_1 = 1017;
	APP_TIMER_2 = 1018;
	APP_TIMER_3 = 1019;
	APP_TIMER_4 = 1020;
	APP_TIMER_5 = 1021;
	APP_TIMER_6 = 1022;
	APP_TIMER_7 = 1023;
	APP_TIMER_8 = 1024;

};


message App_ControlMessage
{
    fields:
		int radioTXPowerLevel;
		int radioTXMode;
		
		double macDutyCycle;
		double macListenInterval;
		double macBeaconIntervalFraction;
		double macProbTX;
		int macNumTX;
		double macRndTXOffset;
		double macReTXInterval;
		int macBackoffType;
		double macBackoffBaseValue;
		bool macUsingCarrierSense;
};