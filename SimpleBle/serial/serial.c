/*
 * serial.c
 *
 *  Created on: Feb 14, 2024
 *      Author: danielbraun
 */


#include <stdint.h>

#include "serial.h"

#include "../misc.h"
#include "main.h"
#include "../itm_debug.h"


extern DMA_HandleTypeDef hdma_lpuart1_rx;
extern DMA_HandleTypeDef hdma_lpuart1_tx;



void _start_rx(int offset)
{
	//HAL_UART_Receive_DMA(&hlpuart1, rxbuf, 2*sizeof(msg_64_t));
	HAL_StatusTypeDef rc = UART_Start_Receive_IT(&hlpuart1, rxbuf+offset, sizeof(rxbuf)-offset);
	if (rc != HAL_OK) {
		itm_debug2(DBG_SERIAL|DBG_ERR, "Ustrt", rc, hlpuart1.ErrorCode);
	}

}
