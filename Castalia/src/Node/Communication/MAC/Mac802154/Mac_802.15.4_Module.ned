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

package Node.Communication.MAC.Mac802154;

simple Mac802154Module like Node.Communication.MAC.MacGenericModule

{
    parameters:
        //debug parameters
        bool printDebugInfo = default(false);
        bool printStateTransitions = default(false);

        //mac layer parameters
        int maxMACFrameSize = default(121);
        int macFrameOverhead = default(14);
        int macBufferSize = default(32);

        bool enableSlottedCSMA = default(true);
        bool enableCAP = default(true);
        bool isFFD = default(false);
        bool isPANCoordinator = default(false);
        bool batteryLifeExtention = default(false);

        int frameOrder = default(4);
        int beaconOrder = default(6);
        int unitBackoffPeriod = default(20); 
        int baseSlotDuration = default(60);
        
        int numSuperframeSlots = default(16);
        int macMinBE = default(5);
        int macMaxBE = default(7);
        int macMaxCSMABackoffs = default(4);
        int macMaxFrameRetries = default(2);
        int maxLostBeacons = default(4);
        int minCAPLength = default(440);
        int requestGTS = default(0);

    gates:

        output toNetworkModule;
        output toRadioModule;
        input fromNetworkModule;
        input fromRadioModule;
        input fromCommModuleResourceMgr;


}
