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
 * File: $Id: portserial.c,v 1.0 2020/01/13 rainbowyu $
 */

#include "port.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "mb.h"
#include "mbport.h"
#include "rtdevice.h"
#include "board.h"

/* ----------------------- Defines ------------------------------------------*/
/* serial transmit event */
#define EVENT_SERIAL_TRANS_START    (1<<0)

/* STM32 uart driver */
struct drv_uart{
    UART_HandleTypeDef UartHandle;
    IRQn_Type irq;
    void *user_data;
};

/* ----------------------- static functions ---------------------------------*/
static rt_err_t serial_rx_ind(rt_device_t dev, rt_size_t size);
static void serial_soft_trans_irq(void* parameter);

/* ----------------------- Start implementation -----------------------------*/
BOOL 
xMBPortSerialInit( struct xMBPortSlave* pPortSlave, UCHAR ucPORT, ULONG ulBaudRate, UCHAR ucDataBits,
        eMBParity eParity){
    /**
     * set 485 mode receive and transmit control IO
     * @note pPortSlave->xSlaveOsSerial.control_pin need be defined by user
     */
    if(pPortSlave->type.portRTU.xSlaveOsSerial.control_pin!=0xff){
        rt_pin_mode(pPortSlave->type.portRTU.xSlaveOsSerial.control_pin, PIN_MODE_OUTPUT);
    }

    /* set serial name */
    if (ucPORT == 1) {
#if defined(BSP_USING_UART1) || defined(BSP_USING_REMAP_UART1)
        extern struct rt_serial_device serial1;
        pPortSlave->type.portRTU.xSlaveOsSerial.serial = &serial1;
#endif
    } else if (ucPORT == 2) {
#if defined(BSP_USING_UART2)
        extern struct rt_serial_device serial2;
        pPortSlave->type.portRTU.xSlaveOsSerial.serial = &serial2;
#endif
    } else if (ucPORT == 3) {
#if defined(BSP_USING_UART3)
        extern struct rt_serial_device serial3;
        pPortSlave->type.portRTU.xSlaveOsSerial.serial = &serial3;
#endif
    } else if (ucPORT == 6) {
#if defined(BSP_USING_UART6)
        extern struct rt_serial_device serial6;
        pPortSlave->type.portRTU.xSlaveOsSerial.serial = &serial6;
#endif
    }
    /* set serial configure parameter */
    pPortSlave->type.portRTU.xSlaveOsSerial.serial->config.baud_rate = ulBaudRate;
    pPortSlave->type.portRTU.xSlaveOsSerial.serial->config.stop_bits = STOP_BITS_1;
    switch(eParity){
        case MB_PAR_NONE: {
            pPortSlave->type.portRTU.xSlaveOsSerial.serial->config.data_bits = DATA_BITS_8;
            pPortSlave->type.portRTU.xSlaveOsSerial.serial->config.parity = PARITY_NONE;
            break;
        }
        case MB_PAR_ODD: {
            pPortSlave->type.portRTU.xSlaveOsSerial.serial->config.data_bits = DATA_BITS_9;
            pPortSlave->type.portRTU.xSlaveOsSerial.serial->config.parity = PARITY_ODD;
            break;
        }
        case MB_PAR_EVEN: {
            pPortSlave->type.portRTU.xSlaveOsSerial.serial->config.data_bits = DATA_BITS_9;
            pPortSlave->type.portRTU.xSlaveOsSerial.serial->config.parity = PARITY_EVEN;
            break;
        }
    }
    /* set serial configure */
    pPortSlave->type.portRTU.xSlaveOsSerial.serial->ops->configure(pPortSlave->type.portRTU.xSlaveOsSerial.serial,
            &(pPortSlave->type.portRTU.xSlaveOsSerial.serial->config));

    /* set user data */
    struct drv_uart* pDrvUart = (struct drv_uart *)pPortSlave->type.portRTU.xSlaveOsSerial.serial->parent.user_data;
    pDrvUart->user_data = pPortSlave;
    
    /* open serial device */
    if (!rt_device_open(&pPortSlave->type.portRTU.xSlaveOsSerial.serial->parent,
            RT_DEVICE_OFLAG_RDWR | RT_DEVICE_FLAG_INT_RX)) {
        rt_device_set_rx_indicate(&pPortSlave->type.portRTU.xSlaveOsSerial.serial->parent, serial_rx_ind);
    } else {
        return FALSE;
    }

    /* software initialize */
    rt_event_init(&pPortSlave->type.portRTU.xSlaveOsSerial.event_serial, (const char*)pPortSlave->portName, RT_IPC_FLAG_PRIO);
    rt_thread_init(&pPortSlave->type.portRTU.xSlaveOsSerial.thread_serial_soft_trans_irq,
                   (const char*)pPortSlave->portName,
                   serial_soft_trans_irq,
                   pPortSlave,
                   pPortSlave->type.portRTU.xSlaveOsSerial.serial_soft_trans_irq_stack,
                   sizeof(pPortSlave->type.portRTU.xSlaveOsSerial.serial_soft_trans_irq_stack),
                   10, 5);
    rt_thread_startup(&pPortSlave->type.portRTU.xSlaveOsSerial.thread_serial_soft_trans_irq);

    return TRUE;
}

void
vMBPortSerialEnable( struct xMBPortSlave* pPortSlave, BOOL xRxEnable, BOOL xTxEnable){
    rt_uint32_t recved_event;
    if (xRxEnable){
        /* enable RX interrupt */
        pPortSlave->type.portRTU.xSlaveOsSerial.serial->ops->control(pPortSlave->type.portRTU.xSlaveOsSerial.serial, RT_DEVICE_CTRL_SET_INT, (void *)RT_DEVICE_FLAG_INT_RX);
        /* switch 485 to receive mode */
        //引脚需要反向
        if(pPortSlave->type.portRTU.xSlaveOsSerial.control_pin!=0xff)
            rt_pin_write(pPortSlave->type.portRTU.xSlaveOsSerial.control_pin, PIN_HIGH);
    }
    else{
        /* switch 485 to transmit mode */
        //引脚需要反向
        if(pPortSlave->type.portRTU.xSlaveOsSerial.control_pin!=0xff)
            rt_pin_write(pPortSlave->type.portRTU.xSlaveOsSerial.control_pin, PIN_LOW);
        
        /* disable RX interrupt */
        pPortSlave->type.portRTU.xSlaveOsSerial.serial->ops->control(pPortSlave->type.portRTU.xSlaveOsSerial.serial,
                RT_DEVICE_CTRL_CLR_INT,
                (void *)RT_DEVICE_FLAG_INT_RX);
    }
    if (xTxEnable){
        /* start serial transmit */
        rt_event_send(&pPortSlave->type.portRTU.xSlaveOsSerial.event_serial, EVENT_SERIAL_TRANS_START);
    }
    else{
        /* stop serial transmit */
        rt_event_recv(&pPortSlave->type.portRTU.xSlaveOsSerial.event_serial,
                EVENT_SERIAL_TRANS_START,RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR,
                0,
                &recved_event);
    }
}

void 
vMBPortClose( struct xMBPortSlave* pPortSlave ){
    pPortSlave->type.portRTU.xSlaveOsSerial.serial->parent.close(&(pPortSlave->type.portRTU.xSlaveOsSerial.serial->parent));
}

//新增函数
void 
xMBPortSerialWaitSendOver( struct xMBPortSlave* pPortSlave ){
    struct drv_uart* uart = (struct drv_uart *)pPortSlave->type.portRTU.xSlaveOsSerial.serial->parent.user_data;
    while (__HAL_UART_GET_FLAG(&uart->UartHandle, USART_FLAG_TC) == RESET);
}

BOOL 
xMBPortSerialPutByte( struct xMBPortSlave* pPortSlave, CHAR ucByte){
    pPortSlave->type.portRTU.xSlaveOsSerial.serial->parent.write(&(pPortSlave->type.portRTU.xSlaveOsSerial.serial->parent),
            0, &ucByte, 1);
    return TRUE;
}

BOOL 
xMBPortSerialGetByte( struct xMBPortSlave* pPortSlave, CHAR * pucByte){
    pPortSlave->type.portRTU.xSlaveOsSerial.serial->parent.read(&(pPortSlave->type.portRTU.xSlaveOsSerial.serial->parent),
            0, pucByte, 1);
    return TRUE;
}

/* 
 * Create an interrupt handler for the transmit buffer empty interrupt
 * (or an equivalent) for your target processor. This function should then
 * call pxMBFrameCBTransmitterEmpty( ) which tells the protocol stack that
 * a new character can be sent. The protocol stack will then call 
 * xMBPortSerialPutByte( ) to send the character.
 */
static void
prvvUARTTxReadyISR( xMBPortSlave* pPortSlave ){
    struct eMBControlBlockSlave* peConBlock = pPortSlave->peMBConBSlave;
    peConBlock->pxMBFrameCBTransmitterEmpty( peConBlock );
}

/* 
 * Create an interrupt handler for the receive interrupt for your target
 * processor. This function should then call pxMBFrameCBByteReceived( ). The
 * protocol stack will then call xMBPortSerialGetByte( ) to retrieve the
 * character.
 */
static void
prvvUARTRxISR( xMBPortSlave* pPortSlave ){
    struct eMBControlBlockSlave* peConBlock = pPortSlave->peMBConBSlave;
    peConBlock->pxMBFrameCBByteReceived( peConBlock );
}

/**
 * Software simulation serial transmit IRQ handler.
 *
 * @param parameter parameter
 */
static void 
serial_soft_trans_irq(void* parameter) {
    xMBPortSlave* pPortSlave = (xMBPortSlave*)parameter;
    rt_uint32_t recved_event;
    while (1){
        /* waiting for serial transmit start */
        rt_event_recv(&pPortSlave->type.portRTU.xSlaveOsSerial.event_serial, EVENT_SERIAL_TRANS_START, RT_EVENT_FLAG_OR,
                RT_WAITING_FOREVER, &recved_event);
        /* execute modbus callback */
        prvvUARTTxReadyISR( pPortSlave );
    }
}

/**
 * This function is serial receive callback function
 *
 * @param dev the device of serial
 * @param size the data size that receive
 *
 * @return return RT_EOK
 */
static rt_err_t 
serial_rx_ind(rt_device_t dev, rt_size_t size) {
    struct drv_uart* pDrvUart;
    struct xMBPortSlave* pPortSlave;
    if(dev->user_data != NULL)
        pDrvUart = (struct drv_uart *)dev->user_data;
    else
        return RT_ERROR;
    if(pDrvUart->user_data != NULL)
        pPortSlave = pDrvUart->user_data;
    prvvUARTRxISR( pPortSlave );
    return RT_EOK;
}
