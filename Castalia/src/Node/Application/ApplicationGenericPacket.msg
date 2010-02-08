//***************************************************************************************
//*  Copyright: National ICT Australia  2010						*
//*  Developed at the Networks and Pervasive Computing program				*
//*  Author(s): Yuriy Tselishchev							*
//*  This file is distributed under the terms in the attached LICENSE file.		*
//*  If you do not find this file, copies can be found by writing to:			*
//*											*
//*      NICTA, Locked Bag 9013, Alexandria, NSW 1435, Australia			*
//*      Attention:  License Inquiry.							*
//*											*
//***************************************************************************************


struct ApplicationInteractionControl_type {
    string applicationID;
    string source;
    string destination;
    double RSSI;
    double LQI;
    simtime_t timestamp;
}

packet ApplicationGenericDataPacket
{
    ApplicationInteractionControl_type applicationInteractionControl;
    double data;
    int sequenceNumber;
}
