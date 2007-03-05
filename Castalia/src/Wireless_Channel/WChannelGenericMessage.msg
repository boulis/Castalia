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




enum WChannel_GenericMessageType
{
	WC_CARRIER_SENSE_BEGIN = 1;
	WC_CARRIER_SENSE_END = 2;
	WC_CARRIER_SENSED = 3;
	WC_PKT_BEGIN_TX = 4;
	WC_PKT_END_TX = 5;
};


message WChannel_GenericMessage
{
    fields:
        int srcAddress;
        int destAddress;
        int powerLevel;
};
