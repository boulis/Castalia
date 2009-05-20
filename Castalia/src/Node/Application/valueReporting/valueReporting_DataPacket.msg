//***************************************************************************************
//*  Copyright: Athens Information Technology (AIT),  2007, 2008, 2009			*
//*		http://www.ait.gr							*
//*             Developed at the Broadband Wireless and Sensor Networks group (B-WiSe) 	*
//*		http://www.ait.edu.gr/research/Wireless_and_Sensors/overview.asp	*
//*											*
//*  Author(s): Dimosthenis Pediaditakis						*
//*											*
//*  This file is distributed under the terms in the attached LICENSE file.		*
//*  If you do not find this file, copies can be found by writing to:			*
//*											*
//*      NICTA, Locked Bag 9013, Alexandria, NSW 1435, Australia			*
//*      Attention:  License Inquiry.							*
//**************************************************************************************/


cplusplus {{
#include "App_GenericDataPacket_m.h"
}};

class App_GenericDataPacket;


struct valueReportData
{
	fields:
		unsigned short nodeID;  //the ID of the Node
		double locX;  // x-coordinate of the node
		double locY;  // y-coordinate of the node
		
		int sampleSN; //sample Sequence number
		double value; //the reported value
};


message valueReporting_DataPacket extends App_GenericDataPacket
{
    fields:
       valueReportData  data;
       
       // simulation-specific fields
       simtime_t sentTstamp;
};
