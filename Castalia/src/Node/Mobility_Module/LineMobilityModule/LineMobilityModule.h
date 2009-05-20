/***************************************************************************
 *  Copyright: National ICT Australia,  2009
 *  Developed at the ATP lab, Networked Systems theme
 *  Author(s): Yuri Tselishchev
 *  This file is distributed under the terms in the attached LICENSE file.
 *  If you do not find this file, copies can be found by writing to:
 *
 *      NICTA, Locked Bag 9013, Alexandria, NSW 1435, Australia
 *      Attention:  License Inquiry.
 ***************************************************************************/

#ifndef _MOBILITYMODULE_H_
#define _MOBILITYMODULE_H_

#include <omnetpp.h>
#include "MobilityModule_Message_m.h"
#include "VirtualMobilityModule.h"
#include "DebugInfoWriter.h"

using namespace std;

class LineMobilityModule : public VirtualMobilityModule 
{
	private:
	/*--- The .ned file's parameters ---*/
	    double updateInterval;
	    double loc1_x;
	    double loc1_y;
	    double loc1_z;
	    double loc2_x;
	    double loc2_y;
	    double loc2_z;
	    double speed;
	    	
	/*--- Custom class parameters ---*/
	    double incr_x;
	    double incr_y;
	    double incr_z;
	    double distance;
	    int direction;
		
	protected:
	    virtual void initialize();
	    virtual void handleMessage(cMessage *msg);
	    virtual void finish();
};

#endif 
