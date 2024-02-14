/*
 * misc.c
 *
 *  Created on: Feb 14, 2024
 *      Author: danielbraun
 */


#include <stdint.h>
#include "misc.h"
#include "main.h"

#include "stm32g4xx_hal.h"

void flash_led(void)
{
#ifndef TRAIN_SIMU

#ifndef BOARD_LED_GPIO_Port
#ifdef  LD2_GPIO_Port
#define BOARD_LED_GPIO_Port LD2_GPIO_Port
#endif
#endif


#ifndef BOARD_LED_Pin
#ifdef  LD2_Pin
#define BOARD_LED_Pin LD2_Pin
#endif
#endif

#ifdef BOARD_LED_Pin
	HAL_GPIO_TogglePin(BOARD_LED_GPIO_Port,BOARD_LED_Pin);
	//HAL_GPIO_TogglePin(GPIOC,GPIO_PIN_13);
#endif

#endif /* TRAIN_SIMU */
}



#include "cmsis_os.h"
