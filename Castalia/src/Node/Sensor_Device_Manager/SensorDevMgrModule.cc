/************************************************************************************
 *  Copyright: National ICT Australia,  2006										*
 *  Developed at the Networks and Pervasive Computing program						*
 *  Author(s): Athanassios Boulis, Dimosthenis Pediaditakis							*
 *  This file is distributed under the terms in the attached LICENSE file.			*
 *  If you do not find this file, copies can be found by writing to:				*
 *																					*
 *      NICTA, Locked Bag 9013, Alexandria, NSW 1435, Australia						*
 *      Attention:  License Inquiry.												*
 *																					*
 ************************************************************************************/


#include "SensorDevMgrModule.h"

#define EV   ev.disabled() ? (ostream&)ev : ev

#define CONSUME_ENERGY(a) resMgrModule->consumeEnergy(a)

Define_Module(SensorDevMgrModule);



void SensorDevMgrModule::initialize() 
{
	self = parentModule()->index();
	
	self_xCoo = parentModule()->par("xCoor");
	
	self_yCoo = parentModule()->par("yCoor");
	
	//get a valid reference to the object of the Resources Manager module so that we can make direct calls to its public methods
	//instead of using extra messages & message types for tighlty couplped operations.
	resMgrModule = check_and_cast<ResourceGenericManager*>(parentModule()->submodule("nodeResourceMgr"));
	
	disabled = 1;
	
	parseStringParams(); //read the string params in omnet.ini and initialize the vectors
	
	sensorlastSampleTime.clear();
	sensorLastValue.clear();
	double theBias;
	for(int i=0; i<totalSensors; i++)
	{
		sensorLastValue.push_back(-1);	
		sensorlastSampleTime.push_back(-10000000);
		
		theBias = normal(0, sensorBiasSigma[i]);  // using rng generator --> "0" 
		sensorBias.push_back(theBias);
	}
}


void SensorDevMgrModule::handleMessage(cMessage *msg)
{
	int msgKind = msg->kind();
	
	
	if((disabled) && (msgKind != APP_NODE_STARTUP))
	{
		delete msg;
		msg = NULL;		// safeguard
		return;
	}
	
	switch (msgKind) 
	{
		
		/*--------------------------------------------------------------------------------------------------------------
		 * Sent by the Application submodule in order to start/switch-on the Sensor Device Manager submodule.
		 *--------------------------------------------------------------------------------------------------------------*/
		case APP_NODE_STARTUP:
		{
			disabled = 0;
			
			EV << "\n[SensoDevMgr_" << self <<"]:\n Received STARTUP MESSAGE";
		
			break;
		}
		
		/*
		 * Sent from Application to initiate a new sample request to the physical process.
		 * The message contains information (its index) about the specific sensor device that is asking the sample request.
		 * Each sensor device has its own samplerate and so its sample requests occur/are schedules with different intervals that
		 * are hold inside vector minSamplingIntervals[].
		 */
		case APP_2_SDM_SAMPLE_REQUEST:
		{
			SensorDevMgr_GenericMessage *rcvPacket;
			rcvPacket = check_and_cast<SensorDevMgr_GenericMessage*>(msg);
			int sensorIndex = rcvPacket->getSensorIndex();
			
			double currentTime = simTime();
			double interval = currentTime - sensorlastSampleTime[sensorIndex];
			int getNewSample = (interval < minSamplingIntervals[sensorIndex])?0:1;
			
			if(getNewSample) //the last request for sample was more than minSamplingIntervals[sensorIndex] time ago
			{ 
				PhyProcess_GenericMessage *requestMsg;
				requestMsg = new PhyProcess_GenericMessage("sample request", PHY_SAMPLE_REQ);
				requestMsg->setSrcID(self); //insert information about the ID of the node
				requestMsg->setSensorIndex(sensorIndex); //insert information about the index of the sensor that requests the sample
				requestMsg->setXCoor(self_xCoo);
				requestMsg->setYCoor(self_yCoo);
				
				//send the request to the physical process (using the appropriate gate index for the respective sensor device )
				send(requestMsg, "toNodeContainerModule", corrPhyProcess[sensorIndex]);
				
				//update the remaining energy of the node
				CONSUME_ENERGY(pwrConsumptionPerDevice[sensorIndex]);
				
				// update the most recent sample times in sensorlastSampleTime[]
				sensorlastSampleTime[sensorIndex] = currentTime;
			}
			else // send back the old sample value
			{
				SensorDevMgr_GenericMessage *readingMsg;
				readingMsg = new SensorDevMgr_GenericMessage("sensor reading", SDM_2_APP_SENSOR_READING);
				readingMsg->setSensorType(sensorTypes[sensorIndex].c_str());
				readingMsg->setSensedValue(sensorLastValue[sensorIndex]);
				readingMsg->setSensorIndex(sensorIndex);
				
				send(readingMsg, "toApplicationModule"); //send the sensor reading to the Application module
			}
			
			break;
		}
		
		
		/*
		 * Message received by the physical process. It holds a value and the index of the sensor that sent the sample request.
		 */
		case PHY_SAMPLE_REPLY:
		{
			PhyProcess_GenericMessage *phyReply;
			phyReply = check_and_cast<PhyProcess_GenericMessage*>(msg);
			int sensorIndex = phyReply->getSensorIndex();
			double theValue = phyReply->getValue();
			
			// add the sensor's Bias and the random noise  to the value that the Physical process returned. 
			theValue += sensorBias[sensorIndex];
			theValue += normal(0, sensorNoiseSigma[sensorIndex], 1);
			
			sensorLastValue[sensorIndex] = theValue;
			
			SensorDevMgr_GenericMessage *readingMsg;
			readingMsg = new SensorDevMgr_GenericMessage("sensor reading", SDM_2_APP_SENSOR_READING);
			readingMsg->setSensorType(sensorTypes[sensorIndex].c_str());
			readingMsg->setSensedValue(theValue);
			readingMsg->setSensorIndex(sensorIndex);
			
			send(readingMsg, "toApplicationModule"); //send the sensor reading to the Application module
			
			break;
		}
		
		
		/*--------------------------------------------------------------------------------------------------------------
		 * 
		 *--------------------------------------------------------------------------------------------------------------*/
		case RESOURCE_MGR_OUT_OF_ENERGY:
		{
			disabled = 1;
			break;
		}
		
		
		default:
		{
			EV << "\\n[Application]:\n WARNING: received packet of unknown type";
			break;
		}
	}
	
	delete msg;
	msg = NULL;		// safeguard
}


void SensorDevMgrModule::finish()
{
	
}


void SensorDevMgrModule::parseStringParams(void)
{
	const char *parameterStr, *token;
	double sampleInterval;
	
	//get the physical process index that each sensor device is monitoring
	corrPhyProcess.clear();
	parameterStr = par("corrPhyProcess");
	cStringTokenizer phyTokenizer(parameterStr);
	while ( (token = phyTokenizer.nextToken()) != NULL )
    	corrPhyProcess.push_back(atoi(token));
	
	//get the power consumption of each sensor device
	pwrConsumptionPerDevice.clear();
	parameterStr = par("pwrConsumptionPerDevice");
	cStringTokenizer pwrTokenizer(parameterStr);
	while ( (token = pwrTokenizer.nextToken()) != NULL )
    	pwrConsumptionPerDevice.push_back(((double)atof(token))/1000);
    
    //get the samplerate for each sensor device and calculate the minSamplingIntervals (that is every how many ms to request a sample from the physical process)
    minSamplingIntervals.clear();
	parameterStr = par("maxSampleRates");
	cStringTokenizer ratesTokenizer(parameterStr);
	while ( (token = ratesTokenizer.nextToken()) != NULL )
    {
    	sampleInterval = (double)(1 / atof(token));
    	minSamplingIntervals.push_back(sampleInterval);
    }
    
    //get the type of each sensor device (just a description e.g.: light, temperature etc.)
    sensorTypes.clear();
	parameterStr = par("sensorTypes");
	cStringTokenizer typesTokenizer(parameterStr);
	while ( (token = typesTokenizer.nextToken()) != NULL )
    {
    	string sensorType(token);
    	sensorTypes.push_back(sensorType);
    }
    
    // get the bias sigmas for each sensor device
    sensorBiasSigma.clear();
    parameterStr = par("devicesBias");
	cStringTokenizer biasSigmaTokenizer(parameterStr);
	while ( (token = biasSigmaTokenizer.nextToken()) != NULL )
    	sensorBiasSigma.push_back((double)atof(token));
    
    
    // get the bias sigmas for each sensor device
    sensorNoiseSigma.clear();
    parameterStr = par("devicesNoise");
	cStringTokenizer noiseSigmaTokenizer(parameterStr);
	while ( (token = noiseSigmaTokenizer.nextToken()) != NULL )
    	sensorNoiseSigma.push_back((double)atof(token));
    
    
    totalSensors = par("numSensingDevices");
    
    int totalPhyProcesses = gateSize("toNodeContainerModule");
    
    //check for malformed parameter string in the omnet.ini file
    int aSz, bSz, cSz, dSz, eSz, fSz;
    aSz = (int)pwrConsumptionPerDevice.size();
    bSz = (int)minSamplingIntervals.size();
    cSz = (int)sensorTypes.size();
    dSz = (int)corrPhyProcess.size();
    eSz = (int)sensorBiasSigma.size();
    fSz = (int)sensorNoiseSigma.size();
    
    if( (totalPhyProcesses < totalSensors) || (aSz != totalSensors) || (bSz != totalSensors) || (cSz != totalSensors) || (dSz != totalSensors)|| (eSz != totalSensors)|| (fSz != totalSensors))
    	opp_error("\n[Sensor Device Manager]: The parameters of the sensor device manager are not initialized correctly in omnet.ini file.");
}


double SensorDevMgrModule::getSensorDeviceBias(int index)
{
	Enter_Method("getSensorDeviceBias()");
	if( (totalSensors <= index) || (index < 0) )
	{
		EV << "\n[SensoDevMgr_" << self <<"]:\n Sensor index out of bound ( direct method call : getSensorDeviceBias() )";
		return -1;
	}
	else
		return sensorBiasSigma[index];
	
}
