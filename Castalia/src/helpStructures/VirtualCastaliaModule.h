//***************************************************************************************
//*  Copyright: National ICT Australia, 2010						*
//*  Developed at the Networks and Pervasive Computing program				*
//*  Author(s): Yuriy Tselishchev							*
//*  This file is distributed under the terms in the attached LICENSE file.		*
//*  If you do not find this file, copies can be found by writing to:			*
//*											*
//*      NICTA, Locked Bag 9013, Alexandria, NSW 1435, Australia			*
//*      Attention:  License Inquiry.							*
//*											*
//***************************************************************************************


#ifndef BASECASTALIAMODULE
#define BASECASTALIAMODULE

#include <map>
#include <vector>
#include <omnetpp.h>

#include "DebugInfoWriter.h"

using namespace std;

struct nullstream : ostream {
    struct nullbuf: streambuf {
	int overflow(int c) { return traits_type::not_eof(c); }
    } m_sbuf;
    nullstream(): ios(&m_sbuf), ostream(&m_sbuf) {}
}; 

struct outputKeyDef {
    string descr;
    int index;
    outputKeyDef(const char *d, const int i) : descr(d), index(i) {}
    bool operator<(const outputKeyDef& A) const { return (index < A.index || descr < A.descr );  }
};

struct simpleOutputTypeDef {
    map <string, double> data;
};

struct histogramOutputTypeDef {
    double min;
    double max;
    double cell;
    vector <int> buckets;
    int numBuckets;
    bool active;
};

typedef map<outputKeyDef, simpleOutputTypeDef> simpleOutputMapType;
typedef map<outputKeyDef, histogramOutputTypeDef> histogramOutputMapType;

class VirtualCastaliaModule : public cSimpleModule 
{
    private: 
	simpleOutputMapType simpleoutputs;
	histogramOutputMapType histograms;
	
    protected:
	virtual void finish();
	virtual void finishSpecific();
	std::ostream &trace();
	std::ostream &debug();
	
	nullstream empty;
	
	void declareOutput(const char *descr, int index = -1);
	void collectOutput(const char *descr, int index, const char *label, double amt);
	void collectOutput(const char *descr, const char *label) { return collectOutput(descr,-1,label,1); }
	void collectOutput(const char *descr, const char *label, double amt) { return collectOutput(descr,-1,label,amt); }
	void collectOutput(const char *descr, int index, const char *label) { return collectOutput(descr,index,label,1); }

	void declareHistogram(const char *descr, double min, double max, int buckets, int index = -1);
	void collectHistogram(const char *descr, int index, double value);
	void collectHistogram(const char *descr, double value) { return collectHistogram(descr,-1,value); }
	
};

#endif 
