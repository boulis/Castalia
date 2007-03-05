#include <math.h>
#include <iostream>
#include "Idea.h"


#define sqrt2PI 2.506628273

//Using function neighbors (mod 1 for incoming stimations mod 2 for local estimation)
/*int main()	
{	
	estim local;
	estim global;
	estim Fused;
	local.mean=32;
	local.variance=1;
	global.mean=25;
	global.variance=4;
	fuselocal(local,global,Fused);
	cout << "local to global fused estimation (mean):" << Fused.mean << endl;
	cout << "local to global fused estimation (variance):" << Fused.variance << endl;
	fuseglobal(local,global,Fused);
	cout << "global to global fused estimation (mean):" << Fused.mean << endl;
	cout << "global to global fused estimation (variance):" << Fused.variance << endl;

	return(0);
}*/



int fuselocal(estim localestim, estim globalestim, estim& Fusedestim, int resolation, int isSharpFall)
{
	
	double xarray[resolation];		// specify limited boundary on X axis (6*Standard Deviation of estim with higher mean 
	double A[resolation];	
	double B[resolation];
	double C[resolation];
	double Area = 0;				// under curve Area 
	double meandelta;
        double step;				// steps for caculation
	double samplingarea;
	double Sum = 0, Sum2 = 0;
	double sigmal,sigmag;			// standard deviations

	sigmal=sqrt(localestim.variance);
	sigmag=sqrt(globalestim.variance);
	meandelta=localestim.mean-globalestim.mean;
	double absmeandelta = (meandelta < 0)?-meandelta:meandelta;

	if ( (meandelta>=0) || isSharpFall )
	{
		samplingarea=6*sigmal;
		step=samplingarea/resolation;
		xarray[0]=localestim.mean-3*sigmal;
		for (int i=1;i<resolation;i++)
		{
			xarray[i]= xarray[i-1]+step;
		}
	}
	
	if (meandelta<0) 
	{
		samplingarea=6*sigmag;
		step=samplingarea/resolation;
		xarray[0]=globalestim.mean-3*sigmag;
		for (int i=1;i<resolation;i++)
		{
			xarray[i]= xarray[i-1]+step;
		}
	}

	if( normpdf(xarray, resolation , localestim.mean, sigmal, A, resolation) &&
	    normpdf(xarray, resolation , globalestim.mean, sigmag, B, resolation) )
	{
		for (int i=0; i<resolation; i++)
		{
			C[i]=A[i]+B[i];
			Area+=C[i];
		}

		Area=Area*step;

		for (int i=0; i<resolation; i++)
		{
			C[i]=C[i]/Area;
			//cout << i << "-" << xarray[i] << "-" << C[i] << endl;
			Sum+=(C[i]*xarray[i]);
			Sum2+=(xarray[i]*xarray[i]*C[i]);

		}
		Fusedestim.mean=Sum*step;
		Fusedestim.variance=((Sum2*step)-Fusedestim.mean*Fusedestim.mean);
		//cout << "Fused estimation Mean:" << Fusedestim.mean << endl;
		//cout << "Fused estimation Variance:" << Fusedestim.variance << endl;
	}
	else
		cout << "ERROR pointA";
		
}


int normpdf(double x[], int xsize , double mu, double sigma, double r[], int rsize)
{
   	if(xsize != rsize)
		return(0);
	for (int i=0;i<rsize;i++)
	{
		r[i]=(1/(sigma*sqrt2PI)) * exp((-((x[i]-mu)*(x[i]-mu))) / (2*sigma*sigma));
	}
	return(1);
}

int fuseglobal(estim incomingestim, estim globalestim, estim& Fusedestim)
{
	if ((incomingestim.variance < globalestim.variance)||((incomingestim.variance == globalestim.variance)&&(incomingestim.mean > globalestim.mean)))
	{
		Fusedestim=incomingestim;
	}
	else
	{
		Fusedestim=globalestim;
	}
	//cout << "Fused estimation Mean:" << Fusedestim.mean << endl;
	//cout << "Fused estimation Variance:" << Fusedestim.variance << endl;
}


int neighbors(int mode, estim threshold, NeighborRecord incoming, estim fused, vector<NeighborRecord>& table, double updatetime)
{
	int Cnt = 0;
	int sendto = 0;
	int idcount=1;
	int i;
	NeighborRecord rec;

	switch (mode)
	{
		case 1:
		{

			for (i=0; i<table.size();i++)
			{
				if (table[i].id==incoming.id)
				{
					table[i].estimation.mean = incoming.estimation.mean;
					table[i].estimation.variance = incoming.estimation.variance;
					table[i].time = updatetime;
					table[i].tag = true;
					Cnt=1;
				}

				if ((table[i].estimation.variance > fused.variance) && ((isBeyondThreshold(table[i].estimation.mean, fused.mean, threshold.mean)) || (isBeyondThreshold(table[i].estimation.variance, fused.variance, threshold.variance))) )
				{
					sendto++;
					//rec.id=table[i].id;
					table[i].estimation.mean = fused.mean;//rec.estimation.mean=fused.mean;
					table[i].estimation.variance = fused.variance;//rec.estimation.variance=fused.variance;
					table[i].time = updatetime;//rec.time=updatetime;
					table[i].tag = true;//rec.tag=true;
					//table[i]=rec;
				}
				else 
				{
					table[i].tag=false;
				}					
			}


			if (Cnt==0) //the incoming record is new, didn't exist in the table
			{	
				table.push_back(incoming);
				i=table.size()-1;
				if ((table[i].estimation.variance > fused.variance) && ((isBeyondThreshold(table[i].estimation.mean, fused.mean, threshold.mean)) || (isBeyondThreshold(table[i].estimation.variance, fused.variance, threshold.variance))) )
				{
					sendto++;
					table[i].estimation.mean = fused.mean;
					table[i].estimation.variance = fused.variance;
					table[i].time = updatetime;
					table[i].tag = true;
				}
				else
				{
					table[i].tag=false;
				}
			}
			break;
		}

		case 2:
		{
			for (i=0; i<table.size();i++)
			{
				if ((table[i].estimation.variance>fused.variance) && ((isBeyondThreshold(table[i].estimation.mean, fused.mean, threshold.mean)) || (isBeyondThreshold(table[i].estimation.variance, fused.variance, threshold.variance))) )
				{
					sendto++;
					table[i].estimation.mean = fused.mean;
					table[i].estimation.variance = fused.variance;
					table[i].time = updatetime;
					table[i].tag = true;
				}
				else 
				{ 
					table[i].tag=false;
				}					
			}
			break;
		}
	}
return(sendto);
}

int isBeyondThreshold(double val_A, double val_B, double thrshd)
{
	double diff = (val_A - val_B)/val_A;
	diff = (diff<0)?-diff:diff;
	return (diff > thrshd);
}
