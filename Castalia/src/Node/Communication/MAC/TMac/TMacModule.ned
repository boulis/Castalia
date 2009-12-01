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

package Node.Communication.MAC.TMac;

simple TMacModule like Node.Communication.MAC.MacGenericModule

{
    parameters:
        //debug parameters
        bool printDebugInfo = default(false);
        bool printDroppedPackets = default(false);
        bool printStateTransitions = default(false);

        //mac layer packet sizes
        int ackFrameSize = default(14);
        int syncFrameSize = default(22);
        int rtsFrameSize = default(14);
        int ctsFrameSize = default(14);

        //mac layer parameters
        int maxMACFrameSize = default(121);
        int macFrameOverhead = default(14);	
        int macBufferSize = default(32);
	int maxTxRetries = default(2);

        //TMAC algorithm parameters			
        bool allowSinkSync = default(true);
        bool useFrts = default(false);				// enable/disable FRTS (Future Request To Send), not supported
        bool useRtsCts = default(true);
        bool disableTAextension = default(false);			//disabling TA extension effectively creates an SMAC protocol
        bool conservativeTA = default(true);			// conservative activation timeout - will always stay awake for 
								// atleast 15 ms after any activity on the radio

        double resyncTime = default(40);		// timer for re-sending SYNC msg
        double contentionPeriod = default(10);
        double listenTimeout = default(15);		// 15 ms, is the timeout TA(Activation event)
        double waitTimeout = default(5);		// timeout for expecting a reply to DATA or RTS packet
        double frameTime = default(610);		// frame time (standard = 610ms)

        int collisionResolution = default(1);			// collision resolution mechanism, choose from 
  								//	0 - immediate retry
								// 	1 - based on overhearing
								//	2 - retry next frame
								
    gates:

        output toNetworkModule;
        output toRadioModule;
        input fromNetworkModule;
        input fromRadioModule;
        input fromCommModuleResourceMgr;
}
