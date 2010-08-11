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

#include "CastaliaModule.h"

#define CASTALIA_PREFIX "Castalia|\t"

void CastaliaModule::finish()
{
	finishSpecific();
	if (simpleoutputs.size() == 0 && histograms.size() == 0)
		return;
	bool header = true;

	simpleOutputMapType::iterator i;
	for (i = simpleoutputs.begin(); i != simpleoutputs.end(); i++) {
		outputKeyDef key = i->first;
		simpleOutputTypeDef out = i->second;
		if (out.data.size() == 0)
			continue;
		if (header) {
			EV << CASTALIA_PREFIX << "module:" << getFullPath() << endl;
			header = false;
		}
		EV << CASTALIA_PREFIX << "\t";
		if (key.index >= 0)
			EV << " index:" << key.index << " ";
		EV << "simple output name:" << key.descr << endl;
		map < string, double >::iterator i2;
		for (i2 = out.data.begin(); i2 != out.data.end(); i2++)
			EV << CASTALIA_PREFIX "\t\t" << i2->second << " " << i2->first << endl;
	}
	simpleoutputs.clear();

	histogramOutputMapType::iterator i3;
	for (i3 = histograms.begin(); i3 != histograms.end(); i3++) {
		outputKeyDef key = i3->first;
		histogramOutputTypeDef hist = i3->second;
		if (!hist.active)
			continue;
		if (header) {
			EV << CASTALIA_PREFIX << "module:" << getFullPath() << endl;
			header = false;
		}
		EV << CASTALIA_PREFIX << "\t";
		if (key.index >= 0)
			EV << " index:" << key.index << " ";
		EV << "histogram name:" << key.descr << endl;
		EV << CASTALIA_PREFIX << "\thistogram_min:" << hist.min <<
			" histogram_max:" << hist.max << endl;
		EV << CASTALIA_PREFIX << "\thistogram_values";
		for (int i = 0; i <= hist.numBuckets; i++)
			EV << " " << hist.buckets[i];
		EV << endl;
	}
	histograms.clear();
}

std::ostream & CastaliaModule::trace()
{
	if (hasPar("collectTraceInfo") && par("collectTraceInfo")) {
		return (ostream &) DebugInfoWriter::getStream() <<
			"\n" << setw(16) << simTime() << setw(40) << getFullPath() << " ";
	} else {
		return empty;
	}
}

std::ostream & CastaliaModule::debug()
{
	return cerr;
}

void CastaliaModule::declareOutput(const char *descr)
{
	outputKeyDef key(descr, -1);
	simpleoutputs[key].data.clear();
}

void CastaliaModule::declareOutput(const char *descr, int index)
{
	if (index < 0)
		opp_error("Negative output index not permitted");
	outputKeyDef key(descr, index);
	simpleoutputs[key].data.clear();
}

void CastaliaModule::collectOutput(const char *descr, int index)
{
	if (index < 0)
		opp_error("Negative output index not permitted");
	collectOutputNocheck(descr, index, "total", 1);
}

void CastaliaModule::collectOutput(const char *descr, int index,
						const char *label, double amt)
{
	if (index < 0)
		opp_error("Negative output index not permitted");
	collectOutputNocheck(descr, index, label, amt);
}

void CastaliaModule::collectOutputNocheck(const char *descr, int index,
						const char *label, double amt)
{
	outputKeyDef key(descr, index);
	simpleOutputMapType::iterator i = simpleoutputs.find(key);
	if (i != simpleoutputs.end()) {
		simpleoutputs[key].data[label] += amt;
	}
}

void CastaliaModule::collectOutput(const char *descr, int index, const char *label)
{
	if (index < 0)
		opp_error("Negative output index not permitted");
	collectOutputNocheck(descr, index, label, 1);
}

void CastaliaModule::declareHistogramNocheck(const char *descr, double min,
					double max, int buckets, int index)
{
	if (min >= max || buckets < 1)
		opp_error("ERROR: declareHistogram failed, bad parameters");

	outputKeyDef key(descr, index);
	histogramOutputTypeDef hist;
	hist.buckets.clear();
	hist.buckets.resize(buckets + 1);
	for (int i = 0; i <= buckets; i++)
		hist.buckets[i] = 0;
	hist.min = min;
	hist.max = max;
	hist.cell = (max - min) / buckets;
	hist.numBuckets = buckets;
	hist.active = false;
	histograms[key] = hist;
}

void CastaliaModule::declareHistogram(const char *descr, double min, double max,
						int buckets, int index)
{
	if (index < 0)
		opp_error("Negative output index not permitted");
	declareHistogramNocheck(descr, min, max, buckets, index);
}

void CastaliaModule::collectHistogram(const char *descr, int index, double value)
{
	if (index < 0)
		opp_error("Negative output index not permitted");
	collectHistogramNocheck(descr, index, value);
}

void CastaliaModule::collectHistogramNocheck(const char *descr, int index, double value)
{
	outputKeyDef key(descr, index);
	histogramOutputMapType::iterator i = histograms.find(key);
	if (i != histograms.end()) {
		int num;
		if (value < i->second.min)
			return;
		else if (value >= i->second.max)
			num = i->second.numBuckets;
		else
			num = (int)floor((value - i->second.min) / i->second.cell);
		i->second.buckets[num]++;
		i->second.active = true;
	}
}

void CastaliaModule::powerDrawn(double power)
{
/*
    if (!resourceManager) {
	string name(getName());
	if (name.compare("Radio") && name.compare("nodeSensorDevMgr")) 
	    opp_error("%s module has no rights to call drawPower() function",getFullPath().c_str());
	
	cModule *tmp = getParentModule();
	while (tmp) {
	    string name(tmp->getName());
	    if (name.compare("node") == 0) {
			resourceManager = tmp->getSubmodule("nodeResourceMgr");
			break;
	    }
	    tmp = tmp->getParentModule();
	}
        if (!resourceManager) 
        	opp_error("Unable to find pointer to resource manager module");
    }
*/
	ResourceManagerMessage *drawPowerMsg =
	    new ResourceManagerMessage("Power consumption message", RESOURCE_MANAGER_DRAW_POWER);
	drawPowerMsg->setPowerConsumed(power);

	string name(getName());
	if (name.compare("Radio") == 0) {
		sendDirect(drawPowerMsg, 
			getParentModule()->getParentModule()->getSubmodule("ResourceManager"), "powerConsumption");
	} else if (name.compare("SensorManager") == 0) {
		sendDirect(drawPowerMsg,
			getParentModule()->getSubmodule("ResourceManager"), "powerConsumption");
	} else {
		opp_error("%s module has no rights to call drawPower() function", getFullPath().c_str());
	}
}
