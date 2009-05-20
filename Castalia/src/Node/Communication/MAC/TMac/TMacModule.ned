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

simple TMacModule

    parameters:
		printDebugInfo: bool,
    	        //printDroppedPacketsStatistics:	bool,
	        printPotentiallyDroppedPacketsStatistics:	bool,
    	        printStateTransitions:	bool,
    
		maxMACFrameSize		: const,	// in bytes
		macFrameOverhead	: const,	// in 802.15.4 = 5 + (4-->20) bytes
		
		ackFrameSize		: const,	// in 802.15.4 is always fix-sized = 5 bytes
		syncFrameSize		: const,	
		rtsFrameSize		: const,	
		ctsFrameSize		: const,
		
		macBufferSize		: const,	//in number of frames
			
		backoffType		: const,	// 0-->(backoff = sleepinterval), 1-->(backoff = constant value), 2-->(backoff = multiplying value - e.g. 1*a, 2*a, 3*a, 4*a ...), 3-->(backoff = exponential value - e.g. 2, 4, 8, 16, 32...),4->backoff = listenTimeout
		backoffBaseValue	: const,	// the backoff base value in ms
		resyncTimer		: const,	//timer for re-sending SYNC msg
		syncSetupTime		: bool,
		contentionPeriod	: const,
		listenTimeout		: const,	//15 ms, is the timeout TA(Activation event)
		useFrts			: const,	//enable/disable FRTS (Future Request To Send)
		useRtsCts		: const,	
		frameTime		: const;	//frame time (standard = 610ms)

		

    gates:
        
 	out:	toNetworkModule, toRadioModule ;
	in:	fromNetworkModule, fromRadioModule, fromCommModuleResourceMgr;
      

endsimple
