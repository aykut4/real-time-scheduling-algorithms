#include "schedule.h"
//===============================================================================================
typedef struct SchedulerTaskControlBlock
{
	TaskFunction_t taskFunc;
	const char *taskName;
	UBaseType_t stackSize;
	void *parameters;
	UBaseType_t priority;
	TaskHandle_t *taskHandle;
	TickType_t readyTime;
	TickType_t period;
	TickType_t softDeadline;
    TickType_t hardDeadline;
	BaseType_t taskState;       // RUNNING, BLOCKED, SUSPENDED
	BaseType_t bEmptyBlock; 	// empty == pdTRUE, not empty == pdFALSE
    TickType_t previousWakeTime;
    TickType_t executionTime;

} SchedulerTaskControlBlock_t;
//===============================================================================================
static SchedulerTaskControlBlock_t gTaskControlBlocks[MAX_NUM_TASKS] = {0};
static BaseType_t gTaskCounter = 0;
static TickType_t gSystemStartTime = 0;
//===============================================================================================
static void SchedulerParentTaskFunc(void *parameters)
{
    SchedulerTaskControlBlock_t *childTask = (SchedulerTaskControlBlock_t *)pvTaskGetThreadLocalStoragePointer( xTaskGetCurrentTaskHandle(), LOCAL_STORAGE_INDEX );
	configASSERT(childTask != NULL);

    if(childTask->readyTime != 0)
    {
        vTaskDelayUntil(&childTask->previousWakeTime, childTask->readyTime);
    }
    if (childTask->readyTime == 0)
    {
        childTask->previousWakeTime = gSystemStartTime;
    }

    for( ;; )
	{
        #if(SCHEDULING_ALGORITHM == EDF_SCHEDULING || SCHEDULING_ALGORITHM == MUF_SCHEDULING)
            RunSchedulerTask();
        #endif
		childTask->taskState = RUNNING_STATE;

		//taskENTER_CRITICAL();
		//printf( "tickcount %d Task %s Abs deadline %d lastWakeTime %d prio %d Handle %x\r\n", xTaskGetTickCount(), pxThisTask->pcName, pxThisTask->xAbsoluteDeadline, pxThisTask->xLastWakeTime, uxTaskPriorityGet( NULL ), *pxThisTask->pxTaskHandle );
		//taskEXIT_CRITICAL();

		childTask->taskFunc(parameters);
		childTask->taskState = BLOCKED_STATE;

		//taskENTER_CRITICAL();
		//printf( "execution time %d Task %s\r\n", pxThisTask->xExecTime, pxThisTask->pcName );
		//taskEXIT_CRITICAL();

		childTask->executionTime = 0;

		#if(SCHEDULING_ALGORITHM == EDF_SCHEDULING || SCHEDULING_ALGORITHM == MUF_SCHEDULING)
            // handle least laxity for the dynamic part of MUF_SCHEDULING
			childTask->hardDeadline = childTask->previousWakeTime + childTask->period + childTask->softDeadline;
            RunShedulerTask();
		#endif
		vTaskDelayUntil( &childTask->previousWakeTime, childTask->period );
	}
}
//===============================================================================================
void RunSchedulerTask(void)
{
	BaseType_t xHigherPriorityTaskWoken;
	//vTaskNotifyGiveFromISR( xSchedulerHandle, &xHigherPriorityTaskWoken );
	schedYIELD_FROM_ISR( xHigherPriorityTaskWoken );
}
//===============================================================================================
void SortTaskControlBlocks(void)
{
    // for rm sort according to the period statically only once
    // for edf sort according to the deadlines dynamically all the time
    // for muf sort according to the period statically only once for the critical set
}
//===============================================================================================
void RegisterSchedulerTasks(void)
{
    for(BaseType_t i = 0; i < gTaskCounter; i++ )
    {
        configASSERT(gTaskControlBlocks[i].bEmptyBlock == pdFALSE);
        SchedulerTaskControlBlock_t *tmpTask = &gTaskControlBlocks[i];
        configASSERT(xTaskCreate(SchedulerParentTaskFunc, tmpTask->taskName,
                     tmpTask->stackSize, tmpTask->parameters,
                     tmpTask->priority, tmpTask->taskHandle) == pdPASS);
        vTaskSetThreadLocalStoragePointer( *tmpTask->taskHandle,
                                           LOCAL_STORAGE_INDEX, tmpTask);
    }
}
//===============================================================================================
BaseType_t SetNewTaskControlBlock(SchedulerTaskControlBlock_t **newTask)
{
    for(BaseType_t i = 0; i < MAX_NUM_TASKS; i++)
    {
        if(gTaskControlBlocks[i].bEmptyBlock == pdTRUE)
        {
            ++gTaskCounter;
            *newTask = &gTaskControlBlocks[i];
            return pdTRUE;
        }
    }
    return pdFALSE;
}
//===============================================================================================
BaseType_t RemoveTaskControlBlock(TaskHandle_t taskHandle)
{
    for(BaseType_t i = 0; i < MAX_NUM_TASKS; i++ )
    {
        if(gTaskControlBlocks[i].bEmptyBlock == pdFALSE && 
           *gTaskControlBlocks[i].taskHandle == taskHandle )
        {
            gTaskControlBlocks[i].bEmptyBlock = pdTRUE;
            --gTaskCounter;
            return pdTRUE;
        }
    }
    return pdFALSE;
}
//===============================================================================================
void SchedulerTaskCreate( TaskFunction_t taskFunc, const char *taskName, UBaseType_t stackSize,
                          void *parameters, UBaseType_t priority, TaskHandle_t *taskHandle,
                          TickType_t readyTime, TickType_t period, TickType_t deadline )
{
    SchedulerTaskControlBlock_t *newTask;
    taskENTER_CRITICAL();
    configASSERT(SetNewTaskControlBlock(&newTask) == pdTRUE);
    *newTask = (SchedulerTaskControlBlock_t) 
                {   .taskFunc = taskFunc, .taskName = taskName, .stackSize = stackSize,
                    .parameters = parameters, .priority = priority, .taskHandle = taskHandle,
                    .readyTime = readyTime, .period = period, .softDeadline = deadline,
                    .hardDeadline = deadline + readyTime + gSystemStartTime,
                    .taskState = BLOCKED_STATE, .bEmptyBlock = pdFALSE};
	taskEXIT_CRITICAL();
}
//===============================================================================================
void SchedulerTaskDelete( TaskHandle_t taskHandle )
{
    configASSERT(taskHandle != NULL);
    configASSERT(RemoveTaskControlBlock(taskHandle) == pdTRUE);
}
//===============================================================================================
void SchedulerInit(void)
{
    for(UBaseType_t i = 0; i < MAX_NUM_TASKS; i++)
    {
        gTaskControlBlocks[i].bEmptyBlock = pdTRUE;
    }
}
//===============================================================================================
void SchedulerStart(void)
{

    #if(SCHEDULING_ALGORITHM == RM_SCHEDULING)
        // set fixed priorities
    #elif(SCHEDULING_ALGORITHM == EDF_SCHEDULING)
        // initialize edf
    #elif(SCHEDULING_ALGORITHM == MUF_SCHEDULING)
        // set fixed priorities and init muf
    #else
        configASSERT(pdTRUE == pdFALSE)
    #endif

    RegisterSchedulerTasks();

    gSystemStartTime = xTaskGetTickCount();
	vTaskStartScheduler();
}
//===============================================================================================