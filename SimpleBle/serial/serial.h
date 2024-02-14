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

int serial_setup(int port, osThreadId task);


int serial_start_rx(int port);

int serial_send_bytes(int port, const uint8_t *b, int len, int needcopy);


#define NOTIF_UART_TX		0x00000100
//#define NOTIF_UART_TX		0x00000100

#endif /* SERIAL_SERIAL_H_ */
