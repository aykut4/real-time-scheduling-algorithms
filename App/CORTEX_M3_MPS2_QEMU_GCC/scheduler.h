#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include <semphr.h>
#include <timers.h>
#include <list.h>

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

/* Real-Time scheduling algorithms available. */
#define schedSCHEDULING_POLICY_RMS 1 		/* Rate-monotonic scheduling */
#define schedSCHEDULING_POLICY_EDF 3 		/* Earliest deadline first */
#define schedSCHEDULING_POLICY_MUF 2		/* Maximum urgency first */

/* Configure scheduling policy by setting this define to the appropriate one. */
#define schedSCHEDULING_POLICY schedSCHEDULING_POLICY_RMS

/* 
 * Naive implementation: Large overhead during context switch.
*/
#define schedEDF_NAIVE 1 			/* Naive EDF implementation. */


/* Maximum number of periodic tasks that can be created. (Scheduler task is
 * not included, but Polling Server is included) */
#define schedMAX_NUMBER_OF_PERIODIC_TASKS 10

#if( schedSCHEDULING_POLICY == schedSCHEDULING_POLICY_EDF )
	/* Set this define to 1 to enable the scheduler task. This define must be set to 1
 	 * when using following features:
 	 * EDF scheduling policy, Timing-Error-Detection of execution time,
 	 * Timing-Error-Detection of deadline, Polling Server. */
	#define schedUSE_SCHEDULER_TASK 1
#else
	#define schedUSE_SCHEDULER_TASK 0
#endif

#if( schedUSE_SCHEDULER_TASK == 1 )
	/* Priority of the scheduler task. */
	#define schedSCHEDULER_PRIORITY ( configMAX_PRIORITIES - 1 )
	/* Stack size of the scheduler task. */
	#define schedSCHEDULER_TASK_STACK_SIZE 1000
	/* The period of the scheduler task in software ticks. */
	#define schedSCHEDULER_TASK_PERIOD pdMS_TO_TICKS( 100 )
	
	/* This define needs to be configured port specifically. For some ports
	 * it is portYIELD_FROM_ISR and for others it is portEND_SWITCHING_ISR. */
	#define schedYIELD_FROM_ISR( xSwitchingRequired ) portEND_SWITCHING_ISR( xSwitchingRequired )
#endif /* schedUSE_SCHEDULER_TASK */


/* This function must be called before any other function call from scheduler.h. */
void vSchedulerInit( void );

/* Creates a periodic task.
 *
 * pvTaskCode: The task function.
 * pcName: Name of the task.
 * usStackDepth: Stack size of the task in words, not bytes.
 * pvParameters: Parameters to the task function.
 * uxPriority: Priority of the task. (Only used when scheduling policy is set to manual)
 * pxCreatedTask: Pointer to the task handle.
 * xPhaseTick: Phase given in software ticks. Counted from when vSchedulerStart is called.
 * xPeriodTick: Period given in software ticks.
 * xMaxExecTimeTick: Worst-case execution time given in software ticks.
 * xDeadlineTick: Relative deadline given in software ticks.
 * */
void vSchedulerPeriodicTaskCreate( TaskFunction_t pvTaskCode, const char *pcName, UBaseType_t uxStackDepth, void *pvParameters, UBaseType_t uxPriority,
		TaskHandle_t *pxCreatedTask, TickType_t xPhaseTick, TickType_t xPeriodTick, TickType_t xMaxExecTimeTick, TickType_t xDeadlineTick );

/* Deletes a periodic task associated with the given task handle. */
void vSchedulerPeriodicTaskDelete( TaskHandle_t xTaskHandle );

/* Starts scheduling tasks. */
void vSchedulerStart( void );

#endif /* SCHEDULER_H*/