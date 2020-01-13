/*
 * FreeModbusPlus Libary: RT-Thread Port
 * Copyright (C) 2020 rainbowyu <yushigengyu@qq.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * File: $Id: portserial.h,v 1.0 2020/01/13 rainbowyu $
 */

#ifndef FREEMODBUSPLUS_PORTSERIAL_H
#define FREEMODBUSPLUS_PORTSERIAL_H
#include "port.h"
#include "mbport.h"

BOOL xMBPortSerialInit( struct xMBPortSlave* pPortSlave, UCHAR ucPORT,
        ULONG ulBaudRate, UCHAR ucDataBits, eMBParity eParity);
BOOL vMBPortSerialEnable( struct xMBPortSlave* pPortSlave, BOOL xRxEnable, BOOL xTxEnable);
void vMBPortClose( struct xMBPortSlave* pPortSlave );
void xMBPortSerialWaitSendOver( struct xMBPortSlave* pPortSlave ); //new function
BOOL xMBPortSerialPutByte( struct xMBPortSlave* pPortSlave, CHAR ucByte);
BOOL xMBPortSerialGetByte( struct xMBPortSlave* pPortSlave, CHAR * pucByte);
#endif //FREEMODBUSPLUS_PORTSERIAL_H
