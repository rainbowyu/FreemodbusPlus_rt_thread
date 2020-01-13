/* 
 * FreeModbus Libary: A portable Modbus implementation for Modbus ASCII/RTU.
 * Copyright (c) 2006 Christian Walter <wolti@sil.at>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * File: $Id: mbport.h,v 1.17 2006/12/07 22:10:34 wolti Exp $
 *            mbport.h,v 1.60 2013/08/17 11:42:56 Armink Add Master Functions  $
 *            mbport.h,v 1.1  2020/01/13 rainbowyu 修改为freemodbusPlus适配
 */

#ifndef _MB_PORT_H
#define _MB_PORT_H

#ifdef __cplusplus
PR_BEGIN_EXTERN_C
#endif
#include "port.h"

/* ----------------------- Defines ------------------------------------------*/
#define PORTNAME_MAX 10
#define PORTSERIAL_TASKSTACK 2048

/* ----------------------- Type definitions ---------------------------------*/

typedef enum{
    EV_READY            = 1<<0,         /*!< Startup finished. */
    EV_FRAME_RECEIVED   = 1<<1,         /*!< Frame received. */
    EV_EXECUTE          = 1<<2,         /*!< Execute function. */
    EV_FRAME_SENT       = 1<<3          /*!< Frame sent. */
} eMBEventType;

typedef enum{
    SET_EVENTINIT,
    SET_EVENTPOST,
    SET_EVENTGET,
}PORTEVENT_FLAG;

typedef enum{
    SET_SERIALINIT,
    SET_SERIALCLOSE,
    SET_SERIALENABLE,
    SET_SERIALGETBYTE,
    SET_SERIALPUTBYTE,
    SET_SERIALWAITSENDOVER,
}PORTSERIAL_FLAG;

typedef enum{
    SET_TCPINIT,
	SET_TCPDISABLE,
    SET_TCPSEND
}PORTTCP_FLAG;

typedef enum{
    SET_TIMERSINIT,
    SET_TIMERSENABLE,
    SET_TIMERSDISABLE,
}PORTTIMERS_FLAG;

typedef enum{
    PORT_RTU = 10,
    PORT_TCP
}PORT_TYPE;

//uart模式校验模式
typedef enum{
    MB_PAR_NONE,                /*!< No parity. */
    MB_PAR_ODD,                 /*!< Odd parity. */
    MB_PAR_EVEN                 /*!< Even parity. */
} eMBParity;

typedef struct{
    struct rt_event event;
}OsPortEvent;

//系统相关串口数据
typedef struct{
    //485半双工输入输出控制引脚
    rt_base_t control_pin;
    //软件串口发送IRQ handler thread栈
    rt_uint8_t serial_soft_trans_irq_stack[PORTSERIAL_TASKSTACK];
    //软件串口发送IRQ handler thread
    struct rt_thread thread_serial_soft_trans_irq;
    //串口event
    struct rt_event event_serial;
    //rtt串口设备
    struct rt_serial_device *serial;
}OsPortSerial;

//系统相关定时器数据
typedef struct{
    //rtt 定时器
    struct rt_timer timer;
}OsPortTimer;

struct xMBPortSlave;

typedef struct{
    struct xMBPortSlave* pPortSlave;
	struct tcp_pcb* pPcbCurrent;
}TcpPcbCBParam;

typedef BOOL portEventInit( struct xMBPortSlave* pPortSlave );
typedef BOOL portEventPost( struct xMBPortSlave* pPortSlave, eMBEventType eEvent );
typedef BOOL portEventGet( struct xMBPortSlave* pPortSlave, eMBEventType* eEvent );
             
typedef void portSerialWaitSendOver( struct xMBPortSlave* pPortSlave );//新增函数
typedef BOOL portSerialInit( struct xMBPortSlave* pPortSlave, UCHAR ucPort, ULONG ulBaudRate, UCHAR ucDataBits, eMBParity eParity );
typedef void portSerialClose( struct xMBPortSlave* pPortSlave );
typedef void portSerialEnable( struct xMBPortSlave* pPortSlave, BOOL xRxEnable, BOOL xTxEnable );
typedef BOOL portSerialGetByte( struct xMBPortSlave* pPortSlave, CHAR * pucByte );
typedef BOOL portSerialPutByte( struct xMBPortSlave* pPortSlave, CHAR ucByte );

typedef BOOL portTcpInit( struct xMBPortSlave* pPortSlave, USHORT usTCPPort );
typedef void portTcpStart( struct xMBPortSlave* pPortSlave );
typedef void portTcpDisable( struct xMBPortSlave* pPortSlave );
typedef BOOL portTcpSendResponse( struct xMBPortSlave* pPortSlave, const UCHAR *pucMBTCPFrame, USHORT usTCPLength );
             
typedef BOOL portTimersInit( struct xMBPortSlave* pPortSlave, USHORT usTimeOut50us );
typedef void portTimersEnable( struct xMBPortSlave* pPortSlave );
typedef void portTimersDisable( struct xMBPortSlave* pPortSlave );

ALIGN(RT_ALIGN_SIZE)
typedef struct xMBPortSlaveRTU{
	/* ----------------------- Serial port functions ----------------------------*/
    OsPortSerial xSlaveOsSerial;
    portSerialWaitSendOver *pPortSerialWaitSendOver;
    portSerialInit *pPortSerialInit;
    portSerialClose *pPortSerialClose;
    portSerialEnable *pPortSerialEnable;
    portSerialGetByte *pPortSerialGetByte;
    portSerialPutByte *pPortSerialPutByte;
    /* ----------------------- Timers functions ---------------------------------*/
    OsPortTimer xSlaveOsTimer;
    portTimersInit *pPortTimersInit;
    portTimersEnable *pPortTimersEnable;
    portTimersDisable *pPortTimersDisable;
}xMBPortSlaveRTU;

typedef struct xMBPortSlaveTCP{
    /* ----------------------- TCP port functions -------------------------------*/
    struct tcp_pcb *pxPcbListen;
    struct tcp_pcb *pxPcbClient;
	TcpPcbCBParam pcbListenParam;
	TcpPcbCBParam pcbClientParam;
    portTcpInit *portTcpInit;
    portTcpDisable *portTcpDisable;
    portTcpSendResponse *portTcpSendResponse;
}xMBPortSlaveTCP;

typedef union xMBPortSlaveType{
#if	MB_SLAVE_RTU_ENABLED > 0
	xMBPortSlaveRTU portRTU;
#endif
	
#if MB_SLAVE_TCP_ENABLED > 0
	xMBPortSlaveTCP portTCP;
#endif
}xMBPortSlaveType;

typedef struct xMBPortSlave{
    struct eMBControlBlockSlave* peMBConBSlave;
    char* portName[PORTNAME_MAX+1];
    /* ----------------------- Event port functions -----------------------------*/
    OsPortEvent xSlaveOsEvent;
    portEventInit *pPortEventInit;
    portEventPost *pPortEventPost;
    portEventGet *pPortEventGet;
	
	/* acroding to the modbusType */
    PORT_TYPE typeFlag;
	xMBPortSlaveType type;
}xMBPortSlave, *xMBPortSlave_t;

typedef struct{
    ULONG pin;
    struct eMBControlBlockSlave* peConBlock;
}PortSlaveRTUInitParam;

typedef struct{
    struct eMBControlBlockSlave* peConBlock;
}PortSlaveTCPInitParam;

typedef union {
    PortSlaveTCPInitParam tcpParam;
    PortSlaveRTUInitParam rtuParam;
}PortSlaveInitParam;

BOOL xMBPortSlaveInit(const char* name, xMBPortSlave_t pPortSlave, PORT_TYPE type, PortSlaveInitParam param);

#ifdef __cplusplus
PR_END_EXTERN_C
#endif
#endif
