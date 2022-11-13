/*
 * FreeRTOS Kernel V10.2.0
 * Copyright (C) 2019 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://www.FreeRTOS.org
 * http://aws.amazon.com/freertos
 *
 * 1 tab == 4 spaces!
 */

/* 
	NOTE : Tasks run in system mode and the scheduler runs in Supervisor mode.
	The processor MUST be in supervisor mode when vTaskStartScheduler is 
	called.  The demo applications included in the FreeRTOS.org download switch
	to supervisor mode prior to main being called.  If you are not using one of
	these demo application projects then ensure Supervisor mode is used.
*/


/*
 * Creates all the demo application tasks, then starts the scheduler.  The WEB
 * documentation provides more details of the demo application tasks.
 * 
 * Main.c also creates a task called "Check".  This only executes every three 
 * seconds but has the highest priority so is guaranteed to get processor time.  
 * Its main function is to check that all the other tasks are still operational.
 * Each task (other than the "flash" tasks) maintains a unique count that is 
 * incremented each time the task successfully completes its function.  Should 
 * any error occur within such a task the count is permanently halted.  The 
 * check task inspects the count of each task to ensure it has changed since
 * the last time the check task executed.  If all the count variables have 
 * changed all the tasks are still executing error free, and the check task
 * toggles the onboard LED.  Should any task contain an error at any time 
 * the LED toggle rate will change from 3 seconds to 500ms.
 *
 */

/* Standard includes. */
#include "STD_TYPES.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "lpc21xx.h"
#include "queue.h"

/* Peripheral includes. */
#include "serial.h"
#include "GPIO.h"

/*-----------------------------------------------------------*/

/* Constants to setup I/O and processor. */
#define mainBUS_CLK_FULL	( ( unsigned char ) 0x01 )

/* Constants for the ComTest demo application tasks. */
#define mainCOM_TEST_BAUD_RATE	( ( unsigned long ) 9600 )


TaskHandle_t Button_1_Monitor_Handle = NULL;
TaskHandle_t Button_2_Monitor_Handle = NULL;
TaskHandle_t Periodic_Transmitter_Handle = NULL;
TaskHandle_t Uart_Receiver_Handle = NULL;
TaskHandle_t Load_1_Simulation_Handle = NULL;
TaskHandle_t Load_2_Simulation_Handle = NULL;

ButtonState_t B1;
ButtonState_t B2;

QueueHandle_t MessageQueue1 = NULL;
QueueHandle_t MessageQueue2 = NULL;
QueueHandle_t MessageQueue3 = NULL;

int idletask_in_time,idletask_out_time,idletask_total_time;
int system_time,cpu_load;

/*
 * Configure the processor for use with the Keil demo board.  This is very
 * minimal as most of the setup is managed by the settings in the project
 * file.
 */
static void prvSetupHardware( void );
/*-----------------------------------------------------------*/
void Button_1_Monitor_Task(void* pvParameters)
{
	pinState_t B1_CurrentState;
	pinState_t B1_PreviousState = GPIO_read(PORT_1, PIN0);
	TickType_t xLastWakeTime = xTaskGetTickCount();
	

	vTaskSetApplicationTaskTag(NULL,(void*) 5);
	for(;;)
	{
		B1_CurrentState = GPIO_read(PORT_1, PIN0);
		
		if ( ( B1_CurrentState == PIN_IS_HIGH ) && ( B1_PreviousState == PIN_IS_LOW ) )
		{
			/*Rising edge */
		B1.Edge_Flag = '+';
		
		}
		else if ( ( B1_CurrentState == PIN_IS_LOW ) && ( B1_PreviousState == PIN_IS_HIGH ) )
		{
			/*Falling edge */
		B1.Edge_Flag = '-';
		}
		else
		{
			/* No change*/
		B1.Edge_Flag = '=';
		}
		
		B1.ButtonID =1;
		B1_PreviousState = B1_CurrentState;
		

		xQueueOverwrite(MessageQueue1, &B1 );

		vTaskDelayUntil(&xLastWakeTime, 50);
	}
}

void Button_2_Monitor_Task(void* pvParameters)
{
	pinState_t B2_CurrentState;
	pinState_t B2_PreviousState = GPIO_read(PORT_1, PIN1);
	TickType_t xLastWakeTime = xTaskGetTickCount();
	

	vTaskSetApplicationTaskTag(NULL,(void*) 6);
	for(;;)
	{

		B2_CurrentState = GPIO_read(PORT_1, PIN1);

		if ( ( B2_CurrentState == PIN_IS_HIGH ) && ( B2_PreviousState == PIN_IS_LOW ) )
		{
			/* Rising edge */
			B2.Edge_Flag = '+';
		}
		else if ( ( B2_CurrentState == PIN_IS_LOW ) && ( B2_PreviousState == PIN_IS_HIGH ) )
		{
			/* Falling edge */
			B2.Edge_Flag = '-';
		}
		else
		{
			/* No change */
			B2.Edge_Flag = '=';
		}
		
		B2_PreviousState = B2_CurrentState;
		B2.ButtonID =2;
		xQueueOverwrite(MessageQueue2, &B2 );
		

		vTaskDelayUntil(&xLastWakeTime, 50);
	}
}

void Periodic_Transmitter_Task(void* pvParameters)
{
	TickType_t xLastWakeTime = xTaskGetTickCount();
	u8 i = 0;
	char String[6];
	strcpy(String, "\nIDLE");
	String[5] = '\0';
	vTaskSetApplicationTaskTag(NULL,(void*) 3);
	for(;;)
	{

		for (i = 0; i < 5; i++)
		{
			xQueueSend(MessageQueue3, String + i, 100);
		}

		vTaskDelayUntil(&xLastWakeTime, 100);
	}
}

void Uart_Receiver_Task(void* pvParameters)
{
	TickType_t xLastWakeTime = xTaskGetTickCount();
	ButtonState_t B1_Data;
	ButtonState_t B2_Data;
	u8 StringReceived[6];
	u8 i = 0, flag = 0;
	vTaskSetApplicationTaskTag(NULL,(void*) 4);
	for(;;)
	{
		i = 0;
		if ( xQueueReceive(MessageQueue1, &B1_Data, 0) && ( B1_Data.Edge_Flag != '=' ) )
		{	
			xSerialPutChar('\n');		
			xSerialPutChar('B');
			xSerialPutChar('1');
			xSerialPutChar(':');
			xSerialPutChar(' ');
			xSerialPutChar(B1_Data.Edge_Flag);
			
		}
		else
		{
				
			xSerialPutChar(' ');
		}
		
		i = 0;
		if ( xQueueReceive(MessageQueue2, &B2_Data, 0) && ( B2_Data.Edge_Flag != '=' ) )
		{
			
			xSerialPutChar('\n');		
			xSerialPutChar('B');
			xSerialPutChar('2');
			xSerialPutChar(':');
			xSerialPutChar(' ');
			xSerialPutChar(B2_Data.Edge_Flag);
			
			
		}
		else
		{
			xSerialPutChar(' ');
		}
		
		i = 0;

		if ( (uxQueueMessagesWaiting(MessageQueue3) != 0) && (flag == 0) )
		{	
			flag =1;
			for(i = 0; i < 5; i++)
			{
				xQueueReceive(MessageQueue3, StringReceived + i, 0);
			}

			for(i = 0; i < 5; i++)
			{
				xSerialPutChar(StringReceived[i]);
			}
			
			xQueueReset(MessageQueue3);
		}

		#if ( INCLUDE_vTaskGetSystemStatistics == 1)
			vTaskGetSystemStatistics(SystemStatistics);
			vSerialPutString((uint8*)SystemStatistics, strlen(SystemStatistics));
		#endif

		vTaskDelayUntil(&xLastWakeTime, 20);
	}
}

void Load_1_Simulation_Task(void* pvParameters)
{
	TickType_t xLastWakeTime = xTaskGetTickCount();
	u32 i = 0;
	vTaskSetApplicationTaskTag(NULL,(void*) 1);
	for(;;)
	{
		for(i=0;i<33025;i++){ //5ms execution time
			i=i;
		}
		vTaskDelayUntil(&xLastWakeTime, 10);
	}
}

void Load_2_Simulation_Task(void* pvParameters)
{
	TickType_t xLastWakeTime = xTaskGetTickCount();
	u32 i = 0;
	vTaskSetApplicationTaskTag(NULL,(void*) 2);
	for(;;)
	{
		for(i=0;i<79380;i++){ //12 ms execution time
			i=i;
		}
		vTaskDelayUntil(&xLastWakeTime, 100);
	}
}

/*
 * Application entry point:
 * Starts all the other tasks, then starts the scheduler. 
 */
int main( void )

{
	/* Setup the hardware for use with the Keil demo board. */
	prvSetupHardware();
	

	MessageQueue1 = xQueueCreate(1, sizeof(ButtonState_t));
	MessageQueue2 = xQueueCreate(1, sizeof(ButtonState_t));
	MessageQueue3 = xQueueCreate(5, sizeof(char));

	/* Create Tasks here */
	xTaskPeriodicCreate(Button_1_Monitor_Task,
						"Button 1 Monitor Task",
						100, (void*)0,
						1,
						&Button_1_Monitor_Handle,
						50);
						
	xTaskPeriodicCreate(Button_2_Monitor_Task,
						"Button 2 Monitor Task",
						100,
						(void*)0,
						1,
						&Button_2_Monitor_Handle,
						50);
						
	xTaskPeriodicCreate(Periodic_Transmitter_Task,
						"Periodic Transmitter Task",
						100,
						(void*)0,
						1,
						&Periodic_Transmitter_Handle,
						100);
						
	xTaskPeriodicCreate(Uart_Receiver_Task,
						"Uart Receiver Task",
						100,
						(void*)0,
						1,
						&Uart_Receiver_Handle,
						20);
						
	xTaskPeriodicCreate(Load_1_Simulation_Task,
						"Load 1 Simulation Task",
						100,
						(void*)0,
						1,
						&Load_1_Simulation_Handle,
						10);
						
	xTaskPeriodicCreate(Load_2_Simulation_Task,
						"Load 2 Simulation Task",
						100,
						(void*)0,
						1,
						&Load_2_Simulation_Handle,
						100);
		
	/* Now all the tasks have been started - start the scheduler.
	NOTE : Tasks run in system mode and the scheduler runs in Supervisor mode.
	The processor MUST be in supervisor mode when vTaskStartScheduler is 
	called.  The demo applications included in the FreeRTOS.org download switch
	to supervisor mode prior to main being called.  If you are not using one of
	these demo application projects then ensure Supervisor mode is used here. */
	vTaskStartScheduler();

	/* Should never reach here!  If you do then there was not enough heap
	available for the idle task to be created. */
	for( ;; );
}
/*-----------------------------------------------------------*/
/* Implement Tick Hook */
void vApplicationTickHook(void)
{
	/* Write your code here! */
GPIO_write(PORT_1,PIN3,PIN_IS_HIGH);
GPIO_write(PORT_1,PIN3,PIN_IS_LOW);	
}

/* Function to reset timer 1 */
void timer1Reset(void)
{
	T1TCR |= 0x2;
	T1TCR &= ~0x2;
}

/* Function to initialize and start timer 1 */
static void configTimer1(void)
{
	T1PR = 1000;
	T1TCR |= 0x1;
}

static void prvSetupHardware( void )
{
	/* Perform the hardware setup required.  This is minimal as most of the
	setup is managed by the settings in the project file. */

	/* Configure UART */
	xSerialPortInitMinimal(mainCOM_TEST_BAUD_RATE);

	/* Configure GPIO */
	GPIO_init();
	
	/* Config trace timer 1 and read T1TC to get current tick */
	configTimer1();

	/* Setup the peripheral bus to be the same as the PLL output. */
	VPBDIV = mainBUS_CLK_FULL;
}
/*-----------------------------------------------------------*/