//
// Generated file, do not edit! Created by opp_msgc 4.0 from src/Node/Communication/MAC/Mac802154/MacControlMessage.msg.
//

// Disable warnings about unused variables, empty switch stmts, etc:
#ifdef _MSC_VER
#  pragma warning(disable:4101)
#  pragma warning(disable:4065)
#endif

#include <iostream>
#include <sstream>
#include "MacControlMessage_m.h"

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
    cEnum *e = cEnum::find("MAC_ControlMessageType");
    if (!e) enums.getInstance()->add(e = new cEnum("MAC_ControlMessageType"));
    e->insert(MAC_SELF_TIMER, "MAC_SELF_TIMER");
    e->insert(MAC_SELF_CARRIER_SENSE, "MAC_SELF_CARRIER_SENSE");
    e->insert(MAC_2_RADIO_ENTER_SLEEP, "MAC_2_RADIO_ENTER_SLEEP");
    e->insert(MAC_2_RADIO_ENTER_LISTEN, "MAC_2_RADIO_ENTER_LISTEN");
    e->insert(MAC_2_RADIO_ENTER_TX, "MAC_2_RADIO_ENTER_TX");
    e->insert(MAC_2_RADIO_CHANGE_TX_MODE, "MAC_2_RADIO_CHANGE_TX_MODE");
    e->insert(MAC_2_RADIO_CHANGE_POWER_LEVEL, "MAC_2_RADIO_CHANGE_POWER_LEVEL");
    e->insert(MAC_2_RADIO_SENSE_CARRIER, "MAC_2_RADIO_SENSE_CARRIER");
    e->insert(MAC_2_RADIO_SENSE_CARRIER_INSTANTANEOUS, "MAC_2_RADIO_SENSE_CARRIER_INSTANTANEOUS");
    e->insert(MAC_SELF_CHECK_TX_BUFFER, "MAC_SELF_CHECK_TX_BUFFER");
    e->insert(MAC_SELF_EXIT_EXPECTING_RX, "MAC_SELF_EXIT_EXPECTING_RX");
    e->insert(MAC_SELF_PERFORM_CARRIER_SENSE, "MAC_SELF_PERFORM_CARRIER_SENSE");
    e->insert(MAC_SELF_EXIT_CARRIER_SENSE, "MAC_SELF_EXIT_CARRIER_SENSE");
    e->insert(MAC_SELF_SET_RADIO_SLEEP, "MAC_SELF_SET_RADIO_SLEEP");
    e->insert(MAC_SELF_WAKEUP_RADIO, "MAC_SELF_WAKEUP_RADIO");
    e->insert(MAC_SELF_INITIATE_TX, "MAC_SELF_INITIATE_TX");
    e->insert(MAC_SELF_POP_BUFFER_AND_SEND_DATA, "MAC_SELF_POP_BUFFER_AND_SEND_DATA");
    e->insert(MAC_2_NETWORK_FULL_BUFFER, "MAC_2_NETWORK_FULL_BUFFER");
    e->insert(MAC_SELF_SYNC_SETUP, "MAC_SELF_SYNC_SETUP");
    e->insert(MAC_SELF_SYNC_CREATE, "MAC_SELF_SYNC_CREATE");
    e->insert(MAC_SELF_SYNC_RENEW, "MAC_SELF_SYNC_RENEW");
    e->insert(MAC_SELF_FRAME_START, "MAC_SELF_FRAME_START");
    e->insert(MAC_SELF_WAKEUP_SILENT, "MAC_SELF_WAKEUP_SILENT");
    e->insert(MAC_SELF_CHECK_TA, "MAC_SELF_CHECK_TA");
    e->insert(MAC_TIMEOUT, "MAC_TIMEOUT");
    e->insert(MAC_SELF_PERFORM_CCA, "MAC_SELF_PERFORM_CCA");
    e->insert(MAC_SELF_RESET_TX, "MAC_SELF_RESET_TX");
    e->insert(MAC_SELF_BEACON_TIMEOUT, "MAC_SELF_BEACON_TIMEOUT");
    e->insert(MAC_SELF_GTS_START, "MAC_SELF_GTS_START");
);

Register_Class(MacControlMessage);

MacControlMessage::MacControlMessage(const char *name, int kind) : cMessage(name,kind)
{
    this->sense_carrier_interval_var = 0;
    this->radioTxMode_var = 0;
    this->powerLevel_var = 0;
    this->timerIndex_var = 0;
}

MacControlMessage::MacControlMessage(const MacControlMessage& other) : cMessage()
{
    setName(other.getName());
    operator=(other);
}

MacControlMessage::~MacControlMessage()
{
}

MacControlMessage& MacControlMessage::operator=(const MacControlMessage& other)
{
    if (this==&other) return *this;
    cMessage::operator=(other);
    this->sense_carrier_interval_var = other.sense_carrier_interval_var;
    this->radioTxMode_var = other.radioTxMode_var;
    this->powerLevel_var = other.powerLevel_var;
    this->timerIndex_var = other.timerIndex_var;
    return *this;
}

void MacControlMessage::parsimPack(cCommBuffer *b)
{
    cMessage::parsimPack(b);
    doPacking(b,this->sense_carrier_interval_var);
    doPacking(b,this->radioTxMode_var);
    doPacking(b,this->powerLevel_var);
    doPacking(b,this->timerIndex_var);
}

void MacControlMessage::parsimUnpack(cCommBuffer *b)
{
    cMessage::parsimUnpack(b);
    doUnpacking(b,this->sense_carrier_interval_var);
    doUnpacking(b,this->radioTxMode_var);
    doUnpacking(b,this->powerLevel_var);
    doUnpacking(b,this->timerIndex_var);
}

simtime_t MacControlMessage::getSense_carrier_interval() const
{
    return sense_carrier_interval_var;
}

void MacControlMessage::setSense_carrier_interval(simtime_t sense_carrier_interval_var)
{
    this->sense_carrier_interval_var = sense_carrier_interval_var;
}

int MacControlMessage::getRadioTxMode() const
{
    return radioTxMode_var;
}

void MacControlMessage::setRadioTxMode(int radioTxMode_var)
{
    this->radioTxMode_var = radioTxMode_var;
}

int MacControlMessage::getPowerLevel() const
{
    return powerLevel_var;
}

void MacControlMessage::setPowerLevel(int powerLevel_var)
{
    this->powerLevel_var = powerLevel_var;
}

int MacControlMessage::getTimerIndex() const
{
    return timerIndex_var;
}

void MacControlMessage::setTimerIndex(int timerIndex_var)
{
    this->timerIndex_var = timerIndex_var;
}

class MacControlMessageDescriptor : public cClassDescriptor
{
  public:
    MacControlMessageDescriptor();
    virtual ~MacControlMessageDescriptor();

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

Register_ClassDescriptor(MacControlMessageDescriptor);

MacControlMessageDescriptor::MacControlMessageDescriptor() : cClassDescriptor("MacControlMessage", "cMessage")
{
}

MacControlMessageDescriptor::~MacControlMessageDescriptor()
{
}

bool MacControlMessageDescriptor::doesSupport(cObject *obj) const
{
    return dynamic_cast<MacControlMessage *>(obj)!=NULL;
}

const char *MacControlMessageDescriptor::getProperty(const char *propertyname) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    return basedesc ? basedesc->getProperty(propertyname) : NULL;
}

int MacControlMessageDescriptor::getFieldCount(void *object) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    return basedesc ? 4+basedesc->getFieldCount(object) : 4;
}

unsigned int MacControlMessageDescriptor::getFieldTypeFlags(void *object, int field) const
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
        default: return 0;
    }
}

const char *MacControlMessageDescriptor::getFieldName(void *object, int field) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->getFieldName(object, field);
        field -= basedesc->getFieldCount(object);
    }
    switch (field) {
        case 0: return "sense_carrier_interval";
        case 1: return "radioTxMode";
        case 2: return "powerLevel";
        case 3: return "timerIndex";
        default: return NULL;
    }
}

const char *MacControlMessageDescriptor::getFieldTypeString(void *object, int field) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->getFieldTypeString(object, field);
        field -= basedesc->getFieldCount(object);
    }
    switch (field) {
        case 0: return "simtime_t";
        case 1: return "int";
        case 2: return "int";
        case 3: return "int";
        default: return NULL;
    }
}

const char *MacControlMessageDescriptor::getFieldProperty(void *object, int field, const char *propertyname) const
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

int MacControlMessageDescriptor::getArraySize(void *object, int field) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->getArraySize(object, field);
        field -= basedesc->getFieldCount(object);
    }
    MacControlMessage *pp = (MacControlMessage *)object; (void)pp;
    switch (field) {
        default: return 0;
    }
}

bool MacControlMessageDescriptor::getFieldAsString(void *object, int field, int i, char *resultbuf, int bufsize) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->getFieldAsString(object,field,i,resultbuf,bufsize);
        field -= basedesc->getFieldCount(object);
    }
    MacControlMessage *pp = (MacControlMessage *)object; (void)pp;
    switch (field) {
        case 0: double2string(pp->getSense_carrier_interval(),resultbuf,bufsize); return true;
        case 1: long2string(pp->getRadioTxMode(),resultbuf,bufsize); return true;
        case 2: long2string(pp->getPowerLevel(),resultbuf,bufsize); return true;
        case 3: long2string(pp->getTimerIndex(),resultbuf,bufsize); return true;
        default: return false;
    }
}

bool MacControlMessageDescriptor::setFieldAsString(void *object, int field, int i, const char *value) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->setFieldAsString(object,field,i,value);
        field -= basedesc->getFieldCount(object);
    }
    MacControlMessage *pp = (MacControlMessage *)object; (void)pp;
    switch (field) {
        case 0: pp->setSense_carrier_interval(string2double(value)); return true;
        case 1: pp->setRadioTxMode(string2long(value)); return true;
        case 2: pp->setPowerLevel(string2long(value)); return true;
        case 3: pp->setTimerIndex(string2long(value)); return true;
        default: return false;
    }
}

const char *MacControlMessageDescriptor::getFieldStructName(void *object, int field) const
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

void *MacControlMessageDescriptor::getFieldStructPointer(void *object, int field, int i) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->getFieldStructPointer(object, field, i);
        field -= basedesc->getFieldCount(object);
    }
    MacControlMessage *pp = (MacControlMessage *)object; (void)pp;
    switch (field) {
        default: return NULL;
    }
}


