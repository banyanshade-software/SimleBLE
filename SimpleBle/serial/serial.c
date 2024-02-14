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
extern UART_HandleTypeDef hlpuart1;
static uint8_t rxbuf[9]; // msg_64 + framing



void _start_rx(int offset)
{
	//HAL_UART_Receive_DMA(&hlpuart1, rxbuf, 2*sizeof(msg_64_t));
	HAL_StatusTypeDef rc = UART_Start_Receive_IT(&hlpuart1, rxbuf+offset, sizeof(rxbuf)-offset);
	if (rc != HAL_OK) {
		itm_debug2(DBG_SERIAL|DBG_ERR, "Ustrt", rc, hlpuart1.ErrorCode);
	}

}

static void _send_bytes(const uint8_t *b, int len)
{
	//if ((1)) return; //XXX

	if ((len<=0) || (len>(int)sizeof(txbuf))) {
		FatalError("sndbyt", "bad len for _send_bytes", Error_Uart_Len);
	}
	while (txonprogress) {
		uint32_t notif = 0;
		xTaskNotifyWait(0, 0xFFFFFFFF, &notif, portMAX_DELAY);
	}
 	memcpy(txbuf, b, len);
 	txonprogress = 1;
 	HAL_StatusTypeDef rc;
 	if ((0)) {
 		rc = HAL_UART_Transmit_DMA(&hlpuart1, (const uint8_t *)"hello world---", 12);
 	} else {
 		rc = HAL_UART_Transmit_DMA(&hlpuart1, txbuf, len);
 	}
	if (rc != HAL_OK) {
		itm_debug1(DBG_SERIAL|DBG_ERR, "TxErr", rc);
	 	txonprogress = 0;
		return;
	}

}

//void HAL_UART_TxHalfCpltCallback(UART_HandleTypeDef *huart);
void HAL_UART_TxCpltCallback(_UNUSED_ UART_HandleTypeDef *huart)
{
	txonprogress = 0;
	BaseType_t higher=0;
	xTaskNotifyFromISR(usbTaskHandle, NOTIF_UART_TX, eSetBits, &higher);
	portYIELD_FROM_ISR(higher);
}


static void bh(void)
{

}
void HAL_UARTEx_RxFifoFullCallback(_UNUSED_  UART_HandleTypeDef *huart)
{
	itm_debug1(DBG_SERIAL, "RxFifoFull", 0);
	bh();
}
void HAL_UART_RxHalfCpltCallback(_UNUSED_ UART_HandleTypeDef *huart)
{
	itm_debug1(DBG_SERIAL, "RxHalfCplt", 0);
	bh();
}
void HAL_UART_RxCpltCallback(_UNUSED_ UART_HandleTypeDef *huart)
{
	//itm_debug2(DBG_SERIAL, "Rx", huart->RxXferCount, huart->RxXferSize);
	//itm_debug1(DBG_SERIAL, "RxCplt", 0);
	// 9 bytes received
	int offset = 0;
	if (FRAME_M64==rxbuf[0]) {
		// normal frame, aligned
		msg_64_t m;
		memcpy(&m, rxbuf+1, 8);
		itm_debug1(DBG_SERIAL, "msg8", m.cmd);
		mqf_write_from_usb(&m);
	} else {
		// frame is unaligned
		for (int i=1;i<9; i++) {
			if (rxbuf[i]==FRAME_M64) {
				offset = i;
				memmove(rxbuf, rxbuf+offset, 9-offset);
			}
		}
		itm_debug1(DBG_ERR|DBG_SERIAL, "unalgn", offset);
	}
	bh();
	_start_rx(offset);
}
void HAL_UARTEx_RxEventCallback(_UNUSED_ UART_HandleTypeDef *huart, _UNUSED_ uint16_t Size)
{
	itm_debug1(DBG_SERIAL, "RxEvent", Size);
	bh();
}
void HAL_UART_ErrorCallback(_UNUSED_ UART_HandleTypeDef *huart)
{
	itm_debug1(DBG_SERIAL, "RxErr", huart->ErrorCode);
	if (__HAL_UART_GET_FLAG(huart, UART_FLAG_PE) != RESET) {
		__HAL_UART_CLEAR_FLAG(huart, UART_CLEAR_PEF); /* Clear PE flag */
	} else if (__HAL_UART_GET_FLAG(huart, UART_FLAG_FE) != RESET) {
		__HAL_UART_CLEAR_FLAG(huart, UART_CLEAR_FEF); /* Clear FE flag */
	} else if (__HAL_UART_GET_FLAG(huart, UART_FLAG_NE) != RESET) {
		__HAL_UART_CLEAR_FLAG(huart, UART_CLEAR_NEF); /* Clear NE flag */
	} else if (__HAL_UART_GET_FLAG(huart, UART_FLAG_ORE) != RESET) {
		__HAL_UART_CLEAR_FLAG(huart, UART_CLEAR_OREF); /* Clear ORE flag */
	}
	_start_rx(0);
}

