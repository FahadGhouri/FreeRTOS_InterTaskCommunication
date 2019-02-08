/******************************************************************************
 * NOTE: Windows will not be running the FreeRTOS demo threads continuously, so
 * do not expect to get real time behaviour from the FreeRTOS Windows port, or
 * this demo application.  Also, the timing information in the FreeRTOS+Trace
 * logs have no meaningful units.  See the documentation page for the Windows
 * port for further information:
 * http://www.freertos.org/FreeRTOS-Windows-Simulator-Emulator-for-Visual-Studio-and-Eclipse-MingW.html

/* Standard includes. */
#include <stdio.h>
#include <assert.h>
#include <conio.h>

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "semphr.h"
#include "queue.h"

						/* Macro Definations */
#define QueueLengthSensor1			1
#define QueueLengthSensor2A			1
#define QueueLengthSensor2B			1
#define SensorTaskPriority			2
#define ControllerTaskPriority		3
#define SensorTaskStackSize			50
#define ControllerTaskStackSize		100
#define HotReserveTestTime_mS		5
#define Controller1FailTime_mS		2000

						/* Global Variables */
/* Queue Handles */
QueueHandle_t Sensor1_Data_Queue, Sensor2A_Data_Queue, Sensor2B_Data_Queue;

/* Queue Set Handle */
QueueSetHandle_t Sensor2_Data_QueueSet;

/* Task Handles */
TaskHandle_t Controller1Task_Handle, Controller2Task_Handle;

					/* Function Prototypes */
static void Sensor1Task(void *pvParameters);
static void Sensor2ATask(void *pvParameters);
static void Sensor2BTask(void *pvParameters);
static void ControllerTask(void *pvParameters);

void main_exercise( void )
{
	/* Initialize Queues */

	/* The idea is to keep the queue length as 1 to make sure always the latest sensor
	data is avilable for the controller task */
	/* Queue size is set to 16bit INT to accomodate all the data range generated from the
	sensor tasks */
	Sensor1_Data_Queue = xQueueCreate(QueueLengthSensor1, sizeof(uint16_t));
	Sensor2A_Data_Queue = xQueueCreate(QueueLengthSensor2A, sizeof(uint16_t));
	Sensor2B_Data_Queue = xQueueCreate(QueueLengthSensor2B, sizeof(uint16_t));

	/* Initialize Queue Set for Sensor 2 Data Pair */

	/* The Queue of 2A and 2B Sensor data are paired together using the QueueSet function
	offered by FreeRTOS to make the selection between data available on these two queues 
	easier */
	Sensor2_Data_QueueSet = xQueueCreateSet((QueueLengthSensor2A+QueueLengthSensor2B));
	xQueueAddToSet(Sensor2A_Data_Queue, Sensor2_Data_QueueSet);
	xQueueAddToSet(Sensor2B_Data_Queue, Sensor2_Data_QueueSet);
	
	/* Create Tasks */
	xTaskCreate(Sensor1Task, "Sensor1", SensorTaskStackSize, NULL, SensorTaskPriority, NULL);
	xTaskCreate(Sensor2ATask, "Sensor2A", SensorTaskStackSize, NULL, SensorTaskPriority, NULL);
	xTaskCreate(Sensor2BTask, "Sensor2B", SensorTaskStackSize, NULL, SensorTaskPriority, NULL);
	xTaskCreate(ControllerTask, "Controller1", ControllerTaskStackSize, &Controller1Task_Handle, ControllerTaskPriority, &Controller1Task_Handle);
	xTaskCreate(ControllerTask, "Controller2", ControllerTaskStackSize, &Controller2Task_Handle, ControllerTaskPriority, &Controller2Task_Handle);

	/* Start Schedular */
	vTaskStartScheduler();

	for (;; );
}
/*-----------------------------------------------------------*/


static void Sensor1Task(void *pvParameters) {
	pvParameters = NULL; //For Complier Warnings
	TickType_t xLastWakeTime;
	/* Run Every 200 mSec */
	const TickType_t xFrequency = pdMS_TO_TICKS(200);
	/* Initialise the xLastWakeTime variable with the current time */
	xLastWakeTime = xTaskGetTickCount();

	/* Sensor Synthetic Data Value */
	uint16_t SensorData = 100;

	for (;;) {
		/* Write Data to Sensor Data Queue */
		/* Data is Written in NonBlocking mode with OverWrite if a value is already
		present on the queue */
		xQueueOverwrite(Sensor1_Data_Queue, &SensorData);

		/* Increase Sensor Data and Wrap around if out of range */
		SensorData++;
		if (199 < SensorData) {
			SensorData = 100;
		}

		/* Wake Up in Next Period */
		vTaskDelayUntil(&xLastWakeTime, xFrequency);
	}
}
/*-----------------------------------------------------------*/

static void Sensor2ATask(void *pvParameters) {
	pvParameters = NULL; //For Complier Warnings
	TickType_t xLastWakeTime;
	/* Run Every 500 mSec */
	const TickType_t xFrequency = pdMS_TO_TICKS(500);
	/* Initialise the xLastWakeTime variable with the current time */
	xLastWakeTime = xTaskGetTickCount();

	/* Sensor Synthetic Data Value */
	uint16_t SensorData = 200;

	for (;;) {
		/* Write Data to Sensor Data Queue */
		/* Data is Written in NonBlocking mode with OverWrite if a value is already
		present on the queue */
		xQueueOverwrite(Sensor2A_Data_Queue, &SensorData);

		/* Increase Sensor Data and Wrap around if out of range */
		SensorData++;
		if (249 < SensorData) {
			SensorData = 200;
		}

		/* Wake Up in Next Period */
		vTaskDelayUntil(&xLastWakeTime, xFrequency);
	}
}
/*-----------------------------------------------------------*/

static void Sensor2BTask(void *pvParameters) {
	pvParameters = NULL; //For Complier Warnings
	TickType_t xLastWakeTime;
	/* Run Every 1400 mSec */
	const TickType_t xFrequency = pdMS_TO_TICKS(1400);
	/* Initialise the xLastWakeTime variable with the current time */
	xLastWakeTime = xTaskGetTickCount();

	/* Sensor Synthetic Data Value */
	uint16_t SensorData = 250;

	for (;;) {
		/* Write Data to Sensor Data Queue */
		/* Data is Written in NonBlocking mode with OverWrite if a value is already
		present on the queue */
		xQueueOverwrite(Sensor2B_Data_Queue, &SensorData);

		/* Increase Sensor Data and Wrap around if out of range */
		SensorData++;
		if (299 < SensorData) {
			SensorData = 250;
		}

		/* Wake Up in Next Period */
		vTaskDelayUntil(&xLastWakeTime, xFrequency);
	}
}
/*-----------------------------------------------------------*/

static void ControllerTask(void *pvParameters) {
	TaskHandle_t* Handle = (TaskHandle_t*)(pvParameters);
	//pvParameters = NULL; //For Complier Warnings
	uint16_t Sensor1Data, Sensor2Data;
	QueueSetMemberHandle_t Sensor2Queue;

	for (;;) {

		/* If this task is an instance of Controller 2 it checks for Controller 1 to be active and functioning */
		/* To demostrate this in FreeRTOS, the Controller 2 tasks keeps on waiting untill the Controller 1
		task is no more valid */
		if (0 == strcmp("Controller2", pcTaskGetName(*Handle))) {
			while (eDeleted != eTaskGetState(Controller1Task_Handle)) {
				vTaskDelay(pdMS_TO_TICKS(HotReserveTestTime_mS));
			}
		}

		/* Read Value from Sensors */
		/* Sensor 2 is being used as the first blocking source for the controller task */
		/* This is because of lower Frequency of Sensor 2 Tasks */
		/* The Data will be read by the help of queue-sets to arbitate between Sensor2A Data Queue
		and Sensor 2B Data Queue */
		/* This Mechanism helps eliminate the need for Event Groups Function offered by FreeRTOS */
		Sensor2Queue = xQueueSelectFromSet(Sensor2_Data_QueueSet, portMAX_DELAY);

		/* Use the Recieved Queue Handle to get Sensor2 Data */
		if (pdFALSE == xQueueReceive(Sensor2Queue, &Sensor2Data, 0)) {
			printf("Debug: Sensor 2 Queue Error \n");
		}
		/* Recieve value from Sensor 1 */
		xQueueReceive(Sensor1_Data_Queue, &Sensor1Data, portMAX_DELAY);

		/* Check the Special Case of Deleting Controller 1 Task after 2000 ticks */
		/* Controller 1 Task Fails After Consuming Data from Queues */
		if (0 == strcmp("Controller1", pcTaskGetName(*Handle)) && Controller1FailTime_mS < xTaskGetTickCount()) {
			printf("%s has had an error at %d\n", pcTaskGetName(*Handle), xTaskGetTickCount());
			/* Delete Task */
			vTaskDelete(NULL);
		}

		/* Print to console */
		printf("%s has received sensor Data at %d; Sensor1:%d; Sensor2:%d\n", pcTaskGetName(*Handle),xTaskGetTickCount(),Sensor1Data,Sensor2Data);
		
	}
}
/*-----------------------------------------------------------*/


