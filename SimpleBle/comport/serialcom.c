/*
 * serialcom.c
 *
 *  Created on: Feb 14, 2024
 *      Author: danielbraun
 */
#include <stdint.h>
#include <string.h>
#include "serialcom.h"

#include "../misc.h"
#include "main.h"
#include "../itm_debug.h"
#include "../serial/serial.h"

extern osThreadId comTaskHandle;

static void gotline(serial_t *s, int ok);

void StartComTask(void const * argument)
{
#if TEST_AT_ON_VCOM
	for (;;) {
		osDelay(1000);
	}
#endif
	itm_debug1(DBG_COM, "STRTc", 0);
	serial_t *ser = &serials[PORT_VCOM];
	ser->taskHandle = comTaskHandle;
	ser->linecallback = gotline;
	// serial port is used only for logging
	if ((0)) serial_start_rx(PORT_VCOM);
	//serial_setup(PORT_VCOM, comTaskHandle);

	const char *s1 = "coucou\r\n";
	const char *s2 = "hello\r\n";
	for (int i=0;;i++) {
		osDelay(500);
		flash_led();
		char *s = (i%3) ? s1 : s2;
		int l = strlen(s);
		itm_debug1(DBG_COM, "c-send", l);
		serial_send_bytes(PORT_VCOM, (uint8_t *)s, l, 0);
	}
}

static void gotline(serial_t *s, int ok)
{
	itm_debug1(DBG_COM, "line", s->rxidx);
	s->rxidx = 0;
}

#if 0


extern DMA_HandleTypeDef hdma_lpuart1_rx;
extern DMA_HandleTypeDef hdma_lpuart1_tx;

static void _start_rx(int);

void StartUsbTask(_UNUSED_ const void *argument)
{
	_start_rx(0);
	for (;;) {
		osDelay(10);
		USB_Tasklet(0, 0, 0);
	}
}

static uint8_t txbuf[128];
volatile uint8_t txonprogress = 0;

static uint8_t rxbuf[9]; // msg_64 + framing


static void _start_rx(int offset)
{
	//HAL_UART_Receive_DMA(&hlpuart1, rxbuf, 2*sizeof(msg_64_t));
	HAL_StatusTypeDef rc = UART_Start_Receive_IT(&hlpuart1, rxbuf+offset, sizeof(rxbuf)-offset);
	if (rc != HAL_OK) {
		itm_debug2(DBG_USB|DBG_ERR, "Ustrt", rc, hlpuart1.ErrorCode);
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
		itm_debug1(DBG_USB|DBG_ERR, "TxErr", rc);
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
/*
void HAL_UART_RxHalfCpltCallback(_UNUSED_ UART_HandleTypeDef *huart)
{
}
*/

static void bh(void)
{

}
void HAL_UARTEx_RxFifoFullCallback(_UNUSED_  UART_HandleTypeDef *huart)
{
	itm_debug1(DBG_USB, "RxFifoFull", 0);
	bh();
}
void HAL_UART_RxHalfCpltCallback(_UNUSED_ UART_HandleTypeDef *huart)
{
	itm_debug1(DBG_USB, "RxHalfCplt", 0);
	bh();
}
void HAL_UART_RxCpltCallback(_UNUSED_ UART_HandleTypeDef *huart)
{
	//itm_debug2(DBG_USB, "Rx", huart->RxXferCount, huart->RxXferSize);
	//itm_debug1(DBG_USB, "RxCplt", 0);
	// 9 bytes received
	int offset = 0;
	if (FRAME_M64==rxbuf[0]) {
		// normal frame, aligned
		msg_64_t m;
		memcpy(&m, rxbuf+1, 8);
		itm_debug1(DBG_USB, "msg8", m.cmd);
		mqf_write_from_usb(&m);
	} else {
		// frame is unaligned
		for (int i=1;i<9; i++) {
			if (rxbuf[i]==FRAME_M64) {
				offset = i;
				memmove(rxbuf, rxbuf+offset, 9-offset);
			}
		}
		itm_debug1(DBG_ERR|DBG_USB, "unalgn", offset);
	}
	bh();
	_start_rx(offset);
}
void HAL_UARTEx_RxEventCallback(_UNUSED_ UART_HandleTypeDef *huart, _UNUSED_ uint16_t Size)
{
	itm_debug1(DBG_USB, "RxEvent", Size);
	bh();
}
void HAL_UART_ErrorCallback(_UNUSED_ UART_HandleTypeDef *huart)
{
	itm_debug1(DBG_USB, "RxErr", huart->ErrorCode);
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
#endif
