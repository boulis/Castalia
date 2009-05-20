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



enum ResourceGenericManager_MessageType
{
	RESOURCE_MGR_OUT_OF_ENERGY = 5000;
	RESOURCE_MGR_DESTROY_NODE = 5001;
	RESOURCE_MGR_PERIODIC_ENERGY = 5002;
}

message ResourceGenericManager_Message
{
};
