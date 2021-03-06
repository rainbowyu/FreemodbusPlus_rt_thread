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
 * File: $Id: portevent.h,v 1.0 2020/01/13 rainbowyu $
 */

/* ----------------------- Modbus includes ----------------------------------*/
#include "mb.h"
#include "mbport.h"

/* ----------------------- Start implementation -----------------------------*/
BOOL
xMBPortEventInit( struct xMBPortSlave* pPortSlave )
{
    rt_event_init(&pPortSlave->xSlaveOsEvent.event, (const char*)pPortSlave->portName, RT_IPC_FLAG_PRIO);
    return TRUE;
}

BOOL
xMBPortEventPost( struct xMBPortSlave* pPortSlave, eMBEventType eEvent )
{
    rt_event_send(&pPortSlave->xSlaveOsEvent.event, eEvent);
    return TRUE;
}

BOOL
xMBPortEventGet( struct xMBPortSlave* pPortSlave, eMBEventType * eEvent )
{
    rt_uint32_t recvedEvent;
    /* waiting forever OS event */
    rt_event_recv(&pPortSlave->xSlaveOsEvent.event,
            EV_READY | EV_FRAME_RECEIVED | EV_EXECUTE | EV_FRAME_SENT,
            RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR, RT_WAITING_FOREVER,
            &recvedEvent);
    switch (recvedEvent)
    {
    case EV_READY:
        *eEvent = EV_READY;
        break;
    case EV_FRAME_RECEIVED:
        *eEvent = EV_FRAME_RECEIVED;
        break;
    case EV_EXECUTE:
        *eEvent = EV_EXECUTE;
        break;
    case EV_FRAME_SENT:
        *eEvent = EV_FRAME_SENT;
        break;
    }
    return TRUE;
}
