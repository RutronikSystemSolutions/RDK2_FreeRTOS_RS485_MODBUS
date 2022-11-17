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
 * File: $Id: porttimer.c,v 1.1 2006/08/22 21:35:13 wolti Exp $
 */

/* ----------------------- Platform includes --------------------------------*/
#include "port.h"
 
/* ----------------------- Modbus includes ----------------------------------*/
#include "mb.h"
#include "mbport.h"

/* ----------------------- FreeRTOS -----------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"

/*PSoC62 Header Files*/
#include "cycfg_pins.h"
#include "cyhal.h"

/*50us or 20kHz*/
#define TIMER_FREQ	20000
 
/* ----------------------- static functions ---------------------------------*/
//static void prvvTIMERExpiredISR( void );
static void mb_timer_isr(void *callback_arg, cyhal_timer_event_t event);
 
/* -----------------------    variables     ---------------------------------*/
cyhal_timer_t mb_timer_obj;
cyhal_timer_cfg_t mb_timer_cfg =
{
    .compare_value = 0,                 /* Timer compare value, not used */
    .period = 1,                    	/* Timer period set to a large enough value
                                         * compared to event being measured */
    .direction = CYHAL_TIMER_DIR_UP,    /* Timer counts up */
    .is_compare = false,                /* Don't use compare mode */
    .is_continuous = true,              /* Run timer indefinitely */
    .value = 0                          /* Initial value of counter */
};
 
/* ----------------------- Start implementation -----------------------------*/
BOOL
xMBPortTimersInit( USHORT usTim1Timerout50us )
{
	cy_rslt_t result;

	/*Clean previuos configurations*/
	cyhal_timer_free(&mb_timer_obj);

	/*Timer value should not be bigger than a frequency value.*/
	if(usTim1Timerout50us > (TIMER_FREQ - 1))
	{
		return FALSE;
	}

	/*Initialize the timer*/
	result = cyhal_timer_init(&mb_timer_obj, NC, NULL);
	if(result != CY_RSLT_SUCCESS)
	{
		return FALSE;
	}

	/* Set the timeout value*/
	mb_timer_cfg.period = (uint32_t)usTim1Timerout50us;
	result = cyhal_timer_configure(&mb_timer_obj, &mb_timer_cfg);
	if(result != CY_RSLT_SUCCESS)
	{
		return FALSE;
	}

	/*Set the frequency to have 50us per tick*/
	result = cyhal_timer_set_frequency(&mb_timer_obj, TIMER_FREQ);
	if(result != CY_RSLT_SUCCESS)
	{
		return FALSE;
	}

	/*Register callback and interrupt*/
    cyhal_timer_register_callback(&mb_timer_obj, mb_timer_isr, NULL);
	cyhal_timer_enable_event(&mb_timer_obj, CYHAL_TIMER_IRQ_TERMINAL_COUNT, 3, true);

	return TRUE;
}
 
 
void
vMBPortTimersEnable(  )
{
  /* Enable the timer with the timeout passed to xMBPortTimersInit( ) */
	(void)cyhal_timer_start(&mb_timer_obj);
}
 
void
vMBPortTimersDisable(  )
{
  /* Disable any pending timers. */
	(void)cyhal_timer_stop(&mb_timer_obj);
}
 
/* Create an ISR which is called whenever the timer has expired. This function
* must then call pxMBPortCBTimerExpired( ) to notify the protocol stack that
* the timer has expired.
 
static void prvvTIMERExpiredISR( void )
{
( void )pxMBPortCBTimerExpired(  );
}
*/

/*Timer interrupt routine*/
static void mb_timer_isr(void *callback_arg, cyhal_timer_event_t event)
{
    (void) callback_arg;
    (void) event;
    BOOL bTaskWoken = FALSE;

    vMBPortSetWithinException( TRUE );

    /*Notification*/
    ( void )pxMBPortCBTimerExpired(  );

    vMBPortSetWithinException( FALSE );

    portEND_SWITCHING_ISR( bTaskWoken ? pdTRUE : pdFALSE );
}
