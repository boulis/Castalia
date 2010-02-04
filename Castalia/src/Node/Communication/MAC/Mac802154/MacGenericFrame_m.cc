//
// Generated file, do not edit! Created by opp_msgc 4.0 from src/Node/Communication/MAC/Mac802154/MacGenericFrame.msg.
//

// Disable warnings about unused variables, empty switch stmts, etc:
#ifdef _MSC_VER
#  pragma warning(disable:4101)
#  pragma warning(disable:4065)
#endif

#include <iostream>
#include <sstream>
#include "MacGenericFrame_m.h"

// Template rule which fires if a struct or class doesn't have operator<<
template<typename T>
std::ostream& operator<<(std::ostream& out,const T&) {return out;}

// Another default rule (prevents compiler from choosing base class' doPacking())
template<typename T>
void doPacking(cCommBuffer *, T& t) {
    throw cRuntimeError("Parsim error: no doPacking() function for type %s or its base class (check .msg and _m.cc/h files!)",opp_typename(typeid(t)));
}

template<typename T>
void doUnpacking(cCommBuffer *, T& t) {
    throw cRuntimeError("Parsim error: no doUnpacking() function for type %s or its base class (check .msg and _m.cc/h files!)",opp_typename(typeid(t)));
}




EXECUTE_ON_STARTUP(
    cEnum *e = cEnum::find("MAC_ProtocolFrameType");
    if (!e) enums.getInstance()->add(e = new cEnum("MAC_ProtocolFrameType"));
    e->insert(MAC_PROTO_DATA_FRAME, "MAC_PROTO_DATA_FRAME");
    e->insert(MAC_PROTO_BEACON_FRAME, "MAC_PROTO_BEACON_FRAME");
    e->insert(MAC_PROTO_ACK_FRAME, "MAC_PROTO_ACK_FRAME");
    e->insert(MAC_PROTO_SYNC_FRAME, "MAC_PROTO_SYNC_FRAME");
    e->insert(MAC_PROTO_RTS_FRAME, "MAC_PROTO_RTS_FRAME");
    e->insert(MAC_PROTO_CTS_FRAME, "MAC_PROTO_CTS_FRAME");
    e->insert(MAC_PROTO_DS_FRAME, "MAC_PROTO_DS_FRAME");
    e->insert(MAC_PROTO_FRTS_FRAME, "MAC_PROTO_FRTS_FRAME");
    e->insert(MAC_802154_BEACON_FRAME, "MAC_802154_BEACON_FRAME");
    e->insert(MAC_802154_ASSOCIATE_FRAME, "MAC_802154_ASSOCIATE_FRAME");
    e->insert(MAC_802154_DATA_FRAME, "MAC_802154_DATA_FRAME");
    e->insert(MAC_802154_ACK_FRAME, "MAC_802154_ACK_FRAME");
    e->insert(MAC_802154_GTS_REQUEST_FRAME, "MAC_802154_GTS_REQUEST_FRAME");
);

macHeaderInfo::macHeaderInfo()
{
    frameType = 0;
    srcID = 0;
    destID = 0;
    dataSequenceNumber = 0;
    NAV = 0;
    SYNC = 0;
    SYNC_ID = 0;
    SYNC_SN = 0;
    PANid = 0;
    beaconOrder = 0;
    frameOrder = 0;
    BSN = 0;
    CAPlength = 0;
    GTSlength = 0;
}

void doPacking(cCommBuffer *b, macHeaderInfo& a)
{
    doPacking(b,a.frameType);
    doPacking(b,a.srcID);
    doPacking(b,a.destID);
    doPacking(b,a.dataSequenceNumber);
    doPacking(b,a.NAV);
    doPacking(b,a.SYNC);
    doPacking(b,a.SYNC_ID);
    doPacking(b,a.SYNC_SN);
    doPacking(b,a.PANid);
    doPacking(b,a.beaconOrder);
    doPacking(b,a.frameOrder);
    doPacking(b,a.BSN);
    doPacking(b,a.CAPlength);
    doPacking(b,a.GTSlength);
}

void doUnpacking(cCommBuffer *b, macHeaderInfo& a)
{
    doUnpacking(b,a.frameType);
    doUnpacking(b,a.srcID);
    doUnpacking(b,a.destID);
    doUnpacking(b,a.dataSequenceNumber);
    doUnpacking(b,a.NAV);
    doUnpacking(b,a.SYNC);
    doUnpacking(b,a.SYNC_ID);
    doUnpacking(b,a.SYNC_SN);
    doUnpacking(b,a.PANid);
    doUnpacking(b,a.beaconOrder);
    doUnpacking(b,a.frameOrder);
    doUnpacking(b,a.BSN);
    doUnpacking(b,a.CAPlength);
    doUnpacking(b,a.GTSlength);
}

class macHeaderInfoDescriptor : public cClassDescriptor
{
  public:
    macHeaderInfoDescriptor();
    virtual ~macHeaderInfoDescriptor();

    virtual bool doesSupport(cObject *obj) const;
    virtual const char *getProperty(const char *propertyname) const;
    virtual int getFieldCount(void *object) const;
    virtual const char *getFieldName(void *object, int field) const;
    virtual unsigned int getFieldTypeFlags(void *object, int field) const;
    virtual const char *getFieldTypeString(void *object, int field) const;
    virtual const char *getFieldProperty(void *object, int field, const char *propertyname) const;
    virtual int getArraySize(void *object, int field) const;

    virtual bool getFieldAsString(void *object, int field, int i, char *resultbuf, int bufsize) const;
    virtual bool setFieldAsString(void *object, int field, int i, const char *value) const;

    virtual const char *getFieldStructName(void *object, int field) const;
    virtual void *getFieldStructPointer(void *object, int field, int i) const;
};

Register_ClassDescriptor(macHeaderInfoDescriptor);

macHeaderInfoDescriptor::macHeaderInfoDescriptor() : cClassDescriptor("macHeaderInfo", "")
{
}

macHeaderInfoDescriptor::~macHeaderInfoDescriptor()
{
}

bool macHeaderInfoDescriptor::doesSupport(cObject *obj) const
{
    return dynamic_cast<macHeaderInfo *>(obj)!=NULL;
}

const char *macHeaderInfoDescriptor::getProperty(const char *propertyname) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    return basedesc ? basedesc->getProperty(propertyname) : NULL;
}

int macHeaderInfoDescriptor::getFieldCount(void *object) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    return basedesc ? 14+basedesc->getFieldCount(object) : 14;
}

unsigned int macHeaderInfoDescriptor::getFieldTypeFlags(void *object, int field) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->getFieldTypeFlags(object, field);
        field -= basedesc->getFieldCount(object);
    }
    switch (field) {
        case 0: return FD_ISEDITABLE;
        case 1: return FD_ISEDITABLE;
        case 2: return FD_ISEDITABLE;
        case 3: return FD_ISEDITABLE;
        case 4: return FD_ISEDITABLE;
        case 5: return FD_ISEDITABLE;
        case 6: return FD_ISEDITABLE;
        case 7: return FD_ISEDITABLE;
        case 8: return FD_ISEDITABLE;
        case 9: return FD_ISEDITABLE;
        case 10: return FD_ISEDITABLE;
        case 11: return FD_ISEDITABLE;
        case 12: return FD_ISEDITABLE;
        case 13: return FD_ISEDITABLE;
        default: return 0;
    }
}

const char *macHeaderInfoDescriptor::getFieldName(void *object, int field) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->getFieldName(object, field);
        field -= basedesc->getFieldCount(object);
    }
    switch (field) {
        case 0: return "frameType";
        case 1: return "srcID";
        case 2: return "destID";
        case 3: return "dataSequenceNumber";
        case 4: return "NAV";
        case 5: return "SYNC";
        case 6: return "SYNC_ID";
        case 7: return "SYNC_SN";
        case 8: return "PANid";
        case 9: return "beaconOrder";
        case 10: return "frameOrder";
        case 11: return "BSN";
        case 12: return "CAPlength";
        case 13: return "GTSlength";
        default: return NULL;
    }
}

const char *macHeaderInfoDescriptor::getFieldTypeString(void *object, int field) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->getFieldTypeString(object, field);
        field -= basedesc->getFieldCount(object);
    }
    switch (field) {
        case 0: return "int";
        case 1: return "int";
        case 2: return "int";
        case 3: return "int";
        case 4: return "simtime_t";
        case 5: return "simtime_t";
        case 6: return "int";
        case 7: return "int";
        case 8: return "int";
        case 9: return "int";
        case 10: return "int";
        case 11: return "int";
        case 12: return "int";
        case 13: return "int";
        default: return NULL;
    }
}

const char *macHeaderInfoDescriptor::getFieldProperty(void *object, int field, const char *propertyname) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->getFieldProperty(object, field, propertyname);
        field -= basedesc->getFieldCount(object);
    }
    switch (field) {
        case 0:
            if (!strcmp(propertyname,"enum")) return "MAC_ProtocolFrameType";
            return NULL;
        default: return NULL;
    }
}

int macHeaderInfoDescriptor::getArraySize(void *object, int field) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->getArraySize(object, field);
        field -= basedesc->getFieldCount(object);
    }
    macHeaderInfo *pp = (macHeaderInfo *)object; (void)pp;
    switch (field) {
        default: return 0;
    }
}

bool macHeaderInfoDescriptor::getFieldAsString(void *object, int field, int i, char *resultbuf, int bufsize) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->getFieldAsString(object,field,i,resultbuf,bufsize);
        field -= basedesc->getFieldCount(object);
    }
    macHeaderInfo *pp = (macHeaderInfo *)object; (void)pp;
    switch (field) {
        case 0: long2string(pp->frameType,resultbuf,bufsize); return true;
        case 1: long2string(pp->srcID,resultbuf,bufsize); return true;
        case 2: long2string(pp->destID,resultbuf,bufsize); return true;
        case 3: long2string(pp->dataSequenceNumber,resultbuf,bufsize); return true;
        case 4: double2string(pp->NAV,resultbuf,bufsize); return true;
        case 5: double2string(pp->SYNC,resultbuf,bufsize); return true;
        case 6: long2string(pp->SYNC_ID,resultbuf,bufsize); return true;
        case 7: long2string(pp->SYNC_SN,resultbuf,bufsize); return true;
        case 8: long2string(pp->PANid,resultbuf,bufsize); return true;
        case 9: long2string(pp->beaconOrder,resultbuf,bufsize); return true;
        case 10: long2string(pp->frameOrder,resultbuf,bufsize); return true;
        case 11: long2string(pp->BSN,resultbuf,bufsize); return true;
        case 12: long2string(pp->CAPlength,resultbuf,bufsize); return true;
        case 13: long2string(pp->GTSlength,resultbuf,bufsize); return true;
        default: return false;
    }
}

bool macHeaderInfoDescriptor::setFieldAsString(void *object, int field, int i, const char *value) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->setFieldAsString(object,field,i,value);
        field -= basedesc->getFieldCount(object);
    }
    macHeaderInfo *pp = (macHeaderInfo *)object; (void)pp;
    switch (field) {
        case 0: pp->frameType = string2long(value); return true;
        case 1: pp->srcID = string2long(value); return true;
        case 2: pp->destID = string2long(value); return true;
        case 3: pp->dataSequenceNumber = string2long(value); return true;
        case 4: pp->NAV = string2double(value); return true;
        case 5: pp->SYNC = string2double(value); return true;
        case 6: pp->SYNC_ID = string2long(value); return true;
        case 7: pp->SYNC_SN = string2long(value); return true;
        case 8: pp->PANid = string2long(value); return true;
        case 9: pp->beaconOrder = string2long(value); return true;
        case 10: pp->frameOrder = string2long(value); return true;
        case 11: pp->BSN = string2long(value); return true;
        case 12: pp->CAPlength = string2long(value); return true;
        case 13: pp->GTSlength = string2long(value); return true;
        default: return false;
    }
}

const char *macHeaderInfoDescriptor::getFieldStructName(void *object, int field) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->getFieldStructName(object, field);
        field -= basedesc->getFieldCount(object);
    }
    switch (field) {
        default: return NULL;
    }
}

void *macHeaderInfoDescriptor::getFieldStructPointer(void *object, int field, int i) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->getFieldStructPointer(object, field, i);
        field -= basedesc->getFieldCount(object);
    }
    macHeaderInfo *pp = (macHeaderInfo *)object; (void)pp;
    switch (field) {
        default: return NULL;
    }
}

GTSspec::GTSspec()
{
    owner = 0;
    start = 0;
    length = 0;
}

void doPacking(cCommBuffer *b, GTSspec& a)
{
    doPacking(b,a.owner);
    doPacking(b,a.start);
    doPacking(b,a.length);
}

void doUnpacking(cCommBuffer *b, GTSspec& a)
{
    doUnpacking(b,a.owner);
    doUnpacking(b,a.start);
    doUnpacking(b,a.length);
}

class GTSspecDescriptor : public cClassDescriptor
{
  public:
    GTSspecDescriptor();
    virtual ~GTSspecDescriptor();

    virtual bool doesSupport(cObject *obj) const;
    virtual const char *getProperty(const char *propertyname) const;
    virtual int getFieldCount(void *object) const;
    virtual const char *getFieldName(void *object, int field) const;
    virtual unsigned int getFieldTypeFlags(void *object, int field) const;
    virtual const char *getFieldTypeString(void *object, int field) const;
    virtual const char *getFieldProperty(void *object, int field, const char *propertyname) const;
    virtual int getArraySize(void *object, int field) const;

    virtual bool getFieldAsString(void *object, int field, int i, char *resultbuf, int bufsize) const;
    virtual bool setFieldAsString(void *object, int field, int i, const char *value) const;

    virtual const char *getFieldStructName(void *object, int field) const;
    virtual void *getFieldStructPointer(void *object, int field, int i) const;
};

Register_ClassDescriptor(GTSspecDescriptor);

GTSspecDescriptor::GTSspecDescriptor() : cClassDescriptor("GTSspec", "")
{
}

GTSspecDescriptor::~GTSspecDescriptor()
{
}

bool GTSspecDescriptor::doesSupport(cObject *obj) const
{
    return dynamic_cast<GTSspec *>(obj)!=NULL;
}

const char *GTSspecDescriptor::getProperty(const char *propertyname) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    return basedesc ? basedesc->getProperty(propertyname) : NULL;
}

int GTSspecDescriptor::getFieldCount(void *object) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    return basedesc ? 3+basedesc->getFieldCount(object) : 3;
}

unsigned int GTSspecDescriptor::getFieldTypeFlags(void *object, int field) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->getFieldTypeFlags(object, field);
        field -= basedesc->getFieldCount(object);
    }
    switch (field) {
        case 0: return FD_ISEDITABLE;
        case 1: return FD_ISEDITABLE;
        case 2: return FD_ISEDITABLE;
        default: return 0;
    }
}

const char *GTSspecDescriptor::getFieldName(void *object, int field) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->getFieldName(object, field);
        field -= basedesc->getFieldCount(object);
    }
    switch (field) {
        case 0: return "owner";
        case 1: return "start";
        case 2: return "length";
        default: return NULL;
    }
}

const char *GTSspecDescriptor::getFieldTypeString(void *object, int field) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->getFieldTypeString(object, field);
        field -= basedesc->getFieldCount(object);
    }
    switch (field) {
        case 0: return "int";
        case 1: return "int";
        case 2: return "int";
        default: return NULL;
    }
}

const char *GTSspecDescriptor::getFieldProperty(void *object, int field, const char *propertyname) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->getFieldProperty(object, field, propertyname);
        field -= basedesc->getFieldCount(object);
    }
    switch (field) {
        default: return NULL;
    }
}

int GTSspecDescriptor::getArraySize(void *object, int field) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->getArraySize(object, field);
        field -= basedesc->getFieldCount(object);
    }
    GTSspec *pp = (GTSspec *)object; (void)pp;
    switch (field) {
        default: return 0;
    }
}

bool GTSspecDescriptor::getFieldAsString(void *object, int field, int i, char *resultbuf, int bufsize) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->getFieldAsString(object,field,i,resultbuf,bufsize);
        field -= basedesc->getFieldCount(object);
    }
    GTSspec *pp = (GTSspec *)object; (void)pp;
    switch (field) {
        case 0: long2string(pp->owner,resultbuf,bufsize); return true;
        case 1: long2string(pp->start,resultbuf,bufsize); return true;
        case 2: long2string(pp->length,resultbuf,bufsize); return true;
        default: return false;
    }
}

bool GTSspecDescriptor::setFieldAsString(void *object, int field, int i, const char *value) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->setFieldAsString(object,field,i,value);
        field -= basedesc->getFieldCount(object);
    }
    GTSspec *pp = (GTSspec *)object; (void)pp;
    switch (field) {
        case 0: pp->owner = string2long(value); return true;
        case 1: pp->start = string2long(value); return true;
        case 2: pp->length = string2long(value); return true;
        default: return false;
    }
}

const char *GTSspecDescriptor::getFieldStructName(void *object, int field) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->getFieldStructName(object, field);
        field -= basedesc->getFieldCount(object);
    }
    switch (field) {
        default: return NULL;
    }
}

void *GTSspecDescriptor::getFieldStructPointer(void *object, int field, int i) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->getFieldStructPointer(object, field, i);
        field -= basedesc->getFieldCount(object);
    }
    GTSspec *pp = (GTSspec *)object; (void)pp;
    switch (field) {
        default: return NULL;
    }
}

Register_Class(MAC_GenericFrame);

MAC_GenericFrame::MAC_GenericFrame(const char *name, int kind) : cPacket(name,kind)
{
    GTSlist_arraysize = 0;
    this->GTSlist_var = 0;
    this->rssi_var = 0;
}

MAC_GenericFrame::MAC_GenericFrame(const MAC_GenericFrame& other) : cPacket()
{
    setName(other.getName());
    GTSlist_arraysize = 0;
    this->GTSlist_var = 0;
    operator=(other);
}

MAC_GenericFrame::~MAC_GenericFrame()
{
    delete [] GTSlist_var;
}

MAC_GenericFrame& MAC_GenericFrame::operator=(const MAC_GenericFrame& other)
{
    if (this==&other) return *this;
    cPacket::operator=(other);
    this->header_var = other.header_var;
    delete [] this->GTSlist_var;
    this->GTSlist_var = (other.GTSlist_arraysize==0) ? NULL : new GTSspec[other.GTSlist_arraysize];
    GTSlist_arraysize = other.GTSlist_arraysize;
    for (unsigned int i=0; i<GTSlist_arraysize; i++)
        this->GTSlist_var[i] = other.GTSlist_var[i];
    this->rssi_var = other.rssi_var;
    return *this;
}

void MAC_GenericFrame::parsimPack(cCommBuffer *b)
{
    cPacket::parsimPack(b);
    doPacking(b,this->header_var);
    b->pack(GTSlist_arraysize);
    doPacking(b,this->GTSlist_var,GTSlist_arraysize);
    doPacking(b,this->rssi_var);
}

void MAC_GenericFrame::parsimUnpack(cCommBuffer *b)
{
    cPacket::parsimUnpack(b);
    doUnpacking(b,this->header_var);
    delete [] this->GTSlist_var;
    b->unpack(GTSlist_arraysize);
    if (GTSlist_arraysize==0) {
        this->GTSlist_var = 0;
    } else {
        this->GTSlist_var = new GTSspec[GTSlist_arraysize];
        doUnpacking(b,this->GTSlist_var,GTSlist_arraysize);
    }
    doUnpacking(b,this->rssi_var);
}

macHeaderInfo& MAC_GenericFrame::getHeader()
{
    return header_var;
}

void MAC_GenericFrame::setHeader(const macHeaderInfo& header_var)
{
    this->header_var = header_var;
}

void MAC_GenericFrame::setGTSlistArraySize(unsigned int size)
{
    GTSspec *GTSlist_var2 = (size==0) ? NULL : new GTSspec[size];
    unsigned int sz = GTSlist_arraysize < size ? GTSlist_arraysize : size;
    for (unsigned int i=0; i<sz; i++)
        GTSlist_var2[i] = this->GTSlist_var[i];
    GTSlist_arraysize = size;
    delete [] this->GTSlist_var;
    this->GTSlist_var = GTSlist_var2;
}

unsigned int MAC_GenericFrame::getGTSlistArraySize() const
{
    return GTSlist_arraysize;
}

GTSspec& MAC_GenericFrame::getGTSlist(unsigned int k)
{
    if (k>=GTSlist_arraysize) throw cRuntimeError("Array of size %d indexed by %d", GTSlist_arraysize, k);
    return GTSlist_var[k];
}

void MAC_GenericFrame::setGTSlist(unsigned int k, const GTSspec& GTSlist_var)
{
    if (k>=GTSlist_arraysize) throw cRuntimeError("Array of size %d indexed by %d", GTSlist_arraysize, k);
    this->GTSlist_var[k]=GTSlist_var;
}

double MAC_GenericFrame::getRssi() const
{
    return rssi_var;
}

void MAC_GenericFrame::setRssi(double rssi_var)
{
    this->rssi_var = rssi_var;
}

class MAC_GenericFrameDescriptor : public cClassDescriptor
{
  public:
    MAC_GenericFrameDescriptor();
    virtual ~MAC_GenericFrameDescriptor();

    virtual bool doesSupport(cObject *obj) const;
    virtual const char *getProperty(const char *propertyname) const;
    virtual int getFieldCount(void *object) const;
    virtual const char *getFieldName(void *object, int field) const;
    virtual unsigned int getFieldTypeFlags(void *object, int field) const;
    virtual const char *getFieldTypeString(void *object, int field) const;
    virtual const char *getFieldProperty(void *object, int field, const char *propertyname) const;
    virtual int getArraySize(void *object, int field) const;

    virtual bool getFieldAsString(void *object, int field, int i, char *resultbuf, int bufsize) const;
    virtual bool setFieldAsString(void *object, int field, int i, const char *value) const;

    virtual const char *getFieldStructName(void *object, int field) const;
    virtual void *getFieldStructPointer(void *object, int field, int i) const;
};

Register_ClassDescriptor(MAC_GenericFrameDescriptor);

MAC_GenericFrameDescriptor::MAC_GenericFrameDescriptor() : cClassDescriptor("MAC_GenericFrame", "cPacket")
{
}

MAC_GenericFrameDescriptor::~MAC_GenericFrameDescriptor()
{
}

bool MAC_GenericFrameDescriptor::doesSupport(cObject *obj) const
{
    return dynamic_cast<MAC_GenericFrame *>(obj)!=NULL;
}

const char *MAC_GenericFrameDescriptor::getProperty(const char *propertyname) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    return basedesc ? basedesc->getProperty(propertyname) : NULL;
}

int MAC_GenericFrameDescriptor::getFieldCount(void *object) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    return basedesc ? 3+basedesc->getFieldCount(object) : 3;
}

unsigned int MAC_GenericFrameDescriptor::getFieldTypeFlags(void *object, int field) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->getFieldTypeFlags(object, field);
        field -= basedesc->getFieldCount(object);
    }
    switch (field) {
        case 0: return FD_ISCOMPOUND;
        case 1: return FD_ISARRAY | FD_ISCOMPOUND;
        case 2: return FD_ISEDITABLE;
        default: return 0;
    }
}

const char *MAC_GenericFrameDescriptor::getFieldName(void *object, int field) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->getFieldName(object, field);
        field -= basedesc->getFieldCount(object);
    }
    switch (field) {
        case 0: return "header";
        case 1: return "GTSlist";
        case 2: return "rssi";
        default: return NULL;
    }
}

const char *MAC_GenericFrameDescriptor::getFieldTypeString(void *object, int field) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->getFieldTypeString(object, field);
        field -= basedesc->getFieldCount(object);
    }
    switch (field) {
        case 0: return "macHeaderInfo";
        case 1: return "GTSspec";
        case 2: return "double";
        default: return NULL;
    }
}

const char *MAC_GenericFrameDescriptor::getFieldProperty(void *object, int field, const char *propertyname) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->getFieldProperty(object, field, propertyname);
        field -= basedesc->getFieldCount(object);
    }
    switch (field) {
        default: return NULL;
    }
}

int MAC_GenericFrameDescriptor::getArraySize(void *object, int field) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->getArraySize(object, field);
        field -= basedesc->getFieldCount(object);
    }
    MAC_GenericFrame *pp = (MAC_GenericFrame *)object; (void)pp;
    switch (field) {
        case 1: return pp->getGTSlistArraySize();
        default: return 0;
    }
}

bool MAC_GenericFrameDescriptor::getFieldAsString(void *object, int field, int i, char *resultbuf, int bufsize) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->getFieldAsString(object,field,i,resultbuf,bufsize);
        field -= basedesc->getFieldCount(object);
    }
    MAC_GenericFrame *pp = (MAC_GenericFrame *)object; (void)pp;
    switch (field) {
        case 0: {std::stringstream out; out << pp->getHeader(); opp_strprettytrunc(resultbuf,out.str().c_str(),bufsize-1); return true;}
        case 1: {std::stringstream out; out << pp->getGTSlist(i); opp_strprettytrunc(resultbuf,out.str().c_str(),bufsize-1); return true;}
        case 2: double2string(pp->getRssi(),resultbuf,bufsize); return true;
        default: return false;
    }
}

bool MAC_GenericFrameDescriptor::setFieldAsString(void *object, int field, int i, const char *value) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->setFieldAsString(object,field,i,value);
        field -= basedesc->getFieldCount(object);
    }
    MAC_GenericFrame *pp = (MAC_GenericFrame *)object; (void)pp;
    switch (field) {
        case 2: pp->setRssi(string2double(value)); return true;
        default: return false;
    }
}

const char *MAC_GenericFrameDescriptor::getFieldStructName(void *object, int field) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->getFieldStructName(object, field);
        field -= basedesc->getFieldCount(object);
    }
    switch (field) {
        case 0: return "macHeaderInfo"; break;
        case 1: return "GTSspec"; break;
        default: return NULL;
    }
}

void *MAC_GenericFrameDescriptor::getFieldStructPointer(void *object, int field, int i) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->getFieldStructPointer(object, field, i);
        field -= basedesc->getFieldCount(object);
    }
    MAC_GenericFrame *pp = (MAC_GenericFrame *)object; (void)pp;
    switch (field) {
        case 0: return (void *)(&pp->getHeader()); break;
        case 1: return (void *)(&pp->getGTSlist(i)); break;
        default: return NULL;
    }
}


