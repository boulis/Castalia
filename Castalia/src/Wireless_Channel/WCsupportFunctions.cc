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



#include "WCsupportFunctions.h"

#define ERFINV_ERROR 100000.0



/* Approximates the addition of 2 signals expressed in dBm.
 * Value returned in dBm
 */
float addPower_dBm(float a, float b)
{
	float diff = a - b;
	/* accurate additions for specific diff values
	 * A = 10^(a/10), B = 10^(b/10), A/B = 10^(diff/10)
	 * M = max(A,B), m = min(A,B)
	 * sum = M*(1+ m/M)),  dB_add_to_max = 10*log(1+ m/M)
	 * For diff -> dB_add_to_max
	 * 0 -> 3
	 * 1 -> 2.52
	 * 2 -> 2.12
	 * 3 -> 1.76
	 * 4 -> 1.46
	 * 5 -> 1.17
	 * 6 -> 1
	 * 7 -> 0.79
	 */
	if (diff > 7.0)  return a;
	if (diff < -7.0) return b;
	if (diff > 5.0)  return (a + 1.0);
	if (diff < -5.0) return (b + 1.0);
	if (diff > 3.0)  return (a + 1.5);
	if (diff < -3.0) return (b + 1.5);
	if (diff > 2.0)  return (a + 2.0);
	if (diff < -2.0) return (b + 2.0);
	if (diff > 1.0)  return (a + 2.5);
	if (diff < -1.0) return (b + 2.5);
	if (diff > 0.0)  return (a + 3.0);
	return (b + 3.0);
}


/* Approximates the subtraction of 2 signals expressed in dBm.
 * Substract signal b from signal a. Value returned in dBm
 */
float subtractPower_dBm(float a, float b){
	float diff = a - b;
	/* accurate results for specific diff values
	 * A = 10^(a/10), B = 10^(b/10), A/B = 10^(diff/10)
	 * A, B expressed in mW  a, b expressed in dBm
	 * For diff -> subtraction_result_mW -> subtraction_result_dBm
	 * 0   -> 0                ->  - infinite
	 * 0.5 -> 0.12*B           ->  -9.2dBm + b
	 * 1   -> 0.26*B           ->  -5.8dBm + b
	 * 2   -> 0.58*B           ->  -2.4dBm + b
	 * 3   -> 1.00*B           ->   0.0dBm + b
	 * 4   -> 1.51*B or 0.33*A ->   1.8dBm + b  or -4.81dBm + a
	 * 5   -> 2.16*B or 0.68*A ->   3.3dBm + b  or -1.67dBm + a
	 * 6   -> 2.98*B or 0.75*A ->   4.7dBm + b  or -1.25dBm + a
	 * 7   -> 4.01*B or 0.80*A ->   6.0dBm + b  or -0.97dBm + a
	 * 8   -> 5.31*B or 0.84*A ->               or -0.75dBm + a
	 * 9   -> 6.95*B or 0.87*A ->               or -0.58dBm + a
	 * 10  -> 9.00*B or 0.90*A ->               or -0.45dBm + a
	 * 11  -> 11.6*B or 0.92*A ->               or -0.36dBm + a
	 * 12  -> 14.9*B or 0.94*A ->               or -0.28dBm + a
	 * 13  -> 19.0*B or 0.95*A ->               or -0.22dBm + a
	 */

	 /* The approximations we do here subtract a bit more in most cases
	  * This is best as we do not want to have residual error positive
	  * interference. Moreover intereference of many signals is a bit
	  * less than additive, so it's fine to subtract a bit more.
	  */
	 if (diff < 0.5)  return  -200.0; // practically -infinite
	 if (diff < 1.0)  return (b - 9.0);
	 if (diff < 2.0)  return (b - 5.0);
	 if (diff < 3.0)  return (b - 2.0);
	 if (diff < 4.0)  return  b;
	 if (diff < 5.0)  return (b + 2.0);
	 if (diff < 6.0)  return (a - 1.6);
	 if (diff < 7.0)  return (a - 1.2);
	 if (diff < 8.0)  return (a - 0.9);
	 if (diff < 9.0)  return (a - 0.7);
	 if (diff < 12.0) return (a - 0.5);

	 return a;

}


float erfInv( float y )
{
	static float a[] = {0.0,  0.886226899, -1.645349621,  0.914624893, -0.140543331 };
	static float b[] = {0.0, -2.118377725,  1.442710462, -0.329097515,  0.012229801 };
	static float c[] = {0.0, -1.970840454, -1.624906493,  3.429567803,  1.641345311 };
	static float d[] = {0.0,  3.543889200,  1.637067800 };

	float x, z;

 	if ( y > -1. )
  	{
  		if ( y >= -.7 )
  		{
  			if ( y <= .7 )
  			{
				z = y*y;
				x = y * (((a[4]*z+a[3])*z+a[2])*z+a[1]) / ((((b[4]*z+b[3])*z+b[2])*z+b[1])*z+1.0);
	  		}
	  		else
	  		if ( y < 1 )
	  		{
				z = sqrt(-log((1-y)/2.0));
				x = (((c[4]*z+c[3])*z+c[2])*z+c[1]) / ((d[2]*z+d[1])*z+1.0);
	  		}
	  		else
	  			return ERFINV_ERROR;
		}
		else
		{
			z = sqrt(-log((1+y)/2.0));
	  		x = -(((c[4]*z+c[3])*z+c[2])*z+c[1]) / ((d[2]*z+d[1])*z+1);
		}
	}
	else
		return ERFINV_ERROR;

  	return x;
}


float erfcInv( float y )
{
  	return erfInv(1.0-y);
}
