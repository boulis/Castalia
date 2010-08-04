//********************************************************************************
//*  Copyright: National ICT Australia,  2007 - 2010                             *
//*  Developed at the Networks and Pervasive Computing program                   *
//*  Author(s): Athanassios Boulis, Dimosthenis Pediaditakis                     *
//*  This file is distributed under the terms in the attached LICENSE file.      *
//*  If you do not find this file, copies can be found by writing to:            *
//*                                                                              *
//*      NICTA, Locked Bag 9013, Alexandria, NSW 1435, Australia                 *
//*      Attention:  License Inquiry.                                            *
//*                                                                              *
//*******************************************************************************/

package Node.Communication.MAC;

moduleinterface MacGenericModule {
 parameters:
 	bool collectTraceInfo;
	int macMaxPacketSize;	// in bytes
	int macBufferSize;		// in number of messages
	int macPacketOverhead;

 gates: 
 	output toNetworkModule;
	output toRadioModule;
	input fromNetworkModule;
	input fromRadioModule;
	input fromCommModuleResourceMgr;
}

