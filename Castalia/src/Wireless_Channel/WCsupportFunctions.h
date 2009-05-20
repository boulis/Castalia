/****************************************************************************
 *  Copyright: National ICT Australia,  2007, 2008, 2009
 *  Developed at the ATP lab, Networked Systems theme
 *  Author(s): Athanassios Boulis, Dimosthenis Pediaditakis
 *  This file is distributed under the terms in the attached LICENSE file.
 *  If you do not find this file, copies can be found by writing to:
 *
 *      NICTA, Locked Bag 9013, Alexandria, NSW 1435, Australia
 *      Attention:  License Inquiry.
 ****************************************************************************/



#ifndef SUPPORTFUNCTIONS_H_
#define SUPPORTFUNCTIONS_H_

#include <math.h>
#include <omnetpp.h>


float addPower_dBm(float a, float b);

float subtractPower_dBm(float a, float b);

float erfInv( float y );

float erfcInv( float y );


#endif /*SUPPORTFUNCTIONS_H_*/
