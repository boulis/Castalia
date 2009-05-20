// **************************************************************************************
// *  Copyright: National ICT Australia,  2006 - 2009					*
// *  Developed at the Networks and Pervasive Computing program				*
// *  Author(s): Athanassios Boulis, Dimosthenis Pediaditakis				*
// *  This file is distributed under the terms in the attached LICENSE file.		*
// *  If you do not find this file, copies can be found by writing to:			*
// *											*
// *      NICTA, Locked Bag 9013, Alexandria, NSW 1435, Australia			*
// *      Attention:  License Inquiry.							*
// *			CASTALIA 2.0							*
// **************************************************************************************/




enum MAC_ControlMessageType
{
	//change state directive MAC-->Radio
	MAC_2_RADIO_ENTER_SLEEP = 2000;
	MAC_2_RADIO_ENTER_LISTEN = 2001;
	MAC_2_RADIO_ENTER_TX = 2002;

	// choose a mode with respect to carrier sensing when sending a packet
	MAC_2_RADIO_CHANGE_TX_MODE = 2003;

	//change power Level of the Radio
	MAC_2_RADIO_CHANGE_POWER_LEVEL = 2004;

	//directive MAC-->Radio to perform carrier sense
	MAC_2_RADIO_SENSE_CARRIER = 2005;
	MAC_2_RADIO_SENSE_CARRIER_INSTANTANEOUS = 2015;	

	//self messages MAC-->MAC
	MAC_SELF_CHECK_TX_BUFFER = 2006;
	MAC_SELF_EXIT_EXPECTING_RX = 2007;
	MAC_SELF_PERFORM_CARRIER_SENSE = 2008; 
	MAC_SELF_EXIT_CARRIER_SENSE = 2009;
	MAC_SELF_SET_RADIO_SLEEP = 2010;
	MAC_SELF_WAKEUP_RADIO = 2011;
	MAC_SELF_INITIATE_TX = 2012;
	MAC_SELF_POP_BUFFER_AND_SEND_DATA = 2013;

	MAC_2_NETWORK_FULL_BUFFER = 2014;
	
	
	//TMAC specific control msg
	MAC_SELF_SYNC_SETUP 	  = 201;
	MAC_SELF_SYNC_CREATE	  = 202;
	MAC_SELF_SYNC_RENEW	  = 203;
	MAC_SELF_FRAME_START 	  = 204;
	MAC_SELF_WAKEUP_PASSIVE   = 205;
	
	MAC_TIMER_NAV        = 210;
	MAC_TIMER_WFCTS	     = 211;
	MAC_TIMER_WFDATA     = 212;
	MAC_TIMER_WFACK      = 213;	
};



message MAC_ControlMessage
{
    fields:
	double sense_carrier_interval;
	int radioTxMode;
	int powerLevel;
};
