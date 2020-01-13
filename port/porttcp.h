/*
 * FreeModbusPlus Libary: RT-Thread lwip Port
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
 * File: $Id: porttcp.h,v 1.0 2020/01/13 rainbowyu $
 */

#ifndef FREEMODBUSPLUS_PORTTCP_H
#define FREEMODBUSPLUS_PORTTCP_H
#include "port.h"
#include "mbport.h"
#define MB_TCP_DEBUG
BOOL xMBTCPPortInit( struct xMBPortSlave* pPortSlave, USHORT usTCPPort );
void vMBTCPPortDisable( struct xMBPortSlave* pPortSlave );
BOOL xMBTCPPortSendResponse( struct xMBPortSlave* pPortSlave, const UCHAR *pucMBTCPFrame, USHORT usTCPLength );
#endif //FREEMODBUSPLUS_PORTTIMER_H
