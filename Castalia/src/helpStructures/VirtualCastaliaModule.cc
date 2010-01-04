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



#include "VirtualCastaliaModule.h"

void VirtualCastaliaModule::finish() {
    finishSpecific();
    EV << "CASTALIA module:" << getName() << endl;
    map<int, OutputTypeDef>::iterator iter;
    for (iter = outputs.begin(); iter != outputs.end(); iter++) {
	OutputTypeDef out = iter->second;
	if (out.data.size() != 0) {
	    EV << "CASTALIA index:" << out.index << " output:" << out.description << endl << "CASTALIA results";
	    map<string, int>::iterator iter2;
	    for (iter2 = out.data.begin(); iter2 != out.data.end(); iter2++) {
		EV << " " << iter2->first << ":" << iter2->second;
	    }
	} else if (out.value_declared == 1) {
	    EV << "CASTALIA index:" << out.index << " output:" << out.description << endl << "CASTALIA value:" << out.value;
	} else {
	    continue;
	}
	EV << endl;
    }
}

void VirtualCastaliaModule::finishSpecific() {}

std::ostream &VirtualCastaliaModule::trace() {
    if (hasPar("collectTraceInfo") && par("collectTraceInfo")) {
	return (ostream&)DebugInfoWriter::getStream() << "\n" << simTime() << "-" << getFullPath() << " ";
    } else {
	return empty;
    }
}

std::ostream &VirtualCastaliaModule::debug() {
    return cerr;
}

void VirtualCastaliaModule::declareOutput(int id, const char* descr, int index) {
    outputs[id].description = descr;
    outputs[id].data.clear();
    outputs[id].value = 0.0;
    outputs[id].value_declared = 1;
    outputs[id].index = index;
}

void VirtualCastaliaModule::collectOutput(int id, const char * descr, int amt) {
    map <int, OutputTypeDef>::iterator iter = outputs.find(id);
    if (iter != outputs.end()) {
	outputs[id].data[descr] += amt;
    }
}

void VirtualCastaliaModule::collectOutput(int id, double value) {
    map <int, OutputTypeDef>::iterator iter = outputs.find(id);
    if (iter != outputs.end()) {
	outputs[id].value = value;
	outputs[id].value_declared = 1;
    }
}

void VirtualCastaliaModule::declareHistogram(int id, const char* descr, double min, double max, int buckets, int index) {
    if (min >= max || buckets < 1) {
	debug() << "ERROR: declareHistogram failed, bad parameters";
	return;
    }
    histograms[id].description = descr;
    histograms[id].index = index;
    histograms[id].buckets.clear();
    histograms[id].buckets.resize(buckets+2);
    for (int i = 0; i < buckets+2; i++) {
	histograms[id].buckets[i] = 0;
    }
    histograms[id].min = min;
    histograms[id].max = max;
    histograms[id].numBuckets = buckets;
}

void VirtualCastaliaModule::collectHistogram(int id, double value) {
    map <int, HistogramTypeDef>::iterator iter = histograms.find(id);
    if(iter != histograms.end()) {
	int num;
	if (value < histograms[id].min) {
	    num = 0;
	} else if (value >= histograms[id].max) {
	    num = histograms[id].numBuckets + 1;
	} else {
	    num = (int)ceil((value - histograms[id].min)/histograms[id].numBuckets);
	}
	histograms[id].buckets[num]++;
    }
}
