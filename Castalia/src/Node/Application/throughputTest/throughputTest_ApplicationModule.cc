//***************************************************************************************
//*  Copyright: National ICT Australia,  2007, 2008, 2009, 2010				*
//*  Developed at the Networks and Pervasive Computing program				*
//*  Author(s): Athanassios Boulis, Dimosthenis Pediaditakis, Yuriy Tselishchev		*
//*  This file is distributed under the terms in the attached LICENSE file.		*
//*  If you do not find this file, copies can be found by writing to:			*
//*											*
//*      NICTA, Locked Bag 9013, Alexandria, NSW 1435, Australia			*
//*      Attention:  License Inquiry.							*
//*											*
//***************************************************************************************


#include "throughputTest_ApplicationModule.h"

Define_Module(throughputTest_ApplicationModule);

void throughputTest_ApplicationModule::startup() {
    packet_rate = par("packet_rate");
    recipientAddress = par("nextRecipient").stringValue();

    latencyHistogramMin = hasPar("latencyHistogramMin") ? par("latencyHistogramMin") : 0;
    latencyHistogramMax = hasPar("latencyHistogramMax") ? par("latencyHistogramMax") : 1000;
    latencyHistogramBuckets = hasPar("latencyHistogramBuckets") ? par("latencyHistogramBuckets") : 50;
    latencyHistogram.setRange(latencyHistogramMin,latencyHistogramMax);
    latencyHistogram.setNumCells(latencyHistogramBuckets);
    declareHistogram("Application latency",latencyHistogramMin,latencyHistogramMax,latencyHistogramBuckets);
    latencyOverflow = 0;

    packet_spacing = packet_rate > 0 ? 1/float(packet_rate) : -1;
    packet_info_table.clear();
    total_packets_received = 0;
    packets_lost_at_mac = 0;
    packets_lost_at_network = 0;
    dataSN = 0;

    if (packet_spacing > 0 && recipientAddress.compare(SELF_NETWORK_ADDRESS) != 0) setTimer(SEND_PACKET,packet_spacing);
    else { trace() << "Not sending packets"; }
}

void throughputTest_ApplicationModule::fromNetworkLayer(ApplicationGenericDataPacket* rcvPacket, const char *source, const char *path, double rssi, double lqi) {
    int sequenceNumber = rcvPacket->getSequenceNumber();

    if (recipientAddress.compare(SELF_NETWORK_ADDRESS) == 0) {
	trace() << "Received packet from node " << source << ", SN = " << sequenceNumber; 
	update_packets_received(atoi(source),sequenceNumber);
/*
	long latency = lround(SIMTIME_DBL((simTime() - rcvPacket->getTimestamp())*1000));
	if (latency > latencyHistogramMax) latencyOverflow++;
	latencyHistogram.collect(latency);
	collectHistogram("Application latency",latency);
*/
    } else {
	toNetworkLayer(rcvPacket->dup(),recipientAddress.c_str());
    }
}

void throughputTest_ApplicationModule::timerFiredCallback(int index) {
    switch (index) {
	case SEND_PACKET: {
	    trace() << "Sending packet with SN " << dataSN;
	    toNetworkLayer(createGenericDataPacket(0,dataSN),par("nextRecipient"));
	    dataSN++;
	    setTimer(SEND_PACKET,packet_spacing);
	    break;
	}
    }
}


void throughputTest_ApplicationModule::finishSpecific()
{
	// print out the number of packets received from each node by node 0, and also the total number
	if(recipientAddress.compare(SELF_NETWORK_ADDRESS) == 0) {
		EV << "\n\n** Node [" << self << "] received from:\n";
		for(map<int,packet_info>::iterator i = packet_info_table.begin();
		    i != packet_info_table.end(); i++) {
			EV << "[" << self << "<--" << i->first << "]\t -->\t " << i->second.packets_received.size() << "\n";
			total_packets_received += i->second.packets_received.size();
		}
		EV << "total number of packets received is: " << total_packets_received << "\n";
	}

	if (packet_info_table.size() > 0) {
	    EV << "\npacket latency in ms:\n";
	    EV << "  min: "<<latencyHistogram.getMin()<<",  max: "<<latencyHistogram.getMax()<<
	      ",  avg: "<<latencyHistogram.getMean()<<",  variance: "<<latencyHistogram.getVariance()<<"\n";
	    EV << "Latency historgram from " << latencyHistogramMin << " to " << latencyHistogramMax << ":\n";
	    for (int i=0; i<latencyHistogram.getNumCells(); i++)
	    {
		EV << latencyHistogram.getCellValue(i) << " ";
	    }
	    EV << "\nSamples above " << latencyHistogramMax << ": " << latencyOverflow << "\n\n";
	}

	// output the spent energy of the node
	EV << "Node [" << self << "] spent energy: " << resMgrModule->getSpentEnergy() << "\n";
	if (packets_lost_at_network || packets_lost_at_mac) {
	    EV << "Node [" << self << "] lost packets due to buffer overflow: " << packets_lost_at_mac << "(MAC), " << packets_lost_at_network << "(Network)\n";
	}

	//close the output stream that CASTALIA_DEBUG is writing to
	DebugInfoWriter::closeStream();
}

// this method updates the number of packets received by node 0 from other nodes
void throughputTest_ApplicationModule::update_packets_received(int srcID, int SN)
{
    packet_info_table[srcID].packets_received[SN]++;
}




