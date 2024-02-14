/*
 * serial.c
 *
 *  Created on: Feb 14, 2024
 *      Author: danielbraun
 */


#include <stdint.h>
#include <string.h>
#include "serial.h"

#include "../misc.h"
#include "main.h"
#include "../itm_debug.h"


extern UART_HandleTypeDef hlpuart1;
extern UART_HandleTypeDef huart4;
extern DMA_HandleTypeDef hdma_lpuart1_tx;
extern DMA_HandleTypeDef hdma_uart4_tx;

typedef struct {
	UART_HandleTypeDef *uart;
	DMA_HandleTypeDef  *txdma;
	uint8_t            *rxbuf;
	uint16_t            rxbuflen;
	uint16_t            rxidx;
	uint8_t             txonprogress;
} serial_t;

static uint8_t rxbuf0[256];
static uint8_t rxbuf1[256];


#define NUM_SERIALS 2
serial_t serials[NUM_SERIALS] = {
		{ .rxidx=0, .uart=&hlpuart1, .txdma=&hdma_lpuart1_tx, .rxbuf=rxbuf0, .rxbuflen=sizeof(rxbuf0) },
		{ .rxidx=0, .uart=&huart4,   .txdma=&hdma_uart4_tx,   .rxbuf=rxbuf1, .rxbuflen=sizeof(rxbuf1) }
};



void serial_start_rx(int port)
{
	serial_t *s = &serials[port];
	//HAL_UART_Receive_DMA(&hlpuart1, rxbuf, 2*sizeof(msg_64_t));
	HAL_StatusTypeDef rc = UART_Start_Receive_IT(s->uart, s->rxbuf+s->rxidx, s->rxbuflen-s->rxidx);
	if (rc != HAL_OK) {
		itm_debug2(DBG_SERIAL|DBG_ERR, "Ustrt", rc, s->uart->ErrorCode);
	}
}

static void serial_send_bytes(int port, const uint8_t *b, int len, int needcopy)
{
	serial_t *s = &serials[port];

	while (s->txonprogress) {
		uint32_t notif = 0;
		xTaskNotifyWait(0, 0xFFFFFFFF, &notif, portMAX_DELAY);
	}
 	memcpy(txbuf, b, len);
 	s->txonprogress = 1;
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

