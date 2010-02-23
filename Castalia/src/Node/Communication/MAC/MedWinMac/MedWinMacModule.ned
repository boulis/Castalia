//***************************************************************************
//*  Copyright: National ICT Australia,  2010
//*  Developed at the ATP lab, Networked Systems theme
//*  Author(s): Athanassios Boulis, Yuri Tselishchev
//*  This file is distributed under the terms in the attached LICENSE file.
//*  If you do not find this file, copies can be found by writing to:
//*
//*      NICTA, Locked Bag 9013, Alexandria, NSW 1435, Australia
//*      Attention:  License Inquiry.
//***************************************************************************/

package Node.Communication.MAC.MedWinMac

simple MedWinMacModule like Node.Communication.MAC.MacGenericModule
{
    parameters:
	int scheduledAccessLength = default(0);		//in slots, default: no scheduled access
	int scheduledAccessPeriod = default(1);		//in beacon periods, default: 1-periodic
}