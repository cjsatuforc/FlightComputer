/*
 * syscon.c
 *
 *  Created on: 03/01/2013
 *      Author: alan
 */

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "board.h"
#include "qESC.h"
#include "leds.h"
#include "taskList.h"
#include "qFSM.h"
#include "qUART.h"
#include "qWDT.h"

#include "States.h"
#include "DebugConsole.h"

state_name_t systemState;
xQueueHandle StatesQueue;

void qFSM_ChangeState(state_name_t nextState){
	xQueueSend(StatesQueue,&nextState,portMAX_DELAY);
}


void SystemController(void * pvParams){

	state_name_t newState;
	state_name_t InitialState = (state_name_t) pvParams;
	systemState = STATE_RESET;

	vTaskDelay(3000/portTICK_RATE_MS);

	if (qUART_Init(UART_GROUNDCOMM,57600,8,QUART_PARITY_NONE,1)==RET_ERROR){
		while(1);
	}


	qFSM_registerState(STATE_INIT,"INIT",Init_onEntry,Init_onExit);
	qFSM_registerState(STATE_IDLE,"IDLE",Idle_onEntry,Idle_onExit);
	//qFSM_registerState(STATE_FLYNG,"FLY",Fligth_Manual_onEntry,Fligth_Manual_onExit);

	// Create the system queue
	StatesQueue = xQueueCreate(1,sizeof(state_name_t));

	// Inject the first State
	qFSM_ChangeState(InitialState);

	// --------------------------------------------------
	//	Main autopilot programm
	// --------------------------------------------------
	for (;;){
		xQueueReceive(StatesQueue,&newState,portMAX_DELAY);

		if (systemState == STATE_RESET){
			// The first entry is different, there is no onExit
			ConsolePuts_("===================================\r\n",BLUE);
			ConsolePuts_("SYSTEM RESET!\r\n",BLUE);

			systemState = newState;
			sysStates[systemState].onEntry(NULL);

		}else{
			if (TransitionValid(systemState,newState,transitionTable)==YES){
				ConsolePuts_("STATE TRANSISTION ",YELLOW);
				ConsolePuts_(sysStates[systemState].stateName,YELLOW);
				ConsolePuts_(" -> ",YELLOW);
				ConsolePuts_(sysStates[newState].stateName,YELLOW);
				ConsolePuts_(" [OK]\r\n",GREEN);

				if (sysStates[systemState].onExit!=NULL){
					sysStates[systemState].onExit(NULL);
				}
				systemState = newState;
				if (sysStates[systemState].onEntry==NULL){
					ConsolePuts_("STATE TRANSISTION ",RED);
					ConsolePuts_(sysStates[systemState].stateName,RED);
					ConsolePuts_(" -> ",RED);
					ConsolePuts_(sysStates[newState].stateName,RED);
					ConsolePuts_("[ERROR] DEAD END\r\n",RED);
				}else{
					sysStates[systemState].onEntry(NULL);
				}

			}else{

				ConsolePuts_("STATE TRANSISTION ",RED);
				ConsolePuts_(sysStates[systemState].stateName,RED);
				ConsolePuts_(" -> ",RED);
				ConsolePuts_(sysStates[newState].stateName,RED);
				ConsolePuts_(" [ERROR]\r\n",RED);
			}
		}


	}

}