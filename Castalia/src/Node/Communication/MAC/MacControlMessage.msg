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




enum MAC_ContorlMessageType
{
	//change state directive MAC-->Radio
	MAC_2_RADIO_ENTER_SLEEP = 50;
	MAC_2_RADIO_ENTER_LISTEN = 51;
	MAC_2_RADIO_ENTER_TX = 52;

	// choose a mode with respect to carrier sensing when sending a packet
	MAC_2_RADIO_CHANGE_TX_MODE = 53;

	//change power Level of the Radio
	MAC_2_RADIO_CHANGE_POWER_LEVEL = 54;

	//directive MAC-->Radio to perform carrier sense
	MAC_2_RADIO_SENSE_CARRIER = 55;

	//self messages MAC-->MAC
	MAC_SELF_CHECK_TX_BUFFER = 57;
	MAC_SELF_EXIT_EXPECTING_RX = 58;
	MAC_SELF_PERFORM_CARRIER_SENSE = 59; 
	MAC_SELF_EXIT_CARRIER_SENSE = 60;
	MAC_SELF_SET_RADIO_SLEEP = 61;
	MAC_SELF_WAKEUP_RADIO = 62;
	MAC_SELF_INITIATE_TX = 63;
};



message MAC_ControlMessage
{
    fields:
	double sense_carrier_interval;
	int radioTxMode;
	int powerLevel;
};
