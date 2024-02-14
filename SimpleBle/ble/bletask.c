/*
 * bletask.c
 *
 *  Created on: Feb 14, 2024
 *      Author: danielbraun
 */


#include <stdint.h>
#include "../misc.h"
#include "main.h"
#include "bletask.h"
#include "../itm_debug.h"


// HM-10 ressources
// https://people.ece.cornell.edu/land/courses/ece4760/PIC32/uart/HM10/DSD%20TECH%20HM-10%20datasheet.pdf

void StartBleTask(void const * argument)
{
	itm_debug1(DBG_BLE, "STRTb", 0);
	for (;;) {
		itm_debug1(DBG_BLE, "hop", 0);
		osDelay(1000);
	}
}
