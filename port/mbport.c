#define LOG_TAG "mb.port"
#define LOG_LVL LOG_LVL_INFO
#include "mbconb.h"
#include "mbdef.h"
#include "mbport.h"
#include "ulog.h"
#include "portserial.h"
#include "portevent.h"
#include "porttimer.h"
#include "porttcp.h"

static BOOL xMBPortSlaveConbSet(xMBPortSlave_t pPortSlave, void* pConBSlave);
static BOOL xMBPortSlaveTypeSet(xMBPortSlave_t pPortSlave, PORT_TYPE type);
static BOOL xMBPortSlaveNameSet(xMBPortSlave_t pPortSlave, const char* name);
static BOOL xMBPortSlaveEventSet(xMBPortSlave_t pPortSlave, PORTEVENT_FLAG portFlag, void* portFunction);
static BOOL xMBPortSlaveTcpSet(xMBPortSlave_t pPortSlave, PORTTCP_FLAG portFlag, void* portFunction);
static BOOL xMBPortSlaveSerialSet(xMBPortSlave_t pPortSlave, PORTSERIAL_FLAG portFlag, void* portFunction);
static BOOL xMBPortSlaveSerialConPinSet(xMBPortSlave_t pPortSlave, ULONG pin);
static BOOL xMBPortSlaveTimersSet(xMBPortSlave_t pPortSlave, PORTTIMERS_FLAG portFlag, void* portFunction);
static BOOL xMBPortSlaveCheck(xMBPortSlave_t pPortSlave);

BOOL
static xMBPortSlaveRTUInit(xMBPortSlave_t pPortSlave, ULONG ulPin){
    BOOL ret = TRUE;
    ret &= xMBPortSlaveTypeSet( pPortSlave, PORT_RTU );

    ret &= xMBPortSlaveEventSet( pPortSlave, SET_EVENTINIT, xMBPortEventInit);
    ret &= xMBPortSlaveEventSet( pPortSlave, SET_EVENTPOST, xMBPortEventPost);
    ret &= xMBPortSlaveEventSet( pPortSlave, SET_EVENTGET, xMBPortEventGet);

    ret &= xMBPortSlaveSerialSet( pPortSlave, SET_SERIALINIT, xMBPortSerialInit);
    ret &= xMBPortSlaveSerialSet( pPortSlave, SET_SERIALCLOSE, vMBPortClose);
    ret &= xMBPortSlaveSerialSet( pPortSlave, SET_SERIALENABLE, vMBPortSerialEnable);
    ret &= xMBPortSlaveSerialSet( pPortSlave, SET_SERIALGETBYTE, xMBPortSerialGetByte);
    ret &= xMBPortSlaveSerialSet( pPortSlave, SET_SERIALPUTBYTE, xMBPortSerialPutByte);
    ret &= xMBPortSlaveSerialSet( pPortSlave, SET_SERIALWAITSENDOVER, xMBPortSerialWaitSendOver);

    ret &= xMBPortSlaveSerialConPinSet( pPortSlave, ulPin);

    ret &= xMBPortSlaveTimersSet(pPortSlave, SET_TIMERSINIT, xMBPortTimersInit);
    ret &= xMBPortSlaveTimersSet(pPortSlave, SET_TIMERSENABLE, vMBPortTimersEnable);
    ret &= xMBPortSlaveTimersSet(pPortSlave, SET_TIMERSDISABLE, vMBPortTimersDisable);
    return ret;
};

BOOL xMBPortSlaveTcpInit(xMBPortSlave_t pPortSlave) {
    BOOL ret = TRUE;
    ret &= xMBPortSlaveTypeSet( pPortSlave, PORT_TCP );

    ret &= xMBPortSlaveEventSet( pPortSlave, SET_EVENTINIT, xMBPortEventInit);
    ret &= xMBPortSlaveEventSet( pPortSlave, SET_EVENTPOST, xMBPortEventPost);
    ret &= xMBPortSlaveEventSet( pPortSlave, SET_EVENTGET, xMBPortEventGet);

    ret &= xMBPortSlaveTcpSet(pPortSlave, SET_TCPINIT, xMBTCPPortInit);
    ret &= xMBPortSlaveTcpSet(pPortSlave, SET_TCPDISABLE, vMBTCPPortDisable);
    ret &= xMBPortSlaveTcpSet(pPortSlave, SET_TCPSEND, xMBTCPPortSendResponse);
    return ret;
};

BOOL
xMBPortSlaveInit(const char* name, xMBPortSlave_t pPortSlave, PORT_TYPE type, PortSlaveInitParam param) {
    BOOL ret;
    if(!xMBPortSlaveNameSet(pPortSlave, name)){
        return FALSE;
    }
    switch (type){
        case PORT_RTU:
            if(!xMBPortSlaveConbSet( pPortSlave, param.rtuParam.peConBlock )){
                return FALSE;
            }else if (!xMBPortSlaveRTUInit( pPortSlave, param.rtuParam.pin )){
                return FALSE;
            }
            break;
        case PORT_TCP:
            if(!xMBPortSlaveConbSet( pPortSlave, param.tcpParam.peConBlock )){
                return FALSE;
            }else if(!xMBPortSlaveTcpInit(pPortSlave)){
                return FALSE;
            }
            break;
        default:
            LOG_E("not support modbus type");
            RT_ASSERT(0);
            return FALSE;
    }
    ret = xMBPortSlaveCheck(pPortSlave);
    return ret;
}


BOOL 
xMBPortSlaveNameSet(xMBPortSlave_t pPortSlave, const char* name){
    uint8_t i;
    char* tmp;
    tmp = (char*)pPortSlave->portName;
    for(i=0; i< PORTNAME_MAX; i++){
        if(name[i] == '\0')
            break;
        else{
            (*tmp) = name[i];
            tmp++;
        }
    }
    (*tmp) = '\0';
    return TRUE;
}

BOOL
xMBPortSlaveConbSet(xMBPortSlave_t pPortSlave, void* pConBSlave){
    pPortSlave->peMBConBSlave = pConBSlave;
    return TRUE;
}

BOOL
xMBPortSlaveTypeSet(xMBPortSlave_t pPortSlave, PORT_TYPE type){
    pPortSlave->typeFlag = type;
    return TRUE;
}

BOOL
xMBPortSlaveCheck(xMBPortSlave_t pPortSlave) {
    if(pPortSlave->peMBConBSlave == NULL)
        return FALSE;
    
    if(pPortSlave->pPortEventInit == NULL)
        return FALSE;
    if(pPortSlave->pPortEventPost == NULL)
        return FALSE;
    if(pPortSlave->pPortEventGet == NULL)
        return FALSE;

    if(pPortSlave->typeFlag == PORT_RTU) {
#if MB_SLAVE_RTU_ENABLED > 0
        if (pPortSlave->type.portRTU.pPortTimersInit == NULL)
            return FALSE;
        if (pPortSlave->type.portRTU.pPortTimersEnable == NULL)
            return FALSE;
        if (pPortSlave->type.portRTU.pPortTimersDisable == NULL)
            return FALSE;

        if (pPortSlave->type.portRTU.pPortSerialInit == NULL)
            return FALSE;
        if (pPortSlave->type.portRTU.pPortSerialClose == NULL)
            return FALSE;
        if (pPortSlave->type.portRTU.pPortSerialEnable == NULL)
            return FALSE;
        if (pPortSlave->type.portRTU.pPortSerialGetByte == NULL)
            return FALSE;
        if (pPortSlave->type.portRTU.pPortSerialPutByte == NULL)
            return FALSE;
        if (pPortSlave->type.portRTU.pPortSerialWaitSendOver == NULL)
            return FALSE;
#endif
    }else if(pPortSlave->typeFlag == PORT_TCP){
#if MB_SLAVE_TCP_ENABLED > 0
        if(pPortSlave->type.portTCP.portTcpDisable == NULL)
            return FALSE;
        if(pPortSlave->type.portTCP.portTcpInit == NULL)
            return FALSE;
        if(pPortSlave->type.portTCP.portTcpSendResponse == NULL)
            return FALSE;
#endif
    }else{
        return FALSE;
    }
    return TRUE;
}

BOOL
xMBPortSlaveEventSet(xMBPortSlave_t pPortSlave, PORTEVENT_FLAG portFlag, void* portFunction){
    if(portFunction == NULL)
        return FALSE;
    switch(portFlag){
        case SET_EVENTINIT:
            pPortSlave->pPortEventInit = (portEventInit*)portFunction;
            break;
        case SET_EVENTPOST:
            pPortSlave->pPortEventPost = (portEventPost*)portFunction;
            break;
        case SET_EVENTGET:
            pPortSlave->pPortEventGet = (portEventGet*)portFunction;
            break;
        default:
            return FALSE;
    }
    return TRUE;
}

#if MB_SLAVE_RTU_ENABLED > 0
BOOL
xMBPortSlaveSerialSet(xMBPortSlave_t pPortSlave, PORTSERIAL_FLAG portFlag, void* portFunction){
    if(portFunction == NULL || pPortSlave->typeFlag != PORT_RTU)
        return FALSE;
    switch(portFlag){
        case SET_SERIALINIT:
            pPortSlave->type.portRTU.pPortSerialInit = (portSerialInit*)portFunction;
            break;
        case SET_SERIALCLOSE:
            pPortSlave->type.portRTU.pPortSerialClose = (portSerialClose*)portFunction;
            break;
        case SET_SERIALENABLE:
            pPortSlave->type.portRTU.pPortSerialEnable = (portSerialEnable*)portFunction;
            break;
        case SET_SERIALGETBYTE:
            pPortSlave->type.portRTU.pPortSerialGetByte = (portSerialGetByte*)portFunction;
            break;
        case SET_SERIALPUTBYTE:
            pPortSlave->type.portRTU.pPortSerialPutByte = (portSerialPutByte*)portFunction;
            break;
        case SET_SERIALWAITSENDOVER:
            pPortSlave->type.portRTU.pPortSerialWaitSendOver = (portSerialWaitSendOver*)portFunction;
            break;
        default:
            return FALSE;
    }
    return TRUE;
}

BOOL
xMBPortSlaveSerialConPinSet(xMBPortSlave_t pPortSlave, ULONG pin){
    if(pPortSlave->typeFlag != PORT_RTU)
        return FALSE;
    pPortSlave->type.portRTU.xSlaveOsSerial.control_pin = pin;
    return TRUE;
}

BOOL
xMBPortSlaveTimersSet(xMBPortSlave_t pPortSlave, PORTTIMERS_FLAG portFlag, void* portFunction){
    if(portFunction == NULL || pPortSlave->typeFlag != PORT_RTU)
        return FALSE;
    switch(portFlag){
        case SET_TIMERSINIT:
            pPortSlave->type.portRTU.pPortTimersInit = (portTimersInit*)portFunction;
            break;
        case SET_TIMERSENABLE:
            pPortSlave->type.portRTU.pPortTimersEnable = (portTimersEnable*)portFunction;
            break;
        case SET_TIMERSDISABLE:
            pPortSlave->type.portRTU.pPortTimersDisable = (portTimersDisable*)portFunction;
            break;
        default:
            return FALSE;
    }
    return TRUE;
}
#endif

#if MB_SLAVE_TCP_ENABLED > 0
BOOL
xMBPortSlaveTcpSet(xMBPortSlave_t pPortSlave, PORTTCP_FLAG portFlag, void* portFunction){
    if(portFunction == NULL || pPortSlave->typeFlag != PORT_TCP)
        return FALSE;
    switch(portFlag){
        case SET_TCPINIT:
            pPortSlave->type.portTCP.portTcpInit = (portTcpInit*)portFunction;
            break;
		case SET_TCPDISABLE:
			pPortSlave->type.portTCP.portTcpDisable = (portTcpDisable*)portFunction;
			break;
		case SET_TCPSEND:
			pPortSlave->type.portTCP.portTcpSendResponse = (portTcpSendResponse*)portFunction;
			break;
        default:
            return FALSE;
    }
    return TRUE;
}
#endif
