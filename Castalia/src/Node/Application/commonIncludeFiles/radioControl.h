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



#ifndef RADIOCONTROL_H_
#define RADIOCONTROL_H_


// control functions to the Radio module
void setRadio_TXPowerLevel(int parLevel, double delay=0.0);
void setRadio_TXMode(int parMode, double delay=0.0);
void setRadio_Sleep(double delay=0.0);
void setRadio_Listen(double delay=0.0);


#endif /*RADIOCONTROL_H_*/
