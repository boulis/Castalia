/************************************************************************************
 *  Copyright: National ICT Australia,  2006										*
 *  Developed at the Networks and Pervasive Computing program						*
 *  Author(s): Athanassios Boulis, Dimosthenis Pediaditakis							*
 *  This file is distributed under the terms in the attached LICENSE file.			*
 *  If you do not find this file, copies can be found by writing to:				*
 *																					*
 *      NICTA, Locked Bag 9013, Alexandria, NSW 1435, Australia						*
 *      Attention:  License Inquiry.												*
 *																					*
 ************************************************************************************/



#ifndef _PHYSICALPROCESS_H_
#define _PHYSICALPROCESS_H_

#define SIMTIME_STEP 0.01

#include <omnetpp.h>
using namespace std;

enum phyProcessType
{
	DIRECT_NODE_VALUES = 0,
	SCENARIO_BASED = 1,
	TRACE_FILE = 2
};

typedef struct{
	simtime_t time;
	double x;
	double y;
	double value;
} sourceSnapshot;



class PhysicalProcess : public cSimpleModule 
{
	private:
		// parameters and variables
		int numSources;
		
		int numNodes;
		
		sourceSnapshot **sources_snapshots;	// N by M array, where N is numSources and, M is the maximum number of source snapshots. A source snapshot is a tuple (time, x, y, value)
		sourceSnapshot *curr_source_state;
		int *source_index;
		
		double k, a, sigma;

		int max_num_snapshots;
		const char *description;
		simtime_t time;
		
		int inputType;
		
		double defaultValue;
		double * valuesTable;

	protected:
		virtual void initialize();
		virtual void handleMessage(cMessage *msg);
		virtual void finish();
		double calculateScenarioReturnValue(const double & x_coo, const double & y_coo, const simtime_t & stime);
		void readIniFileParameters(void);
		void readScenariosFromIniFile(void);
		void initHelpStructures(void);
		
};

#endif //_PHYSICALPROCESS_H_
