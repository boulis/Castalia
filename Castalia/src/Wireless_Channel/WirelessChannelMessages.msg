//*************************************************************************
//*  Copyright: National ICT Australia,  2007, 2008, 2009
//*  Developed at the ATP lab, Networked Systems theme
//*  Author(s): Athanassios Boulis, Dimosthenis Pediaditakis
//*  This file is distributed under the terms in the attached LICENSE file.
//*  If you do not find this file, copies can be found by writing to:
//*
//*      NICTA, Locked Bag 9013, Alexandria, NSW 1435, Australia
//*      Attention:  License Inquiry.
//*************************************************************************/


enum WChannel_MessageTypes
{
	WC_CARRIER_SENSE_BEGIN = 8000;
	WC_CARRIER_SENSE_END = 8001;
	WC_CARRIER_SENSED = 8002;
	WC_PKT_BEGIN_TX = 8003;
	WC_PKT_END_TX = 8004;
	WC_SIMTIME_LIMIT = 8005;
	WC_CARRIER_SENSE_INSTANTANEOUS = 8006;
	WC_NO_CARRIER_SENSED = 8007;
	WC_NODE_MOVEMENT = 8008;
};


message WChannel_GenericMessage
{
    fields:
        int srcAddress;
	double rssi;
	double txPower_dB;
};

message WChannel_NodeMoveMessage extends WChannel_GenericMessage
{
    fields:
        double x;
        double y;
        double z;
        double phi;
        double theta;
};
