/*
 * FreeModbusPlus Libary: base on freemodbus. A portable Modbus implementation for Modbus TCP/RTU.
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
 * File: $Id: mbfunclot.h,v 1.0 2020/01/13 rainbowyu $
 */
/* ----------------------- System includes ----------------------------------*/
#include "stdlib.h"

/* ----------------------- Platform includes --------------------------------*/
#include "port.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "mbframe.h"
#include "mbproto.h"
#include "mbconfig.h"
#include "mbutils.h"

/* ----------------------- Defines ------------------------------------------*/
#define MB_PDU_FUNC_READ_ADDR_OFF   (MB_PDU_DATA_OFF + 0)
#define MB_PDU_FUNC_READ_REGCNT_OFF (MB_PDU_DATA_OFF + 4)
#define MB_PDU_FUNC_READ_SIZE       (4)

#define MB_PDU_FUNC_WRITE_ADDR_OFF   (MB_PDU_DATA_OFF + 0)
#define MB_PDU_FUNC_WRITE_REGCNT_OFF (MB_PDU_DATA_OFF + 4)
#define MB_PDU_FUNC_WRITE_BYTE_OFF   (MB_PDU_DATA_OFF + 6)
#define MB_PDU_FUNC_WRITE_DATA_OFF   (MB_PDU_DATA_OFF + 7)
#define MB_PDU_FUNC_WRITE_SIZE_MIN   (9)

#define MB_PDU_FUNC_READ_REGCNT_MAX (120)
#define MB_PDU_FUNC_WRITE_REGCNT_MAX (120)

/* ----------------------- Static functions ---------------------------------*/
eMBException prveMBError2Exception( eMBErrorCode eErrorCode );

#if MB_FUNC_READ_LOT_ENABLED > 0
//modbus 帧格式
//  地址 命令字 读取地址HH 读取地址HL 读取地址LH 读取地址LL 读取长度H 读取长度L
// 0xXX  0x64     0xXX      0xXX     0xXX     0xXX     0xXX    0xXX
eMBException eMBFuncReadLotRegister( eMBControlBlockSlave* peConBlock, UCHAR * pucFrame, USHORT * usLen )
{
    ULONG ulRegAddress;
    USHORT usRegCount;
    UCHAR *pucFrameCur;

    eMBException    eStatus = MB_EX_NONE;
    eMBErrorCode    eRegStatus;

    if( *usLen == 7 )
    {
        ulRegAddress = (ULONG)((ULONG)pucFrame[MB_PDU_FUNC_READ_ADDR_OFF] << 24);
        ulRegAddress |= (ULONG)((ULONG)pucFrame[MB_PDU_FUNC_READ_ADDR_OFF + 1] << 16);
        ulRegAddress |= (ULONG)((ULONG)pucFrame[MB_PDU_FUNC_READ_ADDR_OFF + 2] << 8);
        ulRegAddress |= (ULONG)((ULONG)pucFrame[MB_PDU_FUNC_READ_ADDR_OFF + 3]);
        //ulRegAddress++;

        usRegCount = (USHORT)(pucFrame[MB_PDU_FUNC_READ_REGCNT_OFF] << 8);
        usRegCount |= (USHORT)(pucFrame[MB_PDU_FUNC_READ_REGCNT_OFF + 1]);

        /* Check if the number of registers to read is valid. If not
         * return Modbus illegal data value exception.
         */
        if((usRegCount >= 1) && (usRegCount <= MB_PDU_FUNC_READ_REGCNT_MAX)){//读取的寄存器数
            /* Set the current PDU data pointer to the beginning. */
            pucFrameCur = &pucFrame[MB_PDU_FUNC_OFF];
            *usLen = MB_PDU_FUNC_OFF;

            /* First byte contains the function code. */
            *pucFrameCur++ = MB_FUNC_READ_LOT_REGISTERS;
            *usLen += 1;

            /* Second byte in the response contain the number of bytes. */
            *pucFrameCur++ = (UCHAR)(usRegCount*2);
            *usLen += 1;

            /* Make callback to fill the buffer. */
            pxMBFunctionCBHandler cbHandler = xMBUtilGetCBHandler( peConBlock->xFuncHandlers,  eMBFuncReadLotRegister);
            if(cbHandler == NULL){
                eStatus = MB_EX_ILLEGAL_FUNCTION;
                goto end;
            }
            eRegStatus = cbHandler(pucFrameCur, 0, usRegCount, MB_REG_READ , &ulRegAddress);
            /* If an error occured convert it into a Modbus exception. */
            if( eRegStatus != MB_ENOERR ){
                eStatus = prveMBError2Exception( eRegStatus );
            }
            else{
                *usLen += usRegCount * 2;
            }
        }
        else{
            eStatus = MB_EX_ILLEGAL_DATA_VALUE;
        }
    }
    else
    {
        /* Can't be a valid request because the length is incorrect. */
        eStatus = MB_EX_ILLEGAL_DATA_VALUE;
    }
    end:
    return eStatus;
}
#endif

#if MB_FUNC_WRITE_LOT_ENABLED > 0
//modbus 帧格式
// 地址 命令字 写地址HH 写地址HL 写地址LH 写地址LL 写长度H 写长度L 写字节数 写内容
// 0xXX 0x65    0xXX   0xXX    0xXX    0xXX   0xXX   0xXX   0xXX   0xx
eMBException eMBFuncWriteLotRegister( eMBControlBlockSlave* peConBlock, UCHAR * pucFrame, USHORT * usLen )
{
    ULONG usRegAddress;
    USHORT usRegCount;
    UCHAR ucRegByteCount;

    eMBException    eStatus = MB_EX_NONE;
    eMBErrorCode    eRegStatus;

    if(*usLen >= ( MB_PDU_FUNC_WRITE_SIZE_MIN + MB_PDU_SIZE_MIN )){
        usRegAddress = (ULONG)((ULONG)pucFrame[MB_PDU_FUNC_WRITE_ADDR_OFF] << 24);
        usRegAddress |= (ULONG)((ULONG)pucFrame[MB_PDU_FUNC_WRITE_ADDR_OFF + 1] << 16);
        usRegAddress |= (ULONG)((ULONG)pucFrame[MB_PDU_FUNC_WRITE_ADDR_OFF + 2] << 8);
        usRegAddress |= (ULONG)((ULONG)pucFrame[MB_PDU_FUNC_WRITE_ADDR_OFF + 3]);

        usRegCount = (USHORT)(pucFrame[MB_PDU_FUNC_WRITE_REGCNT_OFF] << 8);
        usRegCount |= (USHORT)(pucFrame[MB_PDU_FUNC_WRITE_REGCNT_OFF + 1]);

        ucRegByteCount = pucFrame[MB_PDU_FUNC_WRITE_BYTE_OFF];

        /* Check if the number of registers to read is valid. If not
         * return Modbus illegal data value exception.
         */
        if((usRegCount >= 1) &&
           (usRegCount <= MB_PDU_FUNC_READ_REGCNT_MAX) &&
           (ucRegByteCount == (UCHAR)(2*usRegCount))){//读取的寄存器数
            /* Make callback to update the register values. */
            pxMBFunctionCBHandler cbHandler = xMBUtilGetCBHandler( peConBlock->xFuncHandlers,  eMBFuncWriteLotRegister);
            if(cbHandler == NULL){
                eStatus = MB_EX_ILLEGAL_FUNCTION;
                goto end;
            }
            eRegStatus = cbHandler(&pucFrame[MB_PDU_FUNC_WRITE_DATA_OFF], 0, usRegCount, MB_REG_WRITE , &usRegAddress);
            /* If an error occured convert it into a Modbus exception. */
            if( eRegStatus != MB_ENOERR ){
                eStatus = prveMBError2Exception( eRegStatus );
            }
            else{
                /* The response contains the function code, the starting
                 * address and the quantity of registers. We reuse the
                 * old values in the buffer because they are still valid.
                 */
                *usLen = MB_PDU_FUNC_WRITE_BYTE_OFF;
            }
        }
        else{
            eStatus = MB_EX_ILLEGAL_DATA_VALUE;
        }
    }
    else
    {
        /* Can't be a valid request because the length is incorrect. */
        eStatus = MB_EX_ILLEGAL_DATA_VALUE;
    }
    end:
    return eStatus;
}

#endif
