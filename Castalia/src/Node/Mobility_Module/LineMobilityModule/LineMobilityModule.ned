//***************************************************************************************
//*  Copyright: National ICT Australia,  2009						*
//*  Developed at the Networks and Pervasive Computing program				*
//*  Author(s): Yuri Tselishchev							*
//*  This file is distributed under the terms in the attached LICENSE file.		*
//*  If you do not find this file, copies can be found by writing to:			*
//*											*
//*      NICTA, Locked Bag 9013, Alexandria, NSW 1435, Australia			*
//*      Attention:  License Inquiry.							*
//*											*
//***************************************************************************************

package Node.Mobility_Module.LineMobilityModule;

simple LineMobilityModule like Node.Mobility_Module.VirtualMobilityModule

{
    parameters:
        bool collectTraceInfo = default(false);
	
        double updateInterval = default(1000);
        double xCoorDestination;
        double yCoorDestination;
        double zCoorDestination;
        double speed;
}
