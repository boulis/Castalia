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
		//debug parameters
		printDebugInfo		: const,
		printDroppedPackets	: const,
    	        printStateTransitions	: const,
    
		//mac layer parameters
		maxMACFrameSize		: const,	// in bytes
		maxTxRetries		: const,
		macFrameOverhead	: const,	// in 802.15.4 = 5 + (4-->20) bytes
		macBufferSize		: const,	//in number of frames
		
		//mac layer packet sizes
		ackFrameSize		: const,	// in 802.15.4 is always fix-sized = 5 bytes
		syncFrameSize		: const,	
		rtsFrameSize		: const,	
		ctsFrameSize		: const,
		
		//TMAC algorithm parameters			
		allowSinkSync		: const,
		resyncTime		: const,	//timer for re-sending SYNC msg
		contentionPeriod	: const,
		listenTimeout		: const,	//15 ms, is the timeout TA(Activation event)
		waitTimeout		: const,
		useFrts			: const,	//enable/disable FRTS (Future Request To Send)
		useRtsCts		: const,	
		frameTime		: const,	//frame time (standard = 610ms)
		disableTAextension	: const;	//disabling TA extension effectively creates an SMAC
		

    gates:
        
 	out:	toNetworkModule, toRadioModule ;
	in:	fromNetworkModule, fromRadioModule, fromCommModuleResourceMgr;
      

endsimple
