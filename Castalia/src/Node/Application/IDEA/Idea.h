#ifndef _IDEA_H_
#define _IDEA_H_
#include <vector>
using namespace std;

struct estim			//structure of an estimation
{
	double mean;
	double variance;
};

struct NeighborRecord		//structure of records in neighborhood table
{
	int id;
	estim estimation;
	double time;
	bool tag;
};

int neighbors(int mode, estim threshold, NeighborRecord incoming, estim fused, vector<NeighborRecord>& table, double updatetime);

int fuselocal(estim localestim, estim globalestim, estim& Fusedestim, int resolation = 100, int isSharpFall=0); //fuse local estimation to global estimation

int normpdf(double x[], int xsize , double mu, double sigma, double r[], int rsize);         //caculating normal pdf

int fuseglobal(estim incomingestim, estim globalestim, estim& Fusedestim);                   //fusing two golobal estimation

int isBeyondThreshold(double val_A, double val_B, double thrshd);

#endif //_IDEA_H_
