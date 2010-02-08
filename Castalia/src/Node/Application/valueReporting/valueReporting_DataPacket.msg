//***************************************************************************************
//*  Copyright: National ICT Australia, 2007 - 2010					*
//*  Developed at the Networks and Pervasive Computing program				*
//*  Author(s): Dimosthenis Pediaditakis, Yuriy Tselishchev				*
//*  This file is distributed under the terms in the attached LICENSE file.		*
//*  If you do not find this file, copies can be found by writing to:			*
//*											*
//*      NICTA, Locked Bag 9013, Alexandria, NSW 1435, Australia			*
//*      Attention:  License Inquiry.							*
//*											*
//***************************************************************************************

cplusplus {{
#include "ApplicationGenericPacket_m.h"
}}

class ApplicationGenericDataPacket;

struct valueReportData {
    unsigned short nodeID;	//the ID of the Node
    double locX;		// x-coordinate of the node
    double locY;		// y-coordinate of the node
}


message valueReporting_DataPacket extends ApplicationGenericDataPacket {
    valueReportData extraData;
}
