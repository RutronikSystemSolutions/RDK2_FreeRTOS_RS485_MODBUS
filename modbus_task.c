/*
 * modbus_task.c
 *
 *  Created on: 2021-05-05
 *      Author: GDR
 */

#include "cyhal.h"
#include "cybsp.h"
#include "cy_retarget_io.h"
#include "FreeRTOS.h"
#include "task.h"
#include "modbus_task.h"
#include "mb.h"
#include "mbport.h"

#define REG_INPUT_START 1
#define REG_INPUT_NREGS 6

TaskHandle_t modbus_task_handle = NULL;
static USHORT usRegInputStart = REG_INPUT_START;
static USHORT usRegInputBuf[REG_INPUT_NREGS];

void modbus_task(void *param)
{
	(void) param;
	eMBErrorCode eStatus;

	/* Hello! */
	usRegInputBuf[0] =  'H';
	usRegInputBuf[1] =  'e';
    usRegInputBuf[2] =  'l';
	usRegInputBuf[3] =  'l';
	usRegInputBuf[4] =  'o';
	usRegInputBuf[5] =  '!';

	/*Initialize FreeModbus Stack*/
	eStatus = eMBInit( MB_RTU, 1, 0, 115200, MB_PAR_NONE );
	if(eStatus != MB_ENOERR)
	{
		printf("MODBUS setup failure.\r\n");
		CY_ASSERT(0);
	}
	eStatus = eMBEnable();
	if(eStatus != MB_ENOERR)
	{
		printf("MODBUS setup failure.\r\n");
		CY_ASSERT(0);
	}

	for(;;)
	{
		/*Poll the FreeModbus Stack*/
		eMBPoll();
	}

}

eMBErrorCode
eMBRegInputCB( UCHAR * pucRegBuffer, USHORT usAddress, USHORT usNRegs )
{
    eMBErrorCode    eStatus = MB_ENOERR;
    int             iRegIndex;

    if( ( usAddress >= REG_INPUT_START )
        && ( usAddress + usNRegs <= REG_INPUT_START + REG_INPUT_NREGS ) )
    {
        iRegIndex = ( int )( usAddress - usRegInputStart );
        while( usNRegs > 0 )
        {
            *pucRegBuffer++ =
                ( unsigned char )( usRegInputBuf[iRegIndex] >> 8 );
            *pucRegBuffer++ =
                ( unsigned char )( usRegInputBuf[iRegIndex] & 0xFF );
            iRegIndex++;
            usNRegs--;
        }

      cyhal_gpio_toggle(LED1);
    }
    else
    {
        eStatus = MB_ENOREG;
    }

    return eStatus;
}

eMBErrorCode
eMBRegHoldingCB( UCHAR * pucRegBuffer, USHORT usAddress, USHORT usNRegs,
                 eMBRegisterMode eMode )
{
    return MB_ENOREG;
}


eMBErrorCode
eMBRegCoilsCB( UCHAR * pucRegBuffer, USHORT usAddress, USHORT usNCoils,
               eMBRegisterMode eMode )
{
    return MB_ENOREG;
}

eMBErrorCode
eMBRegDiscreteCB( UCHAR * pucRegBuffer, USHORT usAddress, USHORT usNDiscrete )
{
    return MB_ENOREG;
}
