/*
 * resmsg.h
 *
 *  Created on: Feb 15, 2024
 *      Author: Daniel BRAUN
 */

#ifndef RESMSG_H_
#define RESMSG_H_

#include "misc.h"

typedef enum {
	type_info,
	type_rssi,
#if TEST_BLE_ECHO_VCOM
	type_tracetx,
	type_tracerx
#endif
}  __attribute((packed)) msgtype_t;


typedef struct {
	uint32_t 	ts;
	uint8_t 	type;
	uint8_t		devnum;
	union {
		int16_t 	rssi;
		uint16_t 	infocode;
#if TEST_BLE_ECHO_VCOM
#define TRACE_LEN 60
		char     	line[TRACE_LEN];
#endif
	} __attribute((packed));
} __attribute((packed)) resmsg_t;
#endif /* RESMSG_H_ */
