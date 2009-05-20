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



enum SensorDevMgr_GenericMessageType
{
	SDM_2_APP_SENSOR_READING = 6000;
	APP_2_SDM_SAMPLE_REQUEST = 6001;
	SDM_2_APP_KILL_NODE = 6002;
}

message SensorDevMgr_GenericMessage
{
    fields:
	double sensedValue;
	string sensorType;
	int sensorIndex;
};
