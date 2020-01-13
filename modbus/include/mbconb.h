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
 */

#ifndef _MB_CONB_H
#define _MB_CONB_H

#include "port.h"
#include "mbdef.h"
#include "mbproto.h"
#include "mbport.h"
#include "mbframe.h"
/* ----------------------- Type definitions ---------------------------------*/
typedef enum{
    MB_STATE_NOT_INITIALIZED,
    MB_STATE_ENABLED,
    MB_STATE_DISABLED
}eMBState;

typedef enum
{
    MB_STATE_RX_INIT,              /*!< Receiver is in initial state. */
    MB_STATE_RX_IDLE,              /*!< Receiver is in idle state. */
    MB_STATE_RX_RCV,               /*!< Frame is beeing received. */
    MB_STATE_RX_ERROR              /*!< If the frame is invalid. */
}eMBRcvState;

typedef enum
{
    MB_STATE_TX_IDLE,              /*!< Transmitter is in idle state. */
    MB_STATE_TX_XMIT               /*!< Transmitter is in transfer state. */
}eMBSndState;

typedef struct eMBControlBlockSlave
{
    UCHAR ucMBAddress;
    eMBMode eMBCurrentMode;
    eMBState state;
    
    volatile eMBSndState eSndState;
    volatile eMBRcvState eRcvState;
    volatile UCHAR* ucBuf;
    volatile UCHAR *pucSndBufferCur;
    volatile USHORT usSndBufferCount;
    volatile USHORT usRcvBufferPos;

    /* Functions pointer which are initialized in eMBInit( ). Depending on the
    * mode (RTU or ASCII) the are set to the correct implementations.
    * Using for Modbus Slave
    */
    /* ----------------------- Prototypes  0-------------------------------------*/
    void ( *pvMBFrameStartCur ) ( struct eMBControlBlockSlave* peConBlock );
    void ( *pvMBFrameStopCur ) ( struct eMBControlBlockSlave* peConBlock );
    eMBErrorCode( *peMBFrameReceiveCur ) ( struct eMBControlBlockSlave* peConBlock,
                                           UCHAR * pucRcvAddress,
                                           UCHAR ** pucFrame,
                                           USHORT * pusLength );
    eMBErrorCode( *peMBFrameSendCur ) ( struct eMBControlBlockSlave* peConBlock,
                                        UCHAR slaveAddress,
                                        const UCHAR * pucFrame,
                                        USHORT usLength );
    void( *pvMBFrameCloseCur ) ( struct eMBControlBlockSlave* peConBlock );
                                        
    /* ----------------------- Callback for the protocol stack ------------------*/
    /*!
    * \brief Callback function for the porting layer when a new byte is
    *   available.
    *
    * Depending upon the mode this callback function is used by the RTU or
    * ASCII transmission layers. In any case a call to xMBPortSerialGetByte()
    * must immediately return a new character.
    *
    * \return <code>TRUE</code> if a event was posted to the queue because
    *   a new byte was received. The port implementation should wake up the
    *   tasks which are currently blocked on the eventqueue.
    */
    BOOL (*pxMBFrameCBByteReceived ) ( struct eMBControlBlockSlave* peConBlock );
    BOOL (*pxMBFrameCBTransmitterEmpty ) ( struct eMBControlBlockSlave* peConBlock );
    BOOL (*pxMBPortCBTimerExpired ) ( struct eMBControlBlockSlave* peConBlock );

    xMBPortSlave port;
    /* An array of Modbus functions handlers which associates Modbus function
     * codes with implementing functions.
     */
    xMBFunction xFuncHandlers[MB_FUNC_HANDLERS_MAX];
}eMBControlBlockSlave;

#endif
