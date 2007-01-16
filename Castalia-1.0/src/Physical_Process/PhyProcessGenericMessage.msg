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


enum PhyProcess_GenericMessageType
{	
	PHY_SAMPLE_REQ = 10;
	PHY_SAMPLE_REPLY = 11;
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
