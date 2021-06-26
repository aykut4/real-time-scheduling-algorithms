#ifndef SCHEDULE_H
#define SCHEDULE_H

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

#define RM_SCHEDULING 1 		// Rate monotonic
#define EDF_SCHEDULING 2 		// Earliest deadline first
#define MUF_SCHEDULING 3		// Maximum urgency first
#define SCHEDULING_ALGORITHM EDF_SCHEDULING

#define MAX_NUM_TASKS 10

#define RUNNING_STATE 1
#define BLOCKED_STATE 2
#define SUSPENDED_STATE 3

#define LOCAL_STORAGE_INDEX 0


void SchedulerInit      ( void );
void SchedulerStart     ( void );
void SchedulerTaskDelete( TaskHandle_t taskHandle );
void SchedulerTaskCreate( TaskFunction_t taskFunc, const char *taskName, UBaseType_t stackSize,
                          void *parameters, UBaseType_t priority, TaskHandle_t *taskHandle,
                          TickType_t readyTime, TickType_t period, TickType_t executionTime );

#endif /* SCHEDULE_H */