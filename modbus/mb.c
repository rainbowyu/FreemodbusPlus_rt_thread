/*
 * Copyright (C) 2020 rainbowyu <yushigengyu@qq.com>
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
 * File: $Id: mb.c,v 1.0 2020/01/13 rainbowyu $
 */

/* ----------------------- System includes ----------------------------------*/
#include "stdlib.h"

/* ----------------------- Platform includes --------------------------------*/
#include "port.h"

/* ----------------------- Modbus includes ----------------------------------*/

#include "mbconfig.h"
#include "mbframe.h"
#include "mbproto.h"
#include "mbconb.h"

#include "mbport.h"
#if MB_SLAVE_RTU_ENABLED == 1
#include "mbrtu.h"
#endif
#if MB_SLAVE_ASCII_ENABLED == 1
#include "mbascii.h"
#endif
#if MB_SLAVE_TCP_ENABLED == 1
#include "mbtcp.h"
#endif

#ifndef MB_PORT_HAS_CLOSE
#define MB_PORT_HAS_CLOSE 0
#endif

/* ----------------------- Start implementation -----------------------------*/
eMBErrorCode
eMBInit( eMBControlBlockSlave* peConBlock, eMBMode eMode, UCHAR ucSlaveAddress, UCHAR ucPort, ULONG ulBaudRate, eMBParity eParity )
{
    eMBErrorCode eStatus = MB_ENOERR;
    
    /* check preconditions */
    if( ( ucSlaveAddress == MB_ADDRESS_BROADCAST ) ||
        ( ucSlaveAddress < MB_ADDRESS_MIN ) || ( ucSlaveAddress > MB_ADDRESS_MAX ) )
    {
        eStatus = MB_EINVAL;
    }
    else
    {
        peConBlock->ucMBAddress = ucSlaveAddress;
        switch ( eMode )
        {
#if MB_SLAVE_RTU_ENABLED > 0
        case MB_RTU:
            peConBlock->pvMBFrameStartCur = eMBRTUStart;
            peConBlock->pvMBFrameStopCur = eMBRTUStop;
            peConBlock->peMBFrameSendCur = eMBRTUSend;
            peConBlock->peMBFrameReceiveCur = eMBRTUReceive;
            peConBlock->pvMBFrameCloseCur = NULL;
            //peConBlock->pvMBFrameCloseCur = MB_PORT_HAS_CLOSE ? peConBlock->port.vMBPortClose : NULL;
        
            peConBlock->pxMBFrameCBByteReceived = xMBRTUReceiveFSM;
            peConBlock->pxMBFrameCBTransmitterEmpty = xMBRTUTransmitFSM;
            peConBlock->pxMBPortCBTimerExpired = xMBRTUTimerT35Expired;

            eStatus = eMBRTUInit( peConBlock, peConBlock->ucMBAddress, ucPort, ulBaudRate, eParity );
            break;
#endif
#if MB_SLAVE_ASCII_ENABLED > 0
        case MB_ASCII:
            pvMBFrameStartCur = eMBASCIIStart;
            pvMBFrameStopCur = eMBASCIIStop;
            peMBFrameSendCur = eMBASCIISend;
            peMBFrameReceiveCur = eMBASCIIReceive;
            pvMBFrameCloseCur = MB_PORT_HAS_CLOSE ? vMBPortClose : NULL;
            pxMBFrameCBByteReceived = xMBASCIIReceiveFSM;
            pxMBFrameCBTransmitterEmpty = xMBASCIITransmitFSM;
            pxMBPortCBTimerExpired = xMBASCIITimerT1SExpired;

            eStatus = eMBASCIIInit( ucMBAddress, ucPort, ulBaudRate, eParity );
            break;
#endif
        default:
            eStatus = MB_EINVAL;
            break;
        }

        if( eStatus == MB_ENOERR )
        {
            if( !peConBlock->port.pPortEventInit( &peConBlock->port ) )
            {
                /* port dependent event module initalization failed. */
                eStatus = MB_EPORTERR;
            }
            else
            {
                peConBlock->eMBCurrentMode = eMode;
                peConBlock->state = MB_STATE_DISABLED;
            }
        }
    }
    return eStatus;
}

#if MB_SLAVE_TCP_ENABLED > 0
eMBErrorCode
eMBTCPInit( eMBControlBlockSlave* peConBlock, USHORT ucTCPPort )
{
    eMBErrorCode    eStatus = MB_ENOERR;

    if( ( eStatus = eMBTCPDoInit( peConBlock, ucTCPPort ) ) != MB_ENOERR )
    {
        eStatus = MB_EPORTERR;
    }
    else if( !peConBlock->port.pPortEventInit( &peConBlock->port ) )
    {
        /* Port dependent event module initalization failed. */
        eStatus = MB_EPORTERR;
    }
    else
    {
        peConBlock->pvMBFrameStartCur = eMBTCPStart;
        peConBlock->pvMBFrameStopCur = eMBTCPStop;
        peConBlock->peMBFrameReceiveCur = eMBTCPReceive;
        peConBlock->peMBFrameSendCur = eMBTCPSend;
        peConBlock->pvMBFrameCloseCur = NULL;
		peConBlock->ucMBAddress = MB_TCP_PSEUDO_ADDRESS;
        peConBlock->eMBCurrentMode = MB_TCP;
		peConBlock->state = MB_STATE_DISABLED;
        eStatus = MB_ENOERR;
    }
    return eStatus;
}
#endif

eMBErrorCode
eMBRegisterCB( eMBControlBlockSlave* peConBlock, UCHAR ucFunctionCode, pxMBFunctionHandler pxHandler, pxMBFunctionCBHandler pxCBHandler)
{
    int             i;
    eMBErrorCode    eStatus;

    if( ( 0 < ucFunctionCode ) && ( ucFunctionCode <= 127 ) )
    {
        ENTER_CRITICAL_SECTION(  );
        if( pxHandler != NULL )
        {
            for( i = 0; i < MB_FUNC_HANDLERS_MAX; i++ )
            {
                if( ( peConBlock->xFuncHandlers[i].pxHandler == NULL ) ||
                    ( peConBlock->xFuncHandlers[i].pxHandler == pxHandler ) )
                {
                    peConBlock->xFuncHandlers[i].ucFunctionCode = ucFunctionCode;
                    peConBlock->xFuncHandlers[i].pxHandler = pxHandler;
                    peConBlock->xFuncHandlers[i].pxCBHandle = pxCBHandler;
                    break;
                }
            }
            eStatus = ( i != MB_FUNC_HANDLERS_MAX ) ? MB_ENOERR : MB_ENORES;
        }
        else
        {
            for( i = 0; i < MB_FUNC_HANDLERS_MAX; i++ )
            {
                if( peConBlock->xFuncHandlers[i].ucFunctionCode == ucFunctionCode )
                {
                    peConBlock->xFuncHandlers[i].ucFunctionCode = 0;
                    peConBlock->xFuncHandlers[i].pxHandler = NULL;
                    peConBlock->xFuncHandlers[i].pxCBHandle = NULL;
                    break;
                }
            }
            /* Remove can't fail. */
            eStatus = MB_ENOERR;
        }
        EXIT_CRITICAL_SECTION(  );
    }
    else
    {
        eStatus = MB_EINVAL;
    }
    return eStatus;
}

eMBErrorCode
eMBClose( eMBControlBlockSlave* peConBlock )
{
    eMBErrorCode eStatus = MB_ENOERR;

    if( peConBlock->state == MB_STATE_DISABLED )
    {
        if( peConBlock->pvMBFrameCloseCur != NULL )
        {
            peConBlock->pvMBFrameCloseCur( peConBlock );
        }
    }
    else
    {
        eStatus = MB_EILLSTATE;
    }
    return eStatus;
}


eMBErrorCode
eMBEnable( eMBControlBlockSlave* peConBlock )
{
    eMBErrorCode    eStatus = MB_ENOERR;

    if( peConBlock->state == MB_STATE_DISABLED )
    {
        /* Activate the protocol stack. */
        peConBlock->pvMBFrameStartCur( peConBlock );
        peConBlock->state = MB_STATE_ENABLED;
    }
    else
    {
        eStatus = MB_EILLSTATE;
    }
    return eStatus;
}

eMBErrorCode
eMBDisable( eMBControlBlockSlave* peConBlock )
{
    eMBErrorCode    eStatus;

    if( peConBlock->state == MB_STATE_ENABLED )
    {
        peConBlock->pvMBFrameStopCur( peConBlock );
        peConBlock->state = MB_STATE_DISABLED;
        eStatus = MB_ENOERR;
    }
    else if( peConBlock->state == MB_STATE_DISABLED )
    {
        eStatus = MB_ENOERR;
    }
    else
    {
        eStatus = MB_EILLSTATE;
    }
    return eStatus;
}

eMBErrorCode eMBPoll( eMBControlBlockSlave* peConBlock )
{
    static UCHAR   *ucMBFrame;
    static UCHAR    ucRcvAddress;
    static UCHAR    ucFunctionCode;
    static USHORT   usLength;
    static eMBException eException;

    int             i;
    eMBErrorCode    eStatus = MB_ENOERR;
    eMBEventType    eEvent;

    /* Check if the protocol stack is ready. */
    if( peConBlock->state != MB_STATE_ENABLED )
    {
        return MB_EILLSTATE;
    }

    /* Check if there is a event available. If not return control to caller.
     * Otherwise we will handle the event. */
    if( peConBlock->port.pPortEventGet( &peConBlock->port, &eEvent ) == TRUE )
    {
        switch ( eEvent )
        {
        case EV_READY:
            break;

        case EV_FRAME_RECEIVED:
            eStatus = peConBlock->peMBFrameReceiveCur(peConBlock, &ucRcvAddress, &ucMBFrame, &usLength );
            if( eStatus == MB_ENOERR )
            {
                /* Check if the frame is for us. If not ignore the frame. */
                if( ( ucRcvAddress == peConBlock->ucMBAddress ) || ( ucRcvAddress == MB_ADDRESS_BROADCAST ) )
                {
                    ( void )peConBlock->port.pPortEventPost( &peConBlock->port, EV_EXECUTE );
                }
            }
            break;

        case EV_EXECUTE:
            ucFunctionCode = ucMBFrame[MB_PDU_FUNC_OFF];
            eException = MB_EX_ILLEGAL_FUNCTION;
            for( i = 0; i < MB_FUNC_HANDLERS_MAX; i++ )
            {
                /* No more function handlers registered. Abort. */
                if( peConBlock->xFuncHandlers[i].ucFunctionCode == 0 )
                {
                    break;
                }
                else if( peConBlock->xFuncHandlers[i].ucFunctionCode == ucFunctionCode )
                {
                    eException = peConBlock->xFuncHandlers[i].pxHandler( peConBlock, ucMBFrame, &usLength );
                    break;
                }
            }

            /* If the request was not sent to the broadcast address we
             * return a reply. */
            if( ucRcvAddress != MB_ADDRESS_BROADCAST )
            {
                if( eException != MB_EX_NONE )
                {
                    /* An exception occured. Build an error frame. */
                    usLength = 0;
                    ucMBFrame[usLength++] = ( UCHAR )( ucFunctionCode | MB_FUNC_ERROR );
                    ucMBFrame[usLength++] = eException;
                }
                eStatus = peConBlock->peMBFrameSendCur(peConBlock, peConBlock->ucMBAddress, ucMBFrame, usLength );
            }
            break;

        case EV_FRAME_SENT:
            break;
        }
    }
    return MB_ENOERR;
}
