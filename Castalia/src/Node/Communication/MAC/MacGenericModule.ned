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




simple MacGenericModule

    parameters:
		printDebugInfo: bool,
		
		maxMACFrameSize	: const,	// in bytes
		macFrameOverhead: const,	// in 802.15.4 = 5 + (4-->20) bytes
		
		macBufferSize	: const;	//in number of messages

    gates:
        out:	toNetworkModule, toRadioModule;
        in:	fromNetworkModule, fromRadioModule, fromCommModuleResourceMgr;

endsimple
