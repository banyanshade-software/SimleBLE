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

void StartBleTask(void const * argument)
{
	itm_debug1(DBG_BLE, "STRTb", 0);
	for (;;) {
		osDelay(1000);
	}
}
