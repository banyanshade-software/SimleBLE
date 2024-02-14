/*
 * bletask.c
 *
 *  Created on: Feb 14, 2024
 *      Author: danielbraun
 */


#include <stdint.h>
#include <string.h>
#include "../misc.h"
#include "main.h"
#include "bletask.h"
#include "../itm_debug.h"
#include "../serial/serial.h"

#if TEST_AT_ON_VCOM
#undef PORT_BLE
#define PORT_BLE 0
#endif

// HM-10 ressources
// https://people.ece.cornell.edu/land/courses/ece4760/PIC32/uart/HM10/DSD%20TECH%20HM-10%20datasheet.pdf


typedef enum {
	state_wstart,
	state_sendnext,
	state_wtempo,
	state_wok,
	state_done,
} /*__attribute((packed))*/ at_state_t;


typedef struct {
	const char *cmd;
} at_cmd_t;

static void gotline(serial_t *s, int ok);

#define AT_DELAY 10000

#define NUM_CMD 11
at_cmd_t atcommand[NUM_CMD] = {
		// long command to exit sleep if any (>80)
		{ .cmd="Longtemps je me suis couche de bonne heure. Parfois a peine ma bougie eteinte, mes yeux se fermaient si vite que je n'avais pas le temps de me dire ''je m'endors.'' Et, une demi-heure apres, la pensee qu'il etait temps de chercher le sommeil m'eveillait...\n" },
		// base AT command
		{ .cmd="AT\n"  },
		// several queries, not usefull
		{ .cmd="AT+PIO10\r\n" },
		{ .cmd="AT+ADDR?\r\n" }, // Query module address
		{ .cmd="AT+ADVI?\r\n" }, // Advertising interval
		{ .cmd="AT+ADTY?\r\n" }, // Advertising type
		{ .cmd="AT+BAUD?\r\n" },
		//discovery scan
		// doc say : Please set AT+ROLE1 and AT+IMME1 first.
		{ .cmd="AT+ROLE1\r\n" }, // 1=central, 0=periph
		{ .cmd="AT+IMME1\r\n" }, // role 0=start immed, 1=wait for AT conn
		{ .cmd="AT+DISC?\r\n" },
		// ibeacon discovery
		{ .cmd="AT+DISI?\r\n" }
};

extern osThreadId bleTaskHandle;

void StartBleTask(void const * argument)
{
	itm_debug1(DBG_BLE, "STRTb", 0);
	serial_t *ser = &serials[PORT_VCOM];
	ser->taskHandle = bleTaskHandle;
	ser->linecallback = gotline;
	serial_start_rx(PORT_VCOM);

	at_state_t state = state_wstart;
	int cmdnum = 0;
	_UNUSED_ uint32_t cmdtick = 0;

	for (;;) {
		switch(state) {
		case state_wstart:
			cmdnum = 0;
			//FALLTHRU
		case state_sendnext:
			if (cmdnum >= NUM_CMD) {
				itm_debug1(DBG_BLE, "DONE", cmdnum);
				state = state_done;
				break;
			}
			itm_debug1(DBG_BLE, "sendcmd", cmdnum);
			const char *cmd = atcommand[cmdnum].cmd;
			serial_send_bytes(PORT_BLE, (const uint8_t *)cmd, strlen(cmd), 0);
			cmdtick = HAL_GetTick();
			state = state_wok;
			break;
		case state_wok:
			for (;;) {
				uint32_t notif = 0;
				BaseType_t rc;
				rc = xTaskNotifyWait(0, 0xFFFFFFFF, &notif, AT_DELAY);
				if (rc == pdTRUE) {
					if (notif & NOTIF_AT_RESP_OK) {
						itm_debug1(DBG_BLE, "gotok", cmdnum);
						cmdnum++;
						state = state_sendnext;
						break;
					} else if (notif & NOTIF_AT_RESP_OTHER) {
						itm_debug1(DBG_BLE, "got other", cmdnum);
						// treate as ok for now
						cmdnum++;
						state = state_sendnext;
						break;
					}
				} else  {
					itm_debug1(DBG_BLE|DBG_ERR, "timeout", cmdnum);
					osDelay(5000);
					state = state_wstart;
					break;
				}
			}
			break;

		case state_done:
			for (;;) {
				osDelay(10000);
			}
			break;
		default:
			itm_debug2(DBG_ERR|DBG_BLE, "inv state", state, cmdnum);
			state = state_done;
			break;
		}
		itm_debug1(DBG_BLE, "hop", 0);
		osDelay(1000);
	}
}

#define STOREBUFLEN 80
static char storebuf[STOREBUFLEN];

static void gotline(serial_t *s, int ok)
{
	if (!ok) {
		s->rxidx = 0;
		return;
	}
	if (s->rxidx<2) { // probably \r  (empty line)
		s->rxidx = 0;
		return;
	}
	int okr = 0;
	if ((s->rxbuf[0]='O') && (s->rxbuf[0]='K')) {
		okr = 1;
	}
	int l = s->rxidx;
	if (l > STOREBUFLEN) l = STOREBUFLEN;
	memcpy(storebuf, s->rxbuf, l);
	BaseType_t higher=0;
	xTaskNotifyFromISR(s->taskHandle, okr ? NOTIF_AT_RESP_OK : NOTIF_AT_RESP_OTHER, eSetBits, &higher);
	portYIELD_FROM_ISR(higher);
}



