/*
 * serialcom.c
 *
 *  Created on: Feb 14, 2024
 *      Author: danielbraun
 */

#include <stdint.h>
#include "../misc.h"
#include "main.h"
#include "serialcom.h"
#include "../itm_debug.h"

void StartComTask(void const * argument)
{
	itm_debug1(DBG_COM, "STRTc", 0);
	for (;;) {
		osDelay(500);
		flash_led();
	}
}
