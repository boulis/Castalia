//***************************************************************************************
//*  Copyright: National ICT Australia, 2009						*
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

struct OutputTypeDef {
    string description;
    int index;
    double value;
    int value_declared;
    map <string, int> data;
};

struct HistogramTypeDef {
    string description;
    int index;
    double min;
    double max;
    vector <int> buckets;
    int numBuckets;    
};

class VirtualCastaliaModule : public cSimpleModule 
{
    private: 
	map <int, OutputTypeDef> outputs;
	map <int, HistogramTypeDef> histograms;
	
    protected:
	virtual void finish();
	virtual void finishSpecific();
	std::ostream &trace();
	std::ostream &debug();
	
	nullstream empty;
	
	void declareOutput(int id, const char *descr, int index = -1);
	void collectOutput(int id, const char *descr = "total", int amt = 1);
	void collectOutput(int id, double value);
	void declareHistogram(int id, const char *descr, double min, double max, int buckets, int index = 0);
	void collectHistogram(int id, double value);
};

#endif 
