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





enum Radio_ContorlMessageType
{
	RADIO_SELF_CHECK_BUFFER = 4000;
	RADIO_SELF_VALIDATE_CS = 4001;
	RADIO_SELF_ENTER_TX_NOW = 4002;
	RADIO_SELF_ENTER_SLEEP_NOW = 4003;
	RADIO_SELF_ENTER_LISTEN_NOW = 4004;

	RADIO_2_MAC_SENSED_CARRIER = 4005;
	RADIO_2_MAC_FULL_BUFFER = 4006;
	RADIO_2_MAC_STARTED_TX = 4007;
	RADIO_2_MAC_STOPPED_TX = 4008;
	RADIO_2_MAC_NO_SENSED_CARRIER = 4009;
};


// For future per frame basis Carrier Sense specifications from MAC to radio module
enum Radio_TxMode
{
	CARRIER_SENSE_NONE = 4009;  	//forced send
	CARRIER_SENSE_ONCE_CHECK = 4010;	//sense once the medium and if free send else drop
	CARRIER_SENSE_PERSISTENT = 4011;	//sense the medium until it is free
};


message Radio_ControlMessage
{
    fields:
	int durationOfTX;
	int lengthOfSentData;
};
