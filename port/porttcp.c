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
 * File: $Id: porttcp.c,v 1.0 2020/01/13 rainbowyu $
 */
#define LOG_TAG "mb.tcp"
#define LOG_LVL LOG_LVL_INFO
/* ----------------------- System includes ----------------------------------*/
#include <stdio.h>
#include "ulog.h"
#include "port.h"

/* ----------------------- lwIP includes ------------------------------------*/
#include "lwip/api.h"
#include "lwip/tcp.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "mb.h"
#include "mbport.h"
#include "mbtcp.h"
#include "porttcp.h"

/* ----------------------- MBAP Header --------------------------------------*/
#define MB_TCP_UID          6
#define MB_TCP_LEN          4
#define MB_TCP_FUNC         7

/* ----------------------- Defines  -----------------------------------------*/
#define MB_TCP_DEFAULT_PORT 502 /* TCP listening port. */


/* ----------------------- Prototypes ---------------------------------------*/
void            vMBPortEventClose( void );

/* ----------------------- Static functions ---------------------------------*/
static err_t    prvxMBTCPPortAccept( void *pvArg, struct tcp_pcb *pxPCB, err_t xErr );
static err_t    prvxMBTCPPortReceive( void *pvArg, struct tcp_pcb *pxPCB, struct pbuf *p,
                                      err_t xErr );
static void     prvvMBTCPPortError( void *pvArg, err_t xErr );

/* ----------------------- Begin implementation -----------------------------*/
BOOL
xMBTCPPortInit( struct xMBPortSlave* pPortSlave, USHORT usTCPPort )
{
    struct tcp_pcb *pxPCBListenNew, *pxPCBListenOld;
    BOOL            bOkay = FALSE;
    USHORT          usPort;

	pPortSlave->type.portTCP.pcbListenParam.pPortSlave = pPortSlave;
	pPortSlave->type.portTCP.pcbClientParam.pPortSlave = pPortSlave;
    if( usTCPPort == 0 )
    {
        usPort = MB_TCP_DEFAULT_PORT;
    }
    else
    {
        usPort = ( USHORT ) usTCPPort;
    }

    if( ( pxPCBListenNew = pxPCBListenOld = tcp_new(  ) ) == NULL )
    {
        /* Can't create TCP socket. */
        bOkay = FALSE;
    }
    else if( tcp_bind( pxPCBListenNew, IP_ADDR_ANY, ( u16_t ) usPort ) != ERR_OK )
    {
        /* Bind failed - Maybe illegal port value or in use. */
        ( void )tcp_close( pxPCBListenOld );
        bOkay = FALSE;
    }
    else if( ( pxPCBListenNew = tcp_listen( pxPCBListenNew ) ) == NULL )
    {
        ( void )tcp_close( pxPCBListenOld );
        bOkay = FALSE;
    }
    else
    {
        /* Register callback function for new clients. */
        tcp_accept( pxPCBListenNew, prvxMBTCPPortAccept );

		/* Set callback argument later used in the error handler. */
		pPortSlave->type.portTCP.pcbListenParam.pPcbCurrent = pxPCBListenNew;
        tcp_arg( pxPCBListenNew,  &pPortSlave->type.portTCP.pcbListenParam);
		
        /* Everything okay. Set global variable. */
        pPortSlave->type.portTCP.pxPcbListen = pxPCBListenNew;

#ifdef MB_TCP_DEBUG
        LOG_I( "MBTCP-ACCEPT", "Protocol stack ready.\r\n" );
#endif
    }
    bOkay = TRUE;
    return bOkay;
}

void
prvvMBPortReleaseClient( struct xMBPortSlave* pPortSlave, struct tcp_pcb *pxPCB )
{
    if( pxPCB != NULL )
    {
        if( tcp_close( pxPCB ) != ERR_OK )
        {
            tcp_abort( pxPCB );
        }
        ENTER_CRITICAL_SECTION(  );
		if( pxPCB == pPortSlave->type.portTCP.pxPcbClient )
        {
#ifdef MB_TCP_DEBUG
            LOG_I("MBTCP-CLOSE", "Closed connection to %d.%d.%d.%d.\r\n",
                        ip4_addr1( &( pxPCB->remote_ip ) ),
                        ip4_addr2( &( pxPCB->remote_ip ) ),
                        ip4_addr3( &( pxPCB->remote_ip ) ), ip4_addr4( &( pxPCB->remote_ip ) ) );
#endif
            pPortSlave->type.portTCP.pxPcbClient = NULL;
        }
		if( pxPCB == pPortSlave->type.portTCP.pxPcbListen )
        {
            pPortSlave->type.portTCP.pxPcbListen = NULL;
        }
        EXIT_CRITICAL_SECTION(  );
    }
}

void
vMBTCPPortDisable( struct xMBPortSlave* pPortSlave )
{
    prvvMBPortReleaseClient( pPortSlave, pPortSlave->type.portTCP.pxPcbClient );
}

err_t
prvxMBTCPPortAccept( void *pvArg, struct tcp_pcb *pxPCB, err_t xErr )
{
    err_t error;
	TcpPcbCBParam* pParam = (TcpPcbCBParam*)pvArg;
	struct tcp_pcb *pxPCBClient = pParam->pPortSlave->type.portTCP.pxPcbClient;
	struct eMBControlBlockSlave* peConBlock = pParam->pPortSlave->peMBConBSlave;
    if( xErr != ERR_OK )
    {
        return xErr;
    }

    /* We can handle only one client. */
    if( pxPCBClient == NULL )
    {
		
        /* Register the client. */
        pxPCBClient = pxPCB;

        /* Set up the receive function prvxMBTCPPortReceive( ) to be called when data
         * arrives.
         */
        tcp_recv( pxPCB, prvxMBTCPPortReceive );

        /* Register error handler. */
        tcp_err( pxPCB, prvvMBTCPPortError );

        /* Set callback argument later used in the error handler. */
		pParam->pPortSlave->type.portTCP.pxPcbClient = pxPCB;
		pParam->pPortSlave->type.portTCP.pcbClientParam.pPcbCurrent = pxPCB;
        tcp_arg( pxPCB, &pParam->pPortSlave->type.portTCP.pcbClientParam);

        /* Reset the buffers and state variables. */
		peConBlock->usRcvBufferPos = 0;
        

#ifdef MB_TCP_DEBUG
        LOG_I( "MBTCP-ACCEPT", "Accepted new client %d.%d.%d.%d\r\n",
                    ip4_addr1( &( pxPCB->remote_ip ) ),
                    ip4_addr2( &( pxPCB->remote_ip ) ),
                    ip4_addr3( &( pxPCB->remote_ip ) ), ip4_addr4( &( pxPCB->remote_ip ) ) );
#endif

        error = ERR_OK;
    }
    else
    {
        prvvMBPortReleaseClient( pParam->pPortSlave, pxPCB );
        error = ERR_OK;
    }
    return error;
}

/* Called in case of an unrecoverable error. In any case we drop the client
 * connection. */
void
prvvMBTCPPortError( void *pvArg, err_t xErr )
{
	TcpPcbCBParam* pParam = (TcpPcbCBParam*)pvArg;
    struct tcp_pcb *pxPCB = pParam->pPcbCurrent;

    if( pxPCB != NULL )
    {
#ifdef MB_TCP_DEBUG
        LOG_I( "Error with client connection! Droping it." );
#endif
        prvvMBPortReleaseClient( pParam->pPortSlave, pxPCB );
    }
}

err_t
prvxMBTCPPortReceive( void *pvArg, struct tcp_pcb *pxPCB, struct pbuf *p, err_t xErr )
{
    USHORT          usLength;

    err_t           error = xErr;
	TcpPcbCBParam* pParam = (TcpPcbCBParam*)pvArg;
	struct eMBControlBlockSlave* peConBlock = pParam->pPortSlave->peMBConBSlave;
    if( error != ERR_OK )
    {
		LOG_E( "Error with client receive:%d", error );
        return error;
    }

    /* If pbuf is NULL then remote end has closed connection. */
    if( p == NULL )
    {
        prvvMBPortReleaseClient( pParam->pPortSlave, pxPCB );
        return ERR_OK;
    }

    /* Acknowledge that we have received the data bytes. */
    tcp_recved( pxPCB, p->len );

    /* Check for internal buffer overflow. In case of an error drop the
     * client. */
	if( ( peConBlock->usRcvBufferPos + p->len ) >= MB_TCP_BUF_SIZE )
    {
        prvvMBPortReleaseClient( pParam->pPortSlave, pxPCB );
        error = ERR_OK;
    }
    else
    {
		memcpy( (void*)&peConBlock->ucBuf[peConBlock->usRcvBufferPos], p->payload, p->len );
        peConBlock->usRcvBufferPos += p->len;

        /* If we have received the MBAP header we can analyze it and calculate
         * the number of bytes left to complete the current request. If complete
         * notify the protocol stack.
         */
		if( peConBlock->usRcvBufferPos >= MB_TCP_FUNC )
        {
            /* Length is a byte count of Modbus PDU (function code + data) and the
             * unit identifier. */
            usLength = peConBlock->ucBuf[MB_TCP_LEN] << 8U;
            usLength |= peConBlock->ucBuf[MB_TCP_LEN + 1];

            /* Is the frame already complete. */
			if( peConBlock->usRcvBufferPos < ( MB_TCP_UID + usLength ) )
            {
            }
			else if( peConBlock->usRcvBufferPos == ( MB_TCP_UID + usLength ) )
            {
#ifdef MB_TCP_DEBUG
				ulog_hexdump(LOG_TAG, 16, (uint8_t *)&peConBlock->ucBuf[0], peConBlock->usRcvBufferPos);
//                prvvMBTCPLogFrame( "MBTCP-RECV", &aucTCPBuf[0], usTCPBufPos );
#endif
				( void )peConBlock->port.pPortEventPost( &peConBlock->port, EV_FRAME_RECEIVED );
            }
            else
            {
#ifdef MB_TCP_DEBUG
                LOG_I( "Received to many bytes! Droping client." );
#endif
                /* This should not happen. We can't deal with such a client and
                 * drop the connection for security reasons.
                 */
                prvvMBPortReleaseClient( pParam->pPortSlave, pxPCB );
            }
        }
    }
    pbuf_free( p );
    return error;
}

BOOL
xMBTCPPortSendResponse( struct xMBPortSlave* pPortSlave, const UCHAR * pucMBTCPFrame, USHORT usTCPLength )
{
    BOOL            bFrameSent = FALSE;
    struct tcp_pcb *pxPCBClient = pPortSlave->type.portTCP.pxPcbClient;
    if( pxPCBClient )
    {
        /* Make sure we can send the packet. */
        assert( tcp_sndbuf( pxPCBClient ) >= usTCPLength );

        if( tcp_write( pxPCBClient, pucMBTCPFrame, ( u16_t ) usTCPLength, NETCONN_COPY ) == ERR_OK )
        {
#ifdef MB_TCP_DEBUG
			ulog_hexdump(LOG_TAG, 16, (uint8_t *)pucMBTCPFrame, usTCPLength);
#endif
            /* Make sure data gets sent immediately. */
            ( void )tcp_output( pxPCBClient );
            bFrameSent = TRUE;
        }
        else
        {
            /* Drop the connection in case of an write error. */
            prvvMBPortReleaseClient( pPortSlave, pxPCBClient );
        }
    }
    return bFrameSent;
}
