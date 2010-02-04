//***************************************************************************************
//*  Copyright: National ICT Australia,  2007 - 2010					*
//*  Developed at the Networks and Pervasive Computing program				*
//*  Author(s): Athanassios Boulis, Dimosthenis Pediaditakis, Yuriy Tselishchev		*
//*  This file is distributed under the terms in the attached LICENSE file.		*
//*  If you do not find this file, copies can be found by writing to:			*
//*											*
//*      NICTA, Locked Bag 9013, Alexandria, NSW 1435, Australia			*
//*      Attention:  License Inquiry.							*
//*											*
//***************************************************************************************


enum multipathRingsRoutingControlDef {
    NOT_CONNECTED = 1;
    CONNECTED_TO_TREE = 2;
    TREE_LEVEL_UPDATED = 3;
}

message multipathRingsRoutingControlMessage {
    int multipathRingsRoutingControlMessageKind enum(multipathRingsRoutingControlDef);
    int sinkID;
    int level;
}
