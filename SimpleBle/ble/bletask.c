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
#include "../resmsg.h"

extern osMessageQId resQueueHandle;


#if TEST_AT_ON_VCOM
#undef PORT_BLE
#define PORT_BLE 0
#endif

// HM-10 ressources
// https://people.ece.cornell.edu/land/courses/ece4760/PIC32/uart/HM10/DSD%20TECH%20HM-10%20datasheet.pdf

// AT-09 BLE Module
// https://www.studiopieters.nl/arduino-at-09-ble-module/
// http://denethor.wlu.ca/arduino/MLT-BT05-AT-commands-TRANSLATED.pdf
// https://github.com/MikeBland/mbtx/blob/67b420da816811c60e76f29b2caf57f69fa63083/radio/ersky9x/src/X12D/e.cpp#L2329


typedef enum {
	state_wstart,
	state_sendnext,
	state_wtempo,
	state_wok,
	state_done,
} /*__attribute((packed))*/ at_state_t;


typedef struct {
	const char *cmd;
	uint32_t waitbefore;
} at_cmd_t;

static void gotline(serial_t *s, int ok);

#define AT_DELAY 10000

#define NUM_CMD 12

at_cmd_t atcommand[NUM_CMD] = {
		// long command to exit sleep if any (>80)
		//{ .cmd="Longtemps je me suis couche de bonne heure. Parfois a peine ma bougie eteinte, mes yeux se fermaient si vite que je n'avais pas le temps de me dire ''je m'endors.'' Et, une demi-heure apres, la pensee qu'il etait temps de chercher le sommeil m'eveillait...\n" },
		// base AT command
/*0*/	{ .cmd="AT\r\n"  },
		//{ .cmd="AT+VERSION?\r\n" },
		//{ .cmd="AT+HELP\r\n" },
		//{ .waitbefore=6000, .cmd="AT\r\n"  },

#if 1
		//{ .cmd="AT+RESET?\r\n"  },
		//{ .cmd="AT+HELP?\r\n"  },
		//{ .cmd="AT+MARJ?\r\n"  },
		//{ .cmd="AT+STATE?\r\n" },
		//{ .cmd="AT+PWRM?\r\n" },
		// several queries, not usefull
		//{ .cmd="AT+PIO10\r\n" },
		//{ .cmd="AT+ADDR?\r\n" }, // Query module address
		//{ .cmd="AT+ADVI?\r\n" }, // Advertising interval
		//{ .cmd="AT+ADTY?\r\n" }, // Advertising type
		//{ .cmd="AT+BAUD?\r\n" },
		//discovery scan
		// doc say : Please set AT+ROLE1 and AT+IMME1 first.
		{ .cmd="AT+NAMEbletest\r\n" },
/*1*/	{ .cmd="AT+ROLE1\r\n" }, // 1=central, 0=periph
/*2*/   //{ .cmd="AT+IMME1\r\n" }, // role 0=start immed, 1=wait for AT conn
		//{ .cmd="AT+DISC?\r\n" },
		// ibeacon discovery
        // { .cmd="AT+SHOW\r\n" },
/*3*/	//{ .cmd="AT+DISI?\r\n" },
		//{ .cmd="AT\r\n" },$
		//{ .cmd="AT+CMODE?\r\n" },
		// { .cmd="AT+ADVI3\r\n" },
		// { .cmd="AT+POWE0\r\n" },
		// { .cmd="AT+RMAAD\r\n" },
//{ .cmd="AT+CMODE0"},
{ .cmd="AT+TYPE?\n\r" },

{ .cmd="AT+INQM?\r\n" },
{ .cmd="AT+INQM=1,9,20\r\n" },
		{ .cmd="AT+INQ\r\n" }, // INQ1 start scanning, INQ0 stopscan
		{ .cmd="\r\n" },
		{ .cmd="AT+INQ\r\n" }, // INQ1 start scanning, INQ0 stopscan
		{ .cmd="\r\n" },
		{ .cmd="AT+SHOW\r\n" },

		{ .cmd="AT+SHOW\r\n" },
		{ .cmd="AT+SHOW\r\n" },
		//{ .cmd="AT+INQ0\r\n" }, // INQ1 start scanning, INQ0 stopscan
		{ .cmd="AT+SHOW\r\n" },
		{ .cmd="AT+SHOW\r\n" },

#endif
#if 0
		{ .cmd="AT+INQ?\r\n" },
		{ .cmd="AT\r\n" },
		{ .cmd="AT+DISI?\r\n" },
		{ .cmd="AT\r\n" },
		{ .cmd="AT\r\n" },
		{ .cmd="AT\r\n" },
		{ .cmd="AT+DISA?\r\n" },
		{ .cmd="AT\r\n" },
			{ .cmd="AT\r\n" },
			{ .cmd="AT\r\n" },
#endif
#if 0
			{ .cmd="\r\n" },
			{ .cmd="\r\n" },
			{ .cmd="\r\n" },
			{ .cmd="\r\n" },
			{ .cmd="\r\n" },
			{ .cmd="\r\n" },
			{ .cmd="\r\n" },
			{ .cmd="\r\n" },
			{ .cmd="\r\n" },
			{ .cmd="\r\n" },
			{ .cmd="\r\n" },
			{ .cmd="\r\n" },
#endif

};

extern osThreadId bleTaskHandle;

void StartBleTask(void const * argument)
{
	uint32_t t0 = HAL_GetTick();

	if ((0)) {
		for (;;) {
			osDelay(1000);
		}
	}
	if ((0)) {
		for (int i=0; ;i++) {
			osDelay(1000);
			resmsg_t m;
			m.ts = HAL_GetTick() - t0;
			m.type = 1;
			m.devnum = 0;
			m.rssi = i; // for instance
			xQueueSend(resQueueHandle, &m, 10);
		}
	}
	itm_debug1(DBG_BLE, "STRTb", 0);
	serial_t *ser = &serials[PORT_BLE];
	ser->taskHandle = bleTaskHandle;
	ser->linecallback = gotline;
	ser->igncar = '\n';
	serial_start_rx(PORT_BLE);

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
			if (atcommand[cmdnum].waitbefore>0) {
				osDelay(atcommand[cmdnum].waitbefore);
			}
			itm_debug1(DBG_BLE, "sendcmd", cmdnum);
			const char *cmd = atcommand[cmdnum].cmd;
			int cmdlen = strlen(cmd);
			serial_send_bytes(PORT_BLE, (const uint8_t *)cmd, cmdlen, 0);
			cmdtick = HAL_GetTick();
			state = state_wok;
#if TEST_BLE_ECHO_VCOM
			static resmsg_t m; // as static to avoid stack usage
			int l = MAX(cmdlen, TRACE_LEN-1);
			m.type = type_tracetx;
			m.ts = cmdtick;
			memcpy(m.line, cmd, l);
			m.line[l] = '\0';
			BaseType_t higher2=0;
			xQueueSendFromISR(resQueueHandle,&m, &higher2);
#endif
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
					// wait for additional lines, max 50ms
					if ((1)) {
						for (;;) {
							rc = xTaskNotifyWait(0, 0xFFFFFFFF, &notif, 50);
							if (rc == pdTRUE) {
								itm_debug1(DBG_BLE, "got more", cmdnum);
							} else {
								break;
							}
						}
					}
				} else  {
					itm_debug1(DBG_BLE|DBG_ERR, "timeout", cmdnum);
					osDelay(5000);
					if ((0)) {
						state = state_wstart;
					} else {
						cmdnum++;
						state = state_sendnext;
					}
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
		//itm_debug1(DBG_BLE, "hop", 0);
		osDelay(100);
	}
}

#define STOREBUFLEN 80
static char storebuf[STOREBUFLEN];

static void gotline(serial_t *s, int ok)
{
	itm_debug2(DBG_SERIAL, "line", ok, s->rxidx);
	if (!ok) {
		s->rxidx = 0;
		return;
	}
#if TEST_BLE_ECHO_VCOM
	static resmsg_t m; // as static to avoid stack usage
	int _l = MIN(s->rxidx, TRACE_LEN-1);
	m.type = type_tracerx;
	m.ts = HAL_GetTick();
	memcpy(m.line, s->rxbuf, _l);
	m.line[_l] = '\0';
	BaseType_t higher2=0;
	xQueueSendFromISR(resQueueHandle,&m, &higher2);
#endif

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
#if TEST_BLE_ECHO_VCOM
	higher |= higher2;
#endif
	portYIELD_FROM_ISR(higher);
}

/*
 *
 *
 *
 * RX: ************************************************************
RX: * Command   **********Description                              *
RX: * ----------------------------------------------------------
RX: * AT                  Check if the command terminal work nor
RX: * AT+RESET        minal work noreboot                                *
RX: * AT+VERSION          Get firmware, bluetooth, HCI and LMP v
RX: * AT+HELP             List all the commands                       *
RX: * AT+NAME             Get/Set local device name
RX: * AT+PIN              Get/Set pin code for pairing
RX: * AT+BAUD             Get/Set baud rate
RX: * AT+LADDR            Get local b   tooth address                   *
RX: * AT+ADDR             Get local bluetooth address                   *
RX: * AT+DEFAULT          Restore factory default                       *
RX: * AT+RENEW            Restore factory default                       *
*X: * AT+STATE            Get current state                             *
RX: * AT+PWRM             Get/Set power on mode(low power)
RX: * AT+POWE             Get/Set RF transmit power             *
RX: * AT+SLEEP            Sleep mode                             *
RX: * AT+ROLE             Get/Set current role.
RX: * AT+PARI             Get/Set UART parity bit.
TX: AT+MARJ?
RX: * AT+STOP             Get/Set UART stop bit.
RX: * AT+INQ              Search slave model
RX: * AT+SHOW             Show the searched slave model.
RX: * AT+CONN             Connect the index slave model.
RX: * AT+IMME             System wait for command when power on.
RX: *power onT            System start working.                         *
RX: * AT+UUID             Get/Set system SERVER_UUID .
RX: * AT+CHAR             Get/Set system CHAR_UUID .
RX: * ----------------------------------------------------------
                                        RX: * Note: (M) = The command support master mode only.
 *
 */



/*
 *                                                                 RX: * Command   **********Description                              *
          RX: * ----------------------------------------------------------
                                                                          RX: * AT                  Check if the command terminal work nor
                                                          RX: * AT+RESET        minal work noreboot                                *
             RX: * AT+VERSION          Get firmware, bluetooth, HCI and LMP v
                                                                             RX: * AT+HELP             List all the commands                       *
 RX: * AT+NAME             Get/Set local device name
                                                                 RX: * AT+PIN              Get/Set pin code for pairing
                                                 RX: * AT+BAUD             Get/Set baud rate
                                           RX: * AT+LADDR            Get local b   tooth address                   *
   RX: * AT+ADDR             Get local bluetooth address                   *
   RX: * AT+DEFAULT          Restore factory default                       *
   RX: * AT+RENEW            Restore factory default                       *
   *X: * AT+STATE            Get current state                             *
   RX: * AT+PWRM             Get/Set power on mode(low power)
                                                                           RX: * AT+POWE             Get/Set RF transmit power             *
                  RX: * AT+SLEEP            Sleep mode                             *
          RX: * AT+ROLE             Get/Set current role.
                                                                               RX: * AT+PARI             Get/Set UART parity bit.
                                                               RX: * AT+STOP             Get/Set UART stop bit.
                                               RX: * AT+INQ              Search slave model
                               RX: * AT+SHOW             Show the searched slave model.
               RX: * AT+CONN             Connect the index slave model.
                                                                               RX: * AT+IMME             System wait for command when power on.
                                                               RX: * AT+START   power on System start working.                     *
        RX: * AT+UUID             Get/Set system SERVER_UUID .
                                                                        RX: * AT+CHAR             Get/Set system CHAR_UUID .
                                                        RX: * ----------------------------------------------------------
                                        RX: * Note: (M) = The command support master mode only.
                               TX: AT
ote: (M) = The command support master mode only.
                                                              TX:
TX:
TX:
TX:
TX:
TX:
hello -----------------------------------------------------
TX: AT
RX: OK
hello -----------------------------------------------------
TX: AT
RX: OK
TX: AT+HELP
RX: ************************************************************
                                                                RX: * Command   **********Description                              *
          RX: * ----------------------------------------------------------
                                                                          RX: * AT                  Check if the command terminal work nor
                                                          RX: * AT+RESET        minal work noreboot                                *
             RX: * AT+VERSION          Get firmware, bluetooth, HCI and LMP v
                                                                             RX: * AT+HELP             List all the commands                       *
 RX: * AT+NAME             Get/Set local device name
                                                                 RX: * AT+PIN              Get/Set pin code for pairing
                                                 RX: * AT+BAUD             Get/Set baud rate
                                           RX: * AT+LADDR            Get local b   tooth address                   *
   RX: * AT+ADDR             Get local bluetooth address                   *
   RX: * AT+DEFAULT          Restore factory default                       *
   RX: * AT+RENEW            Restore factory default                       *
   *X: * AT+STATE            Get current state                             *
   RX: * AT+PWRM             Get/Set power on mode(low power)
                                                                           RX: * AT+POWE             Get/Set RF transmit power             *
                  RX: * AT+SLEEP            Sleep mode                             *
          RX: * AT+ROLE             Get/Set current role.
                                                                               RX: * AT+PARI             Get/Set UART parity bit.
                                                               RX: * AT+STOP             Get/Set UART stop bit.
                                               RX: * AT+INQ              Search slave model
                               RX: * AT+SHOW             Show the searched slave model.
               RX: * AT+CONN             Connect the index slave model.
                                                                               RX: * AT+IMME             System wait for command when power on.
                                                               RX: * AT+START   power on System start working.                     *
        RX: * AT+UUID             Get/Set system SERVER_UUID .
                                                                        RX: * AT+CHAR             Get/Set system CHAR_UUID .
                                                        RX: * ----------------------------------------------------------
                                        RX: * Note: (M) = The command support master mode only.
                               TX: AT
ote: (M) = The command support master mode only.
                                                              TX:
TX:
TX:
TX:
hello -----------------------------------------------------
TX: AT
RX: OK
hello -----------------------------------------------------
TX: AT
RX: OK
TX: AT+HELP
RX: ************************************************************
RX: * Command   **********Description                              *
RX: * ----------------------------------------------------------
RX: * AT                  Check if the command terminal work nor
RX: * AT+RESET        minal work noreboot                                *
RX: * AT+VERSION          Get firmware, bluetooth, HCI and LMP v
RX: * AT+HELP             List all the commands                       *
RX: * AT+NAME             Get/Set local device name
RX: * AT+PIN              Get/Set pin code for pairing
RX: * AT+BAUD             Get/Set baud rate
RX: * AT+LADDR            Get local b   tooth address                   *
RX: * AT+ADDR             Get local bluetooth address                   *
RX: * AT+DEFAULT          Restore factory default                       *
RX: * AT+RENEW            Restore factory default                       *
*X: * AT+STATE            Get current state                             *
RX: * AT+PWRM             Get/Set power on mode(low power)
RX: * AT+POWE             Get/Set RF transmit power             *
RX: * AT+SLEEP            Sleep mode                             *
RX: * AT+ROLE             Get/Set current role.
RX: * AT+PARI             Get/Set UART parity bit.
RX: * AT+STOP             Get/Set UART stop bit.
RX: * AT+INQ              Search slave model
RX: * AT+SHOW             Show the searched slave model.
RX: * AT+CONN             Connect the index slave model.
RX: * AT+IMME             System wait for command when power on.
RX: * AT+START   power on System start working.                     *
RX: * AT+UUID             Get/Set system SERVER_UUID .
RX: * AT+CHAR             Get/Set system CHAR_UUID .
RX: * ----------------------------------------------------------
RX: * Note: (M) = The command support master mode only.
TX: AT
ote: (M) = The command support master mode only.
TX:
TX:

 */
