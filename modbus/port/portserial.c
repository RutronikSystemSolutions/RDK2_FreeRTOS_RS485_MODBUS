/*
 * FreeModbus Libary: BARE Port
 * Copyright (C) 2006 Christian Walter <wolti@sil.at>
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
 * File: $Id: portserial.c,v 1.1 2006/08/22 21:35:13 wolti Exp $
 */
#include "port.h"

 
/* ----------------------- Modbus includes ----------------------------------*/
#include "mb.h"
#include "mbport.h"

/*PSoC62 Header Files*/
#include "cycfg_pins.h"
#include "cyhal.h"
#include "cy_pdl.h"
#include "cy_gpio.h"
#include "cycfg_routing.h"

/* ----------------------- FreeRTOS -----------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"

#define INT_PRIORITY	5
 
/* ----------------------- static functions ---------------------------------*/
//static void prvvUARTTxReadyISR( void );
//static void prvvUARTRxISR( void );

static void uart_event_handler(void *handler_arg, cyhal_uart_event_t event);
 
/* -----------------------    variables     ---------------------------------*/
cyhal_uart_t rs485_obj;
cyhal_uart_cfg_t mb_uart_config =
		{
				.data_bits = 8,
				.stop_bits = 1,
				.parity = CYHAL_UART_PARITY_NONE,
				.rx_buffer = NULL,
				.rx_buffer_size = 0,
		};

/*RS485 Tx-Enable pin configuraton*/
const cy_stc_gpio_pin_config_t MB_UART_CTS_config =
{
	.outVal = 1,
	.driveMode = CY_GPIO_DM_STRONG_IN_OFF,
	.hsiom = HSIOM_SEL_ACT_6,
	.intEdge = CY_GPIO_INTR_DISABLE,
	.intMask = 0UL,
	.vtrip = CY_GPIO_VTRIP_CMOS,
	.slewRate = CY_GPIO_SLEW_FAST,
	.driveSel = CY_GPIO_DRIVE_1_2,
	.vregEn = 0UL,
	.ibufMode = 0UL,
	.vtripSel = 0UL,
	.vrefSel = 0UL,
	.vohSel = 0UL,
};
const cyhal_resource_inst_t MB_UART_CTS_obj =
{
	.type = CYHAL_RSC_GPIO,
	.block_num = 3U,
	.channel_num = 3U,
};

/* ----------------------- Start implementation -----------------------------*/
void
vMBPortSerialEnable( BOOL xRxEnable, BOOL xTxEnable )
{
  /* If xRXEnable enable serial receive interrupts. If xTxENable enable
  * transmitter empty interrupts.
  */
  
  /*Enable/Disable Registered Events*/
  if (xRxEnable) 
  {
	  cyhal_uart_enable_event(&rs485_obj,(cyhal_uart_event_t)(CYHAL_UART_IRQ_RX_NOT_EMPTY), INT_PRIORITY, true);
  } 
  else 
  {    
	  cyhal_uart_enable_event(&rs485_obj,(cyhal_uart_event_t)(CYHAL_UART_IRQ_RX_NOT_EMPTY), INT_PRIORITY, false);
  }
  
  if (xTxEnable) 
  {
      cyhal_uart_enable_event(&rs485_obj,(cyhal_uart_event_t)(CYHAL_UART_IRQ_TX_EMPTY), INT_PRIORITY, true);
  } 
  else 
  {
	  cyhal_uart_enable_event(&rs485_obj,(cyhal_uart_event_t)(CYHAL_UART_IRQ_TX_EMPTY), INT_PRIORITY, false);
  }  
}
 
BOOL
xMBPortSerialInit( UCHAR ucPORT, ULONG ulBaudRate, UCHAR ucDataBits, eMBParity eParity )
{
	cy_rslt_t result;
	uint32_t actual_baud;

	(void)ucPORT;

	/*Configure data bits*/
	mb_uart_config.data_bits = (uint32_t)ucDataBits;

	/*Configure parity*/
	if(eParity == MB_PAR_EVEN)
	{
		mb_uart_config.parity = CYHAL_UART_PARITY_EVEN;
	}
	else if(eParity == MB_PAR_ODD)
	{
		mb_uart_config.parity = CYHAL_UART_PARITY_ODD;
	}
	else
	{
		mb_uart_config.parity = CYHAL_UART_PARITY_NONE;
	}

	/* RS485 MODBUS UART Initialization*/
	result = cyhal_uart_init(&rs485_obj, RS485_TX, RS485_RX, NC, NC, NULL, &mb_uart_config);
	if(result != CY_RSLT_SUCCESS)
	{
		return FALSE;
	}

	/*Configure baud rate*/
	result = cyhal_uart_set_baud(&rs485_obj, ulBaudRate, &actual_baud);
	if(result != CY_RSLT_SUCCESS)
	{
		return FALSE;
	}

	/*TX-Enable pin configuration*/
	result = Cy_GPIO_Pin_Init(GPIO_PRT6, 3U, &MB_UART_CTS_config);
	if(result != CY_RSLT_SUCCESS)
	{
		return FALSE;
	}
	result = cyhal_hwmgr_reserve(&MB_UART_CTS_obj);
	if(result != CY_RSLT_SUCCESS)
	{
		return FALSE;
	}


    /* The UART callback handler registration */
    cyhal_uart_register_callback(&rs485_obj, uart_event_handler, NULL);

	return TRUE;
}
 
BOOL
xMBPortSerialPutByte( CHAR ucByte )
{
  /* Put a byte in the UARTs transmit buffer. This function is called
  * by the protocol stack if pxMBFrameCBTransmitterEmpty( ) has been
  * called. */
	size_t tx_length = 1;

	return (CY_RSLT_SUCCESS == cyhal_uart_write( &rs485_obj, &ucByte, &tx_length ));
}
 
BOOL
xMBPortSerialGetByte( CHAR * pucByte )
{
  /* Return the byte in the UARTs receive buffer. This function is called
  * by the protocol stack after pxMBFrameCBByteReceived( ) has been called.
  */
	size_t rx_length = 1;

	return (CY_RSLT_SUCCESS == cyhal_uart_read( &rs485_obj, pucByte, &rx_length ));
}

/* Event handler callback function */
static void  uart_event_handler(void *handler_arg, cyhal_uart_event_t event)
{
    (void) handler_arg;
    BOOL bTaskWoken = FALSE;

    vMBPortSetWithinException( TRUE );

    if ((event & CYHAL_UART_IRQ_TX_EMPTY ) == CYHAL_UART_IRQ_TX_EMPTY)
    {
        /* The tx hardware buffer is empty */
    	bTaskWoken = pxMBFrameCBTransmitterEmpty();
    }
    else if ((event & CYHAL_UART_IRQ_RX_NOT_EMPTY) == CYHAL_UART_IRQ_RX_NOT_EMPTY)
    {
        /* All Rx data has been received */
    	bTaskWoken = pxMBFrameCBByteReceived();
    }

    vMBPortSetWithinException( FALSE );

    portEND_SWITCHING_ISR( bTaskWoken ? pdTRUE : pdFALSE );
}
 
/* Create an interrupt handler for the transmit buffer empty interrupt
* (or an equivalent) for your target processor. This function should then
* call pxMBFrameCBTransmitterEmpty( ) which tells the protocol stack that
* a new character can be sent. The protocol stack will then call 
* xMBPortSerialPutByte( ) to send the character.
 
static void prvvUARTTxReadyISR( void )
{
pxMBFrameCBTransmitterEmpty(  );
}
*/
 
/* Create an interrupt handler for the receive interrupt for your target
* processor. This function should then call pxMBFrameCBByteReceived( ). The
* protocol stack will then call xMBPortSerialGetByte( ) to retrieve the
* character.
 
static void prvvUARTRxISR( void )
{
pxMBFrameCBByteReceived(  );
}
*/
