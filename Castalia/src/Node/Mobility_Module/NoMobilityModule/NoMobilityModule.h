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
#include "DebugInfoWriter.h"
#include "VirtualMobilityModule.h"

using namespace std;

class NoMobilityModule : public VirtualMobilityModule
{
	protected:
	    virtual void initialize();
	    virtual void handleMessage(cMessage *msg);
	    virtual void finish();
};

#endif 
