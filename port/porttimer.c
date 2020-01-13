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
 * File: $Id: porttimer.c,v 1.0 2020/01/13 rainbowyu $
 */

/* ----------------------- Platform includes --------------------------------*/
#include "port.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "mb.h"
#include "mbport.h"

/* ----------------------- static functions ---------------------------------*/
static void prvvTIMERExpiredISR(struct xMBPortSlave* pPortSlave);
static void timer_timeout_ind(void* parameter);

/* ----------------------- Start implementation -----------------------------*/
BOOL xMBPortTimersInit(struct xMBPortSlave* pPortSlave, USHORT usTim1Timerout50us){
    rt_timer_init(  &pPortSlave->type.portRTU.xSlaveOsTimer.timer,
                    (const char*)pPortSlave->portName,
                    timer_timeout_ind, /* bind timeout callback function */
                    pPortSlave,
                    (50 * usTim1Timerout50us) / (1000 * 1000 / RT_TICK_PER_SECOND) + 1,
                    RT_TIMER_FLAG_ONE_SHOT); /* one shot */
    return TRUE;
}

void
vMBPortTimersEnable( struct xMBPortSlave* pPortSlave ){
    rt_timer_start(&pPortSlave->type.portRTU.xSlaveOsTimer.timer);
}

void
vMBPortTimersDisable( struct xMBPortSlave* pPortSlave ){
    rt_timer_stop(&pPortSlave->type.portRTU.xSlaveOsTimer.timer);
}

static void
prvvTIMERExpiredISR( struct xMBPortSlave* pPortSlave ){
    struct eMBControlBlockSlave* peConBlock = pPortSlave->peMBConBSlave;
    (void)peConBlock->pxMBPortCBTimerExpired( peConBlock );
}

static void
timer_timeout_ind(void* parameter){
    prvvTIMERExpiredISR( (struct xMBPortSlave*)parameter );
}
