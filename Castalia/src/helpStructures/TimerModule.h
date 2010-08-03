//***************************************************************************************
//*  Copyright: National ICT Australia, 2010                                            *
//*  Developed at the Networks and Pervasive Computing program                          *
//*  Author(s): Yuriy Tselishchev                                                       *
//*  This file is distributed under the terms in the attached LICENSE file.             *
//*  If you do not find this file, copies can be found by writing to:                   *
//*                                                                                     *
//*      NICTA, Locked Bag 9013, Alexandria, NSW 1435, Australia                        *
//*      Attention:  License Inquiry.                                                   *
//*                                                                                     *
//***************************************************************************************

#ifndef TIMERMODULE
#define TIMERMODULE

#include <omnetpp.h>
#include <map>

#include "CastaliaMessages.h"
#include "TimerServiceMessage_m.h"

class TimerModule:public virtual cSimpleModule {
 private:
	double timerDrift;
 protected:
	std::map<int,TimerServiceMessage*>timerMessages;

	simtime_t getClock();
	void setTimerDrift(double new_drift);
	void setTimer(int index, simtime_t time);
	void cancelTimer(int index);
	void handleTimerMessage(cMessage *);
	virtual void timerFiredCallback(int index);
};

#endif
