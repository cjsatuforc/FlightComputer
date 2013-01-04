/*
 * Config.c
 *
 *  Created on: Mar 3, 2012
 *      Author: Alan
 */

#include "FreeRTOS.h"
#include "task.h"

#include "qESC.h"
#include "qFSM.h"
#include "leds.h"
#include "qWDT.h"

#include "board.h"

/* ================================ */
/* Prototypes	 					*/
/* ================================ */

void Idle_Task(void * pvParameters);
void Idle_onEntry(void * pvParameters);
void Idle_onExit(void * pvParameters);

/* ================================ */
/* Private globals 					*/
/* ================================ */
static xTaskHandle hnd;

void Idle_onEntry(void * p){
	xTaskCreate(Idle_Task, ( signed char * ) "IDLE", 200, ( void * ) NULL, 1, &hnd );
	qWDT_Start(1000000);
}

void Idle_onExit(void * p){
	vTaskDelete(hnd);
}

void Idle_Task(void * pvParameters){
	int i,j;

	for (;;)
	{
		vTaskDelay(500/portTICK_RATE_MS);

		qLed_TurnOn(FRONT_LEFT_LED);
		qLed_TurnOn(FRONT_RIGHT_LED);
		qLed_TurnOn(REAR_LEFT_LED);
		qLed_TurnOn(REAR_RIGHT_LED);

		vTaskDelay(50/portTICK_RATE_MS);

		qLed_TurnOff(FRONT_LEFT_LED);
		qLed_TurnOff(FRONT_RIGHT_LED);
		qLed_TurnOff(REAR_LEFT_LED);
		qLed_TurnOff(REAR_RIGHT_LED);

		vTaskDelay(50/portTICK_RATE_MS);
	}
}
