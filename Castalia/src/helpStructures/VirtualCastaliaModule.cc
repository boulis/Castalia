//***************************************************************************************
//*  Copyright: National ICT Australia,  2010						*
//*  Developed at the Networks and Pervasive Computing program				*
//*  Author(s): Yuriy Tselishchev							*
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
    if (simpleoutputs.size() == 0 && histograms.size() == 0) return; 
    EV << "CASTALIA module:" << getFullPath() << endl;
    simpleOutputMapType::iterator i;
    for (i = simpleoutputs.begin(); i != simpleoutputs.end(); i++) {
	outputKeyDef key = i->first;
	simpleOutputTypeDef out = i->second;
	if (out.data.size() != 0) {
	    EV << "CASTALIA index:" << key.index << " output:" << key.descr << endl;
	    map<string, double>::iterator i2;
	    for (i2 = out.data.begin(); i2 != out.data.end(); i2++) {
	        EV << "CASTALIA value:" << i2->second << " label:" << i2->first << endl;
	    }
	} 
    }
    histogramOutputMapType::iterator i3;
    for (i3 = histograms.begin(); i3 != histograms.end(); i3++) {
	outputKeyDef key = i3->first;
	histogramOutputTypeDef hist = i3->second;
	if (!hist.active) { continue; }
	EV << "CASTALIA index:" << key.index << " output:" << key.descr << endl;
	EV << "CASTALIA histogram_min:" << hist.min << " histogram_max:" << hist.max << endl;
	EV << "CASTALIA histogram_values";
	for (int i = 0; i < hist.numBuckets+2; i++) {
	    EV << " " << hist.buckets[i];
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

void VirtualCastaliaModule::declareOutput(const char* descr, int index) {
    outputKeyDef key(descr,index);
    simpleoutputs[key].data.clear();
}

void VirtualCastaliaModule::collectOutput(const char* descr, int index, const char* label, double amt) {
    outputKeyDef key(descr,index);
    simpleOutputMapType::iterator i = simpleoutputs.find(key);
    if (i != simpleoutputs.end()) {
	simpleoutputs[key].data[label] += amt;
    }
}
            

void VirtualCastaliaModule::declareHistogram(const char* descr, double min, double max, int buckets, int index) {
    if (min >= max || buckets < 1) {
	debug() << "ERROR: declareHistogram failed, bad parameters";
	return;
    }
    outputKeyDef key(descr,index);
    histograms[key].buckets.clear();
    histograms[key].buckets.resize(buckets+2);
    for (int i = 0; i < buckets+2; i++) {
	histograms[key].buckets[i] = 0;
    }
    histograms[key].min = min;
    histograms[key].max = max;
    histograms[key].cell = (max - min)/buckets;
    histograms[key].numBuckets = buckets;
    histograms[key].active = false;
}

void VirtualCastaliaModule::collectHistogram(const char* descr, int index, double value) {
    outputKeyDef key(descr,index);
    histogramOutputMapType::iterator i = histograms.find(key);
    if(i != histograms.end()) {
	int num;
	if (value < histograms[key].min) {
	    num = 0;
	} else if (value >= histograms[key].max) {
	    num = histograms[key].numBuckets + 1;
	} else {
	    num = (int)ceil((value - histograms[key].min)/histograms[key].cell);
	}
	histograms[key].buckets[num]++;
	histograms[key].active = true;
    }
}
