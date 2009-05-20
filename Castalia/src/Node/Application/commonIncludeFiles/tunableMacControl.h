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



#ifndef TUNABLEMACCONTROLIMPL_H_
#define TUNABLEMACCONTROLIMPL_H_


// control functions to the MAC module
void setMac_DutyCycle(double parDutyCycle, double delay=0.0);
void setMac_ListenInterval(double parInterval, double delay=0.0);
void setMac_BeaconIntervalFraction(double parInterval, double delay=0.0);
void setMac_ProbTX(double parProb, double delay=0.0);
void setMac_NumTX(int parNum, double delay=0.0);
void setMac_RndTXOffset(double parOffset, double delay=0.0);
void setMac_ReTXInterval(double parInterval, double delay=0.0);
void setMac_BackoffType(int parType, double delay=0.0);
void setMac_BackoffBaseValue(double parValue, double delay=0.0);
void setMac_UsingCarrierSense(bool parValue, double delay=0.0);


#endif /*TUNABLEMACCONTROLIMPL_H_*/
