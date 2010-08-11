/****************************************************************************
 *  Copyright: National ICT Australia,  2007 - 2010                         *
 *  Developed at the ATP lab, Networked Systems research theme              *
 *  Author(s): Yuriy Tselishchev                                            *
 *  This file is distributed under the terms in the attached LICENSE file.  *
 *  If you do not find this file, copies can be found by writing to:        *
 *                                                                          *
 *      NICTA, Locked Bag 9013, Alexandria, NSW 1435, Australia             *
 *      Attention:  License Inquiry.                                        *
 *                                                                          *  
 ****************************************************************************/

#include "TimerService.h"

void TimerService::setTimerDrift(double new_drift)
{
	timerDrift = new_drift;
}

void TimerService::timerFiredCallback(int timerIndex)
{
}

void TimerService::cancelTimer(int timerIndex)
{
	std::map<int,TimerServiceMessage*>::iterator iter = timerMessages.find(timerIndex);
	if (iter != timerMessages.end()) {
		if (timerMessages[timerIndex] != NULL && timerMessages[timerIndex]->isScheduled()) {
			cancelAndDelete(timerMessages[timerIndex]);
		}
		timerMessages.erase(iter);
	}
}

void TimerService::setTimer(int timerIndex, simtime_t time)
{
	cancelTimer(timerIndex);
	timerMessages[timerIndex] = new TimerServiceMessage("Timer message", TIMER_SERVICE);
	timerMessages[timerIndex]->setTimerIndex(timerIndex);
	scheduleAt(simTime() + timerDrift * time, timerMessages[timerIndex]);
}

void TimerService::handleTimerMessage(cMessage * msg)
{
	int msgKind = (int)msg->getKind();
	if (msgKind == TIMER_SERVICE) {
		TimerServiceMessage *timerMsg = check_and_cast<TimerServiceMessage*>(msg);
		int timerIndex = timerMsg->getTimerIndex();
		std::map<int,TimerServiceMessage*>::iterator iter = timerMessages.find(timerIndex);
		if (iter != timerMessages.end()) {
			timerMessages.erase(iter);
			timerFiredCallback(timerIndex);
		}
	}
}

simtime_t TimerService::getClock()
{
	return simTime() * timerDrift;
}

