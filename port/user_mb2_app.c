/*
 * FreeModbus Libary: user callback functions and buffer define in slave mode
 * Copyright (C) 2013 Armink <armink.ztl@gmail.com>
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
 * File: $Id: user_mb_app.c,v 1.60 2013/11/23 11:49:05 Armink $
 */
#define LOG_TAG "MB_APP"
#include <ulog.h>

#include "user_mb2_app.h"
#include "app_main.h"
#include "mbfunc.h"
#include "portevent.h"
#include "portserial.h"
#include "porttimer.h"

/************************* static functions ******************************/
static BOOL eMBUserPortInit( eMBControlBlockSlave* peConBlock, const char* mbName, ULONG ulPin);
static eMBErrorCode eMBRegHoldingCB(UCHAR * pucRegBuffer, USHORT usAddress, USHORT usNRegs, eMBRegisterMode eMode,
        void* userData);

static const xMBFunction xFuncHandlers[MB_FUNC_HANDLERS_MAX] = {
    {MB_FUNC_READ_HOLDING_REGISTER, eMBFuncReadHoldingRegister, eMBRegHoldingCB},
};
static const char MBName[] = "mb2rtu";
static const ULONG conPin = 255; //unused

extern struct rt_event appEvent;

BOOL
eMBUserInitApp2( eMBControlBlockSlave* peConBlock ){
    for( uint8_t i = 0; i < MB_FUNC_HANDLERS_MAX; i++ ){
        /* No more function handlers registered. Abort. */
        if( xFuncHandlers[i].ucFunctionCode == 0 || xFuncHandlers[i].pxHandler == NULL){
            continue;
        }
        else{
            eMBRegisterCB( peConBlock, xFuncHandlers[i].ucFunctionCode, xFuncHandlers[i].pxHandler,
                    xFuncHandlers[i].pxCBHandle );
        }
    }
    PortSlaveInitParam param;
    param.rtuParam.peConBlock = peConBlock;
    param.rtuParam.pin = conPin;
    return xMBPortSlaveInit( MBName, &peConBlock->port, PORT_RTU, param);
}

//通过寄存器地址映射出每个节点Single在modbus中的地址
//0x0000 - 0x003b 节点0 Single数据区域
//0x0100 - 0x013b 节点1 Single数据区域
//......
//0xff00 - 0xff3b 节点255 Single数据区域
static inline UCHAR mb_node_single_addr_get(USHORT usAddress){
    return (UCHAR)((usAddress >> 8) & (uint16_t) 0xff);
}

typedef struct{
    UCHAR id;
    UCHAR off;
}nodeSingleOff;

static inline nodeSingleOff mb_node_single_id_get(USHORT usAddress){
    nodeSingleOff off;
    usAddress = (usAddress & (uint16_t)0x00ff);
    off.id = usAddress/3;
    off.off = usAddress%3;
    return off;
}

/**
 * Modbus slave holding register callback function.
 *
 * @param pucRegBuffer holding register buffer
 * @param usAddress holding register address
 * @param usNRegs holding register number
 * @param eMode read or write
 *
 * @return result
 */
static eMBErrorCode
eMBRegHoldingCB(UCHAR * pucRegBuffer, USHORT usAddress, USHORT usNRegs, eMBRegisterMode eMode, void* userData)
{
    eMBErrorCode    eStatus = MB_ENOERR;
    APP_ErrType appErr;
    APP_MB_EvType appEVType;
    APP_Message appMsg;
    (void)userData;

    switch (eMode){
        /* read current register values from the protocol stack. */
        case MB_REG_READ:
            if ((usAddress&0x00ff) < 0x3c){
                UCHAR nodeAddr;
                nodeSingleOff singleOff;
                nodeAddr = mb_node_single_addr_get(usAddress);
                singleOff = mb_node_single_id_get(usAddress);
                appMsg.event = MB2_Single;
                appMsg.pEventData.mb2GetSingle.u16NodeAddr = nodeAddr;
                appMsg.pEventData.mb2GetSingle.u8IdStart = singleOff.id;
                appMsg.pEventData.mb2GetSingle.u8Off = singleOff.off;
                appMsg.pEventData.mb2GetSingle.u8Len = (uint8_t)(usNRegs);
                appMsg.pEventData.mb2GetSingle.pu16Single = (uint16_t*)pucRegBuffer;
            }else{
                eStatus = MB_EINVAL;
                break;
            }
            if(APP_OK != app_message_put(&appMsg)){
                LOG_E("app message put err");
            }
            appErr = app_mb_event_get(&appEvent, &appEVType, 500); //等待数据读取完毕
            if(appErr == APP_OK){
                switch (appEVType){
                    case EV_OK:
                        break;
                    case EV_BUSY:         /*!< 节点忙,正在接收数据 */
                        eStatus = MB_ETIMEDOUT;
                        break;
                    case EV_OUT_OF_RANGE: /*!< 读取数据超过节点数据上限 */
                        eStatus = MB_ENOREG;
                        break;
                    case EV_NODE_ERR:
                        eStatus = MB_EINVAL;
                        break;
                    default:
                        RT_ASSERT(0);
                        break;
                }
            }else{   //APP_TIMEOUT
                eStatus = MB_ETIMEDOUT;
            }
            break;

            /* write current register values with new values from the protocol stack. */
        case MB_REG_WRITE:
            eStatus = MB_EINVAL;
            break;

        default:
            eStatus = MB_ENOREG;
            break;
    }
    return eStatus;
}
