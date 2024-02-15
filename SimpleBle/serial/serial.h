/*
 * serial.h
 *
 *  Created on: Feb 14, 2024
 *      Author: danielbraun
 */

#ifndef SERIAL_SERIAL_H_
#define SERIAL_SERIAL_H_

#include "../misc.h"
#include "main.h"

#define PORT_VCOM 	0
#define PORT_BLE	1


//int serial_setup(int port, osThreadId task);


int serial_start_rx(int port);

int serial_send_bytes(int port, const uint8_t *b, int len, int needcopy);


#define NOTIF_UART_TX		0x00000100
#define NOTIF_AT_RESP_OK	0x00000001
#define NOTIF_AT_RESP_OTHER	0x00000002

typedef struct serial {
	osThreadId          taskHandle;
	void 			    (*linecallback)(struct serial *s, int ok);
	UART_HandleTypeDef *uart;
	DMA_HandleTypeDef  *txdma;
	uint8_t            *rxbuf;
	uint8_t            *txbuf;
	uint16_t            rxbuflen;
	uint16_t            txbuflen;
	volatile uint16_t   rxidx;
	volatile uint8_t    txonprogress;
	uint8_t 		    eolcar;
	uint8_t             igncar;
} serial_t;

#define NUM_SERIALS 2

extern serial_t serials[NUM_SERIALS];

#endif /* SERIAL_SERIAL_H_ */
