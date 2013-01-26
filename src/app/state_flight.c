#include "FreeRTOS.h"
#include "task.h"

#include "qESC.h"
#include "qFSM.h"
#include "leds.h"
#include "qWDT.h"

#include "board.h"
#include "DebugConsole.h"
#include "qCOMMS.h"
#include "MPU6050.h"

#include "qPIDs.h"
#include "quadrotor.h"
#include "taskList.h"

#include "telemetry.h"
#include "joystick.h"
#include "qFSM.h"
#include "States.h"
#include "qWDT.h"
#include "lpc17xx_gpio.h"
/* ================================ */
/* Prototypes	 					*/
/* ================================ */

void Flight_Task(void * pvParameters);
void Flight_onEntry(void * pvParameters);
void Fligth_onExit(void * pvParameters);

/* ================================ */
/* Private globals 					*/
/* ================================ */
static xTaskHandle hnd;
static xTaskHandle BeaconHnd;

#define Z_C		0
#define PHI_C	1
#define THETA_C	2
#define PSI_C	3

#define K_Z		700
#define K_PHI	200
#define K_THETA	200
#define K_PSI	300

float control[4]={0.0};
uint16_t inputs[4]={0};

qPID ctrl[4];

//=======================================================
uint32_t signal_time[]={0,2*1000/5,6*1000/5,10*1000/5,14*1000/5};
float signal_values[]={0.0,180.0,0.0,-180.0,0.0};
int j;
uint32_t signal_t=0;
//=======================================================

static 	uint8_t fifoBuffer[64]; // FIFO storage buffer
xSemaphoreHandle mpuSempahore;

float map(long x, long in_min, long in_max, float out_min, float out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void beacon(void * pvParameters){
	int i;

	for(;;){
		for (i=1;i<5;i++) qLed_TurnOn(leds[i]);
		vTaskDelay(50/portTICK_RATE_MS);
		for (i=1;i<5;i++) qLed_TurnOff(leds[i]);
		vTaskDelay(50/portTICK_RATE_MS);
		for (i=1;i<5;i++) qLed_TurnOn(leds[i]);
		vTaskDelay(50/portTICK_RATE_MS);
		for (i=1;i<5;i++) qLed_TurnOff(leds[i]);
		vTaskDelay(50/portTICK_RATE_MS);

		vTaskDelay(800/portTICK_RATE_MS);
	}



}

void Flight_onEntry(void *p){
	ConsolePuts_("FLIGHT State: onEntry\r\n",BLUE);
	xTaskCreate( Flight_Task, ( signed char * ) "IDLE", 2000, ( void * ) NULL, FLIGHT_PRIORITY, &hnd );
	xTaskCreate( beacon, ( signed char * ) "BEACON", 100, ( void * ) NULL, 1, &BeaconHnd );
	StartTelemetry(20);
}

void Flight_onExit(void *p){
	ConsolePuts_("FLIGHT State: onExit\r\n",BLUE);
	vTaskDelete(hnd);
	vTaskDelete(BeaconHnd);
	//MPU6050_setDMPEnabled(FALSE);
	qLed_TurnOff(STATUS_LED);
	StopTelemetry();
}

void Flight_Task(void * pvParameters){
	for (;;){
		vTaskDelay(100/portTICK_RATE_MS);
	}
}

#if 0
void Flight_Task(void * pvParameters){
	uint8_t i;
	uint8_t zc;

	portTickType xLastWakeTime;
	xLastWakeTime = xTaskGetTickCount();
	int16_t buffer[3];

	// MPU control/status vars
	uint8_t mpuIntStatus;   // holds actual interrupt status byte from MPU
	uint16_t packetSize;    // expected DMP packet size (default is 42 bytes)
	uint16_t fifoCount;     // count of all bytes currently in FIFO


	//uint8_t counter = 4;


	for (i=1;i<4;i++){
		ctrl[i].AntiWindup = ENABLED;
		ctrl[i].Bumpless = ENABLED;

		ctrl[i].Mode = AUTOMATIC;
		ctrl[i].OutputMax = 1.0;
		ctrl[i].OutputMin = -1.0;

		ctrl[i].Ts = 0.005;

		ctrl[i].b = 1.0;
		ctrl[i].c = 1.0;
	}

	ctrl[PHI_C].K = 0.02;
	ctrl[PHI_C].Ti = 1/0.03;
	ctrl[PHI_C].Td = 0.000;
	ctrl[PHI_C].Nd = 5;

    ctrl[THETA_C].K = 0.02;
    ctrl[THETA_C].Ti = 1/0.03;
    ctrl[THETA_C].Td = 0.000;
    ctrl[THETA_C].Nd = 5;

    ctrl[PSI_C].K = 0.1;
    ctrl[PSI_C].Ti = 1/0.2;
    ctrl[PSI_C].Td = 0.000;
    ctrl[PSI_C].Nd = 5;

    qPID_Init(&ctrl[PHI_C]);
    qPID_Init(&ctrl[THETA_C]);
    qPID_Init(&ctrl[PSI_C]);

    //uint32_t signal_t=0;
	vSemaphoreCreateBinary(mpuSempahore);
	xSemaphoreTake(mpuSempahore,0);


	MPU6050_setDMPEnabled(TRUE);
	mpuIntStatus = MPU6050_getIntStatus();
	// get expected DMP packet size for later comparison
	packetSize = MPU6050_dmpGetFIFOPacketSize();


	for(;;){

		// Wait here for MPU DMP interrupt
		xSemaphoreTake(mpuSempahore,portMAX_DELAY); //FIXME: instead of portMAX it would be nice to hae a time out for errors
		qLed_TurnOn(STATUS_LED);

		//-----------------------------------------------------------------------
		// Joystick mapping
		//-----------------------------------------------------------------------

		if (Joystick.left_pad.y>127){
			zc = 127;
		}else{
			zc = 255 - Joystick.left_pad.y;
		}

		sv.setpoint[Z_C] = map(zc,127,255,0.0,1.0);
		sv.setpoint[PHI_C] = map(Joystick.right_pad.x,0,255,-90.0,90.0);
		sv.setpoint[THETA_C] = map(Joystick.right_pad.y,0,255,-90.0,90.0);
		sv.setpoint[PSI_C] = 0.0; //map(Joystick.left_pad.x,0,255,-180.0,180.0); THIS IS FOR KILL-ROT ON YAW


		//-----------------------------------------------------------------------
		// MPU Data adquisition
		//-----------------------------------------------------------------------

		portENTER_CRITICAL();

		// reset interrupt flag and get INT_STATUS byte
		mpuIntStatus = MPU6050_getIntStatus();

		// get current FIFO count
		fifoCount = MPU6050_getFIFOCount();

		// check for overflow (this should never happen unless our code is too inefficient)
		if ((mpuIntStatus & 0x10) || fifoCount == 1024) {
		    // reset so we can continue cleanly
		    MPU6050_resetFIFO();
		    ConsolePuts_("DMP FIFO OVERFLOW!\r\n",RED);

		// otherwise, check for DMP data ready interrupt (this should happen frequently)
		} else if (mpuIntStatus & 0x02) {
		    // wait for correct available data length, should be a VERY short wait
		    while (fifoCount < packetSize) fifoCount = MPU6050_getFIFOCount();

		    // read a packet from FIFO
		    MPU6050_getFIFOBytes(fifoBuffer, packetSize);

		    // track FIFO count here in case there is > 1 packet available
		    // (this lets us immediately read more without waiting for an interrupt)
		    fifoCount -= packetSize;

		 }

	    portEXIT_CRITICAL();

		//-----------------------------------------------------------------------
		// Angular velocity data
		//-----------------------------------------------------------------------

#ifndef USE_GYRO_RAW
		    MPU6050_dmpGetGyro(&buffer[0],fifoBuffer);
			sv.omega[0] = -buffer[0];
			sv.omega[1] = buffer[1];
			sv.omega[2] = -buffer[2];
#else
		// DAQ
		MPU6050_getRotation(&buffer[0],&buffer[1],&buffer[2]);
		//MPU6050_dmpGetGyro(&buffer[0],fifoBuffer);

		// MPU axes aligment to Quad body axes
		sv.omega[0] = -(buffer[0]-settings.gyroBias[0]);
		sv.omega[1] = (buffer[1]-settings.gyroBias[1]);
		sv.omega[2] = -(buffer[2]-settings.gyroBias[2]);

		// MPU gyro scale transformation
		sv.omega[0] = sv.omega[0]/16.4;
		sv.omega[1] = sv.omega[1]/16.4;
		sv.omega[2] = sv.omega[2]/16.4;
#endif

		//-----------------------------------------------------------------------
		// Attitude data
		//-----------------------------------------------------------------------
		MPU6050_dmpGetEuler(&sv.attitude[0], fifoBuffer);


		//-----------------------------------------------------------------------
		// PID Process
		//-----------------------------------------------------------------------
		sv.CO[PHI_C] = qPID_Process(&ctrl[PHI_C],sv.setpoint[PHI_C],sv.omega[0],NULL);
		sv.CO[THETA_C] = qPID_Process(&ctrl[THETA_C],sv.setpoint[THETA_C],sv.omega[1],NULL);
		sv.CO[PSI_C] = qPID_Process(&ctrl[PSI_C],sv.setpoint[PSI_C],sv.omega[2],NULL);


		//-----------------------------------------------------------------------
		// Output stage
		//-----------------------------------------------------------------------
#ifdef	USE_FULL_AUTOPILOT
		control[Z_C] = sv.setpoint[Z_C];
		control[PHI_C] = sv.CO[PHI_C];
		control[THETA_C] = sv.CO[THETA_C];
		control[PSI_C] = -sv.CO[PSI_C];
#else
		control[Z_C] = 0.3;
		control[PHI_C] = sv.CO[PHI_C];
		control[THETA_C] = 0.0;
		control[PSI_C] = 0.0;
#endif
		// Output state
		inputs[0] = (	control[Z_C]*K_Z - control[PHI_C]*K_PHI - control[THETA_C]*K_THETA - control[PSI_C]*K_PSI	);
		inputs[1] = (	control[Z_C]*K_Z - control[PHI_C]*K_PHI + control[THETA_C]*K_THETA + control[PSI_C]*K_PSI	);
		inputs[2] = (	control[Z_C]*K_Z + control[PHI_C]*K_PHI + control[THETA_C]*K_THETA - control[PSI_C]*K_PSI	);
		inputs[3] = (	control[Z_C]*K_Z + control[PHI_C]*K_PHI - control[THETA_C]*K_THETA + control[PSI_C]*K_PSI	);

		// Motor command
#define SET_MOTOR_OUTPUT
#ifdef SET_MOTOR_OUTPUT
		qESC_SetOutput(MOTOR1,inputs[0]);
		qESC_SetOutput(MOTOR2,inputs[1]);
		qESC_SetOutput(MOTOR3,inputs[2]);
		qESC_SetOutput(MOTOR4,inputs[3]);
#endif
		qLed_TurnOff(STATUS_LED);
		//vTaskDelayUntil( &xLastWakeTime, 5/portTICK_RATE_MS );
	}
}

void EINT3_IRQHandler(void)
{
	static signed portBASE_TYPE xHigherPriorityTaskWoken;
    xHigherPriorityTaskWoken = pdFALSE;

	if(GPIO_GetIntStatus(0, 4, 1))
	{
		GPIO_ClearInt(0,(1<<4));
		if (mpuSempahore!=NULL){
			xSemaphoreGiveFromISR(mpuSempahore,&xHigherPriorityTaskWoken);
		}
	}
	portEND_SWITCHING_ISR( xHigherPriorityTaskWoken );
}

#endif
