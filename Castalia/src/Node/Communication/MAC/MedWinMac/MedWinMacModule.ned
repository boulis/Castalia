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

package Node.Communication.MAC.MedWinMac;

simple MedWinMacModule like Node.Communication.MAC.MacGenericModule
{
	parameters:
		bool collectTraceInfo = default(false);
		int macMaxPacketSize = default(1000);
		int macBufferSize = default(32);
		int macPacketOverhead = default(7);			//in bytes, 7 bytes is the MedWin header size

		bool isHub = default(false);
		double allocationSlotLength = default(30);	//in msecs, defines how long is a slot

		int beaconPeriodLength = default(32);		//in slots, defined for the Hub
		int RAP1Length= default(8);					//in slots, defined for the Hub
		int scheduledAccessLength = default(0);		//in slots, asked by the sensor, default: no scheduled access
		int scheduledAccessPeriod = default(1);		//in beacon periods, asked by the sensor, default: 1-periodic
		
		int maxPacketRetries = default(2);
		double contentionSlotLength = default(0.36);	//in msecs, mini-slots used in contention 
		bool enhanceGuardTime = default(false);

		// parameters related to the PHY layer. As the proposal suggests we define them independantly.
		
		double pTIFS = default(0.03);				//in msecs, time to start TXing a frame after you received one
		int phyLayerOverhead = default(6);			//in bytes
		double phyDataRate = default(1024);			//in Kbps
		double mClockAccuracy = default(0.0001);	//fraction, clock drift
		
	gates:
		output toNetworkModule;
		output toRadioModule;
		input fromNetworkModule;
		input fromRadioModule;
		input fromCommModuleResourceMgr;
}