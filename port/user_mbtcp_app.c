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
#include <applications/app_main.h>

#include "user_mbtcp_app.h"
#include "app_main.h"
#include "mbfunc.h"
#include "portevent.h"
#include "porttcp.h"
#include "porttimer.h"

/************************* static functions ******************************/
static BOOL eMBUserPortInit( eMBControlBlockSlave* peConBlock, const char* mbName);
static eMBErrorCode
eMBRegLotCB(UCHAR * pucRegBuffer, USHORT usAddress, USHORT usNRegs, eMBRegisterMode eMode, void* userData);

/* An array of Modbus functions handlers which associates Modbus function
 * codes with implementing functions.
 */
static const xMBFunction xFuncHandlers[MB_FUNC_HANDLERS_MAX] = {
    {MB_FUNC_READ_LOT_REGISTERS, eMBFuncReadLotRegister, eMBRegLotCB},
    {MB_FUNC_WRITE_LOT_REGISTERS, eMBFuncWriteLotRegister, eMBRegLotCB}
};
static const char MBName[] = "mbtcp";
static const uint16_t modbusPort = 502; //unused

BOOL
eMBUserInitApp3( eMBControlBlockSlave* peConBlock ){
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
    param.tcpParam.peConBlock = peConBlock;
    return xMBPortSlaveInit( MBName, &peConBlock->port, PORT_TCP, param);
}

extern struct rt_event appEvent;
//协调器数据在modbus中的地址映射
//0x0000 0000 - 0x003f ffff 是节点数据区域
//0x0040 0000 - 0x0040 ffff 是节点参数区域


//通过寄存器地址映射出每个节点数据在modbus中的地址
//0x0000 0000 - 0x0000 ffff 节点0数据区域
//0x0001 0000 - 0x0001 ffff 节点1数据区域
//......
//0x00ff 0000 - 0x00ff ffff 节点255数据区域
static inline USHORT mb_node_data_addr_get(ULONG ulAddress){
    return (USHORT)((ulAddress&0x00ffffff)>>16);
}

static inline USHORT mb_node_data_offset_get(ULONG ulAddress){
    return (USHORT)((ulAddress&0x0000ffff)<<1);
}

//通过寄存器地址映射出每个节点参数在modbus中的地址
//0x0100 0000 - 0x0100 00ff 节点0参数区域
//0x0100 0100 - 0x0100 01ff 节点1参数区域
//......
//0x0100 ff00 - 0x0100 ffff 节点255参数区域
static inline USHORT mb_node_param_addr_get(ULONG ulAddress){
    return (USHORT)((ulAddress&0x0000ff00)>>8);
}

static inline UCHAR mb_node_param_id_get(ULONG ulAddress){
    return (UCHAR)(ulAddress&0x000000ff);
}

//通过寄存器地址映射出每个节点Single在modbus中的地址
//0x0101 0000 - 0x0101 00ff 节点0 Single数据区域
//0x0101 0100 - 0x0101 01ff 节点1 Single数据区域
//......
//0x0101 ff00 - 0x0101 ffff 节点255 Single数据区域
static inline USHORT mb_node_single_addr_get(ULONG ulAddress){
    return (USHORT)((ulAddress&0x0000ff00)>>8);
}

static inline UCHAR mb_node_single_id_get(ULONG ulAddress){
    return (UCHAR)(ulAddress&0x000000ff);
}

//通过寄存器地址映射出coo param在modbus中的地址
//0x0200 0010协调器参数0
//0x0200 0011协调器参数1
//......
//0x0200 001f协调器参数15
static inline UCHAR mb_coo_param_id_get(ULONG ulAddress){
    return (UCHAR)(ulAddress&0x0000000f);
}

//通过寄存器地址映射出需要对协调器执行什么控制
//0x0200 0000协调器保存参数
//0x0200 0001协调器恢复出厂设置
//0x0200 0002协调器重启
static inline UCHAR mb_coo_cmd_get(ULONG ulAddress){
    return (UCHAR)(ulAddress&0x0000000f);
}

//通过寄存器地址映射出需要获取的node位置
//0x0200 0100第0个节点
//0x0200 0101第1个节点
//...
//0x0200 01ff第255个节点
static inline UCHAR mb_node_get(ULONG ulAddress){
    return (UCHAR)(ulAddress&0x000000ff);
}

static eMBErrorCode
eMBRegLotCB(UCHAR * pucRegBuffer, USHORT usAddress, USHORT usNRegs, eMBRegisterMode eMode, void* userData){
    (void)usAddress;
    eMBErrorCode eStatus = MB_ENOERR;
    APP_ErrType appErr;
    APP_MB_EvType appEVType;
    APP_Message appMsg;
    ULONG *pulAddress =  userData;
    ULONG ulAddress = *pulAddress;

    switch (eMode){
        case MB_REG_READ:
            if(ulAddress < 0x01000000) { //读节点数据
                USHORT usNodeAddr = mb_node_data_addr_get(ulAddress);
                USHORT usOffset = mb_node_data_offset_get(ulAddress);
                appMsg.event = MB1_DataGet;
                appMsg.pEventData.getData.u16NodeAddr = usNodeAddr;
                appMsg.pEventData.getData.u16Offset = usOffset;
                appMsg.pEventData.getData.u16DataLen = (uint16_t)(usNRegs*2);
                appMsg.pEventData.getData.pu8DataBuff = pucRegBuffer;
            }else if(ulAddress < 0x01010000){ //读取节点参数
                USHORT usNodeAddr = mb_node_param_addr_get(ulAddress);
                UCHAR ucParamId = mb_node_param_id_get(ulAddress);
                appMsg.event = MB1_NParamGet;
                appMsg.pEventData.mb1GetParamData.u16NodeAddr = usNodeAddr;
                appMsg.pEventData.mb1GetParamData.u8IdStart = ucParamId;
                appMsg.pEventData.mb1GetParamData.u8Len = (uint8_t)(usNRegs);
                appMsg.pEventData.mb1GetParamData.pu16Param = (uint16_t*)pucRegBuffer;
            }else if(ulAddress < 0x01020000){ //读取独立数据
                USHORT usNodeAddr = mb_node_single_addr_get(ulAddress);
                UCHAR ucParamId = mb_node_single_id_get(ulAddress);
                appMsg.event = MB1_Single;
                appMsg.pEventData.mb1GetSingle.u16NodeAddr = usNodeAddr;
                appMsg.pEventData.mb1GetSingle.u8IdStart = ucParamId;
                appMsg.pEventData.mb1GetSingle.u8Len = (uint8_t)(usNRegs);
                appMsg.pEventData.mb1GetSingle.pu16Single = (uint16_t*)pucRegBuffer;
            }else if(ulAddress >= 0x02000010 && ulAddress < 0x02000020){ //读取协调器参数
                UCHAR ucParamId = mb_coo_param_id_get(ulAddress);
                appMsg.event = MB1_CParamGet;
                appMsg.pEventData.mb1GetCoo.u8IdStart = ucParamId;
                appMsg.pEventData.mb1GetCoo.u8Len = (uint8_t)(usNRegs);
                appMsg.pEventData.mb1GetCoo.pu16Coo = (uint16_t*)pucRegBuffer;
            }else if(ulAddress == 0x02000020){
                if(usNRegs == 2){
                    appMsg.event = MB1_CTimeGet;
                    appMsg.pEventData.mb1GetCooTime.pu16Time = (uint16_t*)pucRegBuffer;
                }else{
                    eStatus = MB_EINVAL;
                    break;
                }
            }else if(ulAddress == 0x020000ff){
                if(usNRegs == 1){
                    appMsg.event = MB1_NodeNumGet;
                    appMsg.pEventData.mb1GetNodeNum.pu16NodeNum = (uint16_t*)pucRegBuffer;
                }else{
                    eStatus = MB_EINVAL;
                    break;
                }
            }else if(ulAddress >= 0x02000100 && ulAddress < 0x02000200){
                appMsg.event = MB1_NodeGet;
                appMsg.pEventData.mb1GetNode.u8NodeNo = mb_node_get(ulAddress);
                appMsg.pEventData.mb1GetNode.u8RegNum = (uint8_t)(usNRegs);
                appMsg.pEventData.mb1GetNode.pu16Node = (uint16_t*)pucRegBuffer;
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
        case MB_REG_WRITE:
			
            if(ulAddress < 0x01000000) { //写节点数据,不允许
                eStatus = MB_EINVAL;
                break;
            }else if(ulAddress < 0x01010000){ //写节点参数
                USHORT usNodeAddr = mb_node_param_addr_get(ulAddress);
                UCHAR ucParamId = mb_node_param_id_get(ulAddress);
                appMsg.event = MB1_NParamSet;
                appMsg.pEventData.mb1SetParamData.u16NodeAddr = usNodeAddr;
                appMsg.pEventData.mb1SetParamData.u8IdStart = ucParamId;
                appMsg.pEventData.mb1SetParamData.u8Len = (uint8_t)(usNRegs);
                appMsg.pEventData.mb1SetParamData.pu16Param = (uint16_t*)pucRegBuffer;
            }else if(ulAddress >= 0x02000000 && ulAddress < 0x02000010){ //控制协调器
                appMsg.event = MB1_CCommand;
                appMsg.pEventData.mb1CmdCoo.cmd = (Coo_Cmd)mb_coo_cmd_get(ulAddress);
                appMsg.pEventData.mb1CmdCoo.pParam = (uint16_t*)pucRegBuffer;
                appMsg.pEventData.mb1CmdCoo.u8ParamQty = (uint8_t)usNRegs;
            }else if(ulAddress >= 0x02000010 && ulAddress < 0x02000020){ //写协调器参数
                    UCHAR ucParamId = mb_coo_param_id_get(ulAddress);
                    appMsg.event = MB1_CParamSet;
                    appMsg.pEventData.mb1SetCoo.u8IdStart = ucParamId;
                    appMsg.pEventData.mb1SetCoo.u8Len = (uint8_t)(usNRegs);
                    appMsg.pEventData.mb1SetCoo.pu16Coo = (uint16_t*)pucRegBuffer;
            }else if(ulAddress == 0x02000020){
                if(usNRegs == 2){
                    appMsg.event = MB1_CTimeSet;
                    appMsg.pEventData.mb1SetCooTime.pTimestamp = (uint16_t*)pucRegBuffer;
                }else{
                    eStatus = MB_EINVAL;
                    break;
                }
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
                    case EV_OUT_OF_RANGE: /*!< 写数据超过节点参数上限 */
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
        default:
            RT_ASSERT(0);
            break;
    }
    return eStatus;
}
