//***************************************************************************
//*  Copyright: National ICT Australia,  2009
//*  Developed at the ATP lab, Networked Systems theme
//*  Author(s): Athanassios Boulis, Yuri Tselishchev
//*  This file is distributed under the terms in the attached LICENSE file.
//*  If you do not find this file, copies can be found by writing to:
//*
//*      NICTA, Locked Bag 9013, Alexandria, NSW 1435, Australia
//*      Attention:  License Inquiry.
//***************************************************************************/

simple Mac802154Module

    parameters:
		//debug parameters
		printDebugInfo		: const,
    	        printStateTransitions	: const,
    
		//mac layer parameters
		maxMACFrameSize		: const,	// in bytes
		macFrameOverhead	: const,	// in 802.15.4 = 5 + (4-->20) bytes
		macBufferSize		: const,	//in number of frames
		
		enableSlottedCSMA	: const,
		
		//mac layer packet sizes
		isFFD			: const,
		isPANCoordinator	: const,
		batteryLifeExtention	: const,

		frameOrder 		: const,
		beaconOrder		: const,
		unitBackoffPeriod	: const,
	        baseSlotDuration	: const,
	        numSuperframeSlots	: const,
	        macMinBE		: const,
	        macMaxBE		: const,
	        macMaxCSMABackoffs	: const,
	        macMaxFrameRetries 	: const,
	        maxLostBeacons		: const;
	        
		

    gates:
        
 	out:	toNetworkModule, toRadioModule ;
	in:	fromNetworkModule, fromRadioModule, fromCommModuleResourceMgr;
      

endsimple