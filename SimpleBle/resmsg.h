/*
 * resmsg.h
 *
 *  Created on: Feb 15, 2024
 *      Author: Daniel BRAUN
 */

#ifndef RESMSG_H_
#define RESMSG_H_


typedef struct {
	uint32_t 	ts;
	uint8_t 	type;
	uint8_t		devnum;
	union {
		int16_t 	rssi;
		uint16_t 	infocode;
	} __attribute((packed));
} __attribute((packed)) resmsg_t;
#endif /* RESMSG_H_ */
