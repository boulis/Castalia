// **************************************************************************************
// *  Copyright: National ICT Australia,  2006											*
// *  Developed at the Networks and Pervasive Computing program							*
// *  Author(s): Athanassios Boulis, Dimosthenis Pediaditakis							*
// *  This file is distributed under the terms in the attached LICENSE file.			*
// *  If you do not find this file, copies can be found by writing to:					*
// *																					*
// *      NICTA, Locked Bag 9013, Alexandria, NSW 1435, Australia						*
// *      Attention:  License Inquiry.													*
// *																					*
// **************************************************************************************/




enum Radio_ContorlMessageType
{
	RADIO_SELF_CHECK_BUFFER = 30;
	RADIO_SELF_VALIDATE_CS = 31;
	RADIO_SELF_ENTER_TX_NOW = 32;
	RADIO_SELF_ENTER_SLEEP_NOW = 33;
	RADIO_SELF_ENTER_LISTEN_NOW = 34;

	RADIO_2_MAC_SENSED_CARRIER = 35;
	RADIO_2_MAC_FULL_BUFFER = 36;
};


// For future per frame basis Carrier Sense specifications from MAC to radio module
enum Radio_TxMode
{
	CARRIER_SENSE_NONE = 38;  	//forced send
	CARRIER_SENSE_ONCE_CHECK = 39;	//sense once the medium and if free send else drop
	CARRIER_SENSE_PERSISTENT = 40;	//sense the medium until it is free
};


message Radio_ControlMessage
{
    fields:
	int durationOfTX;
	int lengthOfSentData;
};
