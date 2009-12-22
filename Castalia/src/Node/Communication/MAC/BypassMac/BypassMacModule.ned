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

package Node.Communication.MAC.BypassMac;

simple BypassMacModule like Node.Communication.MAC.MacGenericModule
{
    parameters:
	bool collectTraceInfo = default(false);
        bool printDebugInfo = default(false);
        int macMaxFrameSize = default(0);
        int maxMACFrameSize = default(121); 
        int macFrameOverhead = default(14); 
        int macBufferSize = default(32);

    gates:
        output toNetworkModule;
        output toRadioModule;
        input fromNetworkModule;
        input fromRadioModule;
        input fromCommModuleResourceMgr;
}
