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



enum PhyProcess_GenericMessageType
{	
	PHY_SAMPLE_REQ = 7000;

	PHY_SAMPLE_REPLY = 7001;
	
	PHY_PROCESS_TIMER_1 = 7002;

	PHY_PROCESS_DESTROY_NODE = 7003;
};


message PhyProcess_GenericMessage
{
    fields:
		double value;
		double xCoor;
		double yCoor;
		int srcID; //the ID of the node module that sends the Sample request
		int sensorIndex;
};
