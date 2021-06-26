#include "scheduler.h"
//===============================================================================================
typedef struct SchedulerTaskControlBlock
{
	TaskFunction_t taskFunc;
	const char *taskName;
	UBaseType_t stackSize;
	void *parameters;
	UBaseType_t priority;
    UBaseType_t userPriority;
	TaskHandle_t *taskHandle;
	TickType_t readyTime;
	TickType_t period;
	TickType_t softDeadline;
    TickType_t hardDeadline;
	BaseType_t taskState;       // RUNNING, BLOCKED, SUSPENDED
	BaseType_t bEmptyBlock; 	// empty == pdTRUE, not empty == pdFALSE
    BaseType_t bCritical;       // yes == pdTRUE, no == pdFALSE
    TickType_t previousWakeTime;
    TickType_t executionTime;

} SchedulerTaskControlBlock_t;
//===============================================================================================
static SchedulerTaskControlBlock_t gTaskControlBlocks[MAX_NUM_TASKS] = {0};
static BaseType_t gTaskCounter = 0;
static TickType_t gSystemStartTime = 0;
//===============================================================================================
void PrintTaskControlBlocks()
{
    for (BaseType_t i = 0; i < gTaskCounter; i++)
    {
        printf("Task #%d: taskName: %s, period: %d, deadline: %d, priority: %d, task handle: %d, exec_time: %d\n", i, gTaskControlBlocks[i].taskName, gTaskControlBlocks[i].period, gTaskControlBlocks[i].hardDeadline, gTaskControlBlocks[i].priority, gTaskControlBlocks[i].taskHandle, gTaskControlBlocks[i].executionTime);
    }
}
//===============================================================================================
void SetPriorities(void)
{
    for(BaseType_t i = 0; i < gTaskCounter; i++ )
        {
            vTaskPrioritySet(*gTaskControlBlocks[i].taskHandle, gTaskControlBlocks[i].priority);
        }
}
//===============================================================================================
void SetInitialPriorities(void)
{
    #if(SCHEDULING_ALGORITHM == RM_SCHEDULING)
        BaseType_t maxPriority = configMAX_PRIORITIES;
        BaseType_t indexes[MAX_NUM_TASKS] = {0};
        for (BaseType_t i = 0; i < gTaskCounter; i++)
        {
            TickType_t minPeriod = UINT32_MAX;
            BaseType_t index = 0;
            for (BaseType_t j = 0; j < gTaskCounter; j++)
            {
                if (gTaskControlBlocks[j].period < minPeriod && indexes[j] == 0)
                {
                    minPeriod = gTaskControlBlocks[j].period;
                    index = j;
                }
            }
            if (indexes[index] == 0){
                gTaskControlBlocks[index].priority = --maxPriority;
                indexes[index] = 1;
            }
        }
    #elif(SCHEDULING_ALGORITHM == EDF_SCHEDULING)
        BaseType_t maxPriority = configMAX_PRIORITIES;
        BaseType_t indexes[MAX_NUM_TASKS] = {0};
        for (BaseType_t i = 0; i < gTaskCounter; i++)
        {
            TickType_t minDeadline = UINT32_MAX;
            BaseType_t index = 0;
            for (BaseType_t j = 0; j < gTaskCounter; j++)
            {
                if (gTaskControlBlocks[j].hardDeadline < minDeadline && indexes[j] == 0)
                {
                    minDeadline = gTaskControlBlocks[j].hardDeadline;
                    index = j;
                }
            }
            if (indexes[index] == 0){
                gTaskControlBlocks[index].priority = --maxPriority;
                indexes[index] = 1;
            }
        }
    #elif(SCHEDULING_ALGORITHM == MUF_SCHEDULING)
        BaseType_t utilization = 0;
        BaseType_t indexes[MAX_NUM_TASKS] = {0};
        for (BaseType_t i = 0; i < gTaskCounter; i++)
        {
            TickType_t minPeriod = UINT32_MAX;
            BaseType_t index = 0;
            for (BaseType_t j = 0; j < gTaskCounter; j++)
            {
                if (gTaskControlBlocks[j].period < minPeriod && indexes[j] == 0)
                {
                    minPeriod = gTaskControlBlocks[j].period;
                    index = j;
                }
            }
            if (indexes[index] == 0){
                BaseType_t utilTmp = gTaskControlBlocks[index].executionTime * 100 / gTaskControlBlocks[index].period;
                utilization += utilTmp;
                if (utilization < 100)
                {
                    gTaskControlBlocks[index].bCritical = pdTRUE;
                }
                else
                {
                    gTaskControlBlocks[index].bCritical = pdFALSE;
                }
                indexes[index] = 1;
            }
        }


        TickType_t currentTime = xTaskGetTickCount();
        BaseType_t maxPriority = configMAX_PRIORITIES;
        BaseType_t indexes2[MAX_NUM_TASKS] = {0};
        for (BaseType_t i = 0; i < gTaskCounter; i++)
        {
            TickType_t minLaxity = UINT32_MAX;
            BaseType_t tmpCrit = pdFALSE;
            BaseType_t tmpUserPriority = 0;
            BaseType_t index = 0;
            for (BaseType_t j = 0; j < gTaskCounter; j++)
            {
                if (gTaskControlBlocks[j].bCritical == pdTRUE && tmpCrit == pdFALSE && indexes2[j] == 0)
                {
                    // already know we will miss the deadline
                    if ((gTaskControlBlocks[j].executionTime + currentTime) >= gTaskControlBlocks[j].hardDeadline) continue;
                    // highest priority so far
                    tmpCrit = gTaskControlBlocks[j].bCritical;
                    tmpUserPriority = gTaskControlBlocks[j].userPriority;
                    minLaxity = gTaskControlBlocks[j].hardDeadline - (gTaskControlBlocks[j].executionTime + currentTime);
                    index = j;
                }
                else if (gTaskControlBlocks[j].bCritical == pdFALSE && tmpCrit == pdTRUE)
                {
                    continue;
                }
                else if (((gTaskControlBlocks[j].bCritical == pdTRUE && tmpCrit == pdTRUE) || (gTaskControlBlocks[j].bCritical == pdFALSE && tmpCrit == pdFALSE)) && indexes2[j] == 0 )
                {
                    // see the laxities
                    // already know we will miss the deadline
                    if ((gTaskControlBlocks[j].executionTime + currentTime) >= gTaskControlBlocks[j].hardDeadline) continue;
                    if (gTaskControlBlocks[j].hardDeadline - (gTaskControlBlocks[j].executionTime + currentTime) < minLaxity)
                    {
                        tmpUserPriority = gTaskControlBlocks[j].userPriority;
                        minLaxity = gTaskControlBlocks[j].hardDeadline - (gTaskControlBlocks[j].executionTime + currentTime);
                        index = j;
                    }
                    else if (gTaskControlBlocks[j].hardDeadline - (gTaskControlBlocks[j].executionTime + currentTime) == minLaxity)
                    {
                        // see the user priorities
                        if (gTaskControlBlocks[j].userPriority > tmpUserPriority)
                        {
                            tmpUserPriority = gTaskControlBlocks[j].userPriority;
                            index = j;
                        }
                    }
                    else continue;
                }
                else {}
            }
            if (indexes2[index] == 0){
                gTaskControlBlocks[index].priority = --maxPriority;
                indexes2[index] = 1;
            }
        }
    #else
        configASSERT(pdTRUE == pdFALSE);
    #endif
}
//===============================================================================================
void UpdatePriorities(void)
{
    #if(SCHEDULING_ALGORITHM == EDF_SCHEDULING)
        BaseType_t maxPriority = configMAX_PRIORITIES;
        BaseType_t indexes[MAX_NUM_TASKS] = {0};
        for (BaseType_t i = 0; i < gTaskCounter; i++)
        {
            TickType_t minDeadline = UINT32_MAX;
            BaseType_t index = 0;
            for (BaseType_t j = 0; j < gTaskCounter; j++)
            {
                if (gTaskControlBlocks[j].hardDeadline < minDeadline && indexes[j] == 0)
                {
                    minDeadline = gTaskControlBlocks[j].hardDeadline;
                    index = j;
                }
            }
            if (indexes[index] == 0){
                gTaskControlBlocks[index].priority = --maxPriority;
                indexes[index] = 1;
            }
        }
    #elif(SCHEDULING_ALGORITHM == MUF_SCHEDULING)
         TickType_t currentTime = xTaskGetTickCount();
        BaseType_t maxPriority = configMAX_PRIORITIES;
        BaseType_t indexes2[MAX_NUM_TASKS] = {0};
        for (BaseType_t i = 0; i < gTaskCounter; i++)
        {
            TickType_t minLaxity = UINT32_MAX;
            BaseType_t tmpCrit = pdFALSE;
            BaseType_t tmpUserPriority = 0;
            BaseType_t index = 0;
            for (BaseType_t j = 0; j < gTaskCounter; j++)
            {
                if (gTaskControlBlocks[j].bCritical == pdTRUE && tmpCrit == pdFALSE && indexes2[j] == 0)
                {
                    // already know we will miss the deadline
                    if ((gTaskControlBlocks[j].executionTime + currentTime) >= gTaskControlBlocks[j].hardDeadline) continue;
                    // highest priority so far
                    tmpCrit = gTaskControlBlocks[j].bCritical;
                    tmpUserPriority = gTaskControlBlocks[j].userPriority;
                    minLaxity = gTaskControlBlocks[j].hardDeadline - (gTaskControlBlocks[j].executionTime + currentTime);
                    index = j;
                }
                else if (gTaskControlBlocks[j].bCritical == pdFALSE && tmpCrit == pdTRUE)
                {
                    continue;
                }
                else if (((gTaskControlBlocks[j].bCritical == pdTRUE && tmpCrit == pdTRUE) || (gTaskControlBlocks[j].bCritical == pdFALSE && tmpCrit == pdFALSE)) && indexes2[j] == 0 )
                {
                    // see the laxities
                    // already know we will miss the deadline
                    if ((gTaskControlBlocks[j].executionTime + currentTime) >= gTaskControlBlocks[j].hardDeadline) continue;
                    if (gTaskControlBlocks[j].hardDeadline - (gTaskControlBlocks[j].executionTime + currentTime) < minLaxity)
                    {
                        tmpUserPriority = gTaskControlBlocks[j].userPriority;
                        minLaxity = gTaskControlBlocks[j].hardDeadline - (gTaskControlBlocks[j].executionTime + currentTime);
                        index = j;
                    }
                    else if (gTaskControlBlocks[j].hardDeadline - (gTaskControlBlocks[j].executionTime + currentTime) == minLaxity)
                    {
                        // see the user priorities
                        if (gTaskControlBlocks[j].userPriority > tmpUserPriority)
                        {
                            tmpUserPriority = gTaskControlBlocks[j].userPriority;
                            index = j;
                        }
                    }
                    else continue;
                }
                else {}
            }
            if (indexes2[index] == 0){
                gTaskControlBlocks[index].priority = --maxPriority;
                indexes2[index] = 1;
            }
        }
    #else
        configASSERT(pdTRUE == pdFALSE);
    #endif
}
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
        childTask->hardDeadline = childTask->previousWakeTime + childTask->softDeadline;
        #if(SCHEDULING_ALGORITHM == EDF_SCHEDULING || SCHEDULING_ALGORITHM == MUF_SCHEDULING)
            taskENTER_CRITICAL();
            UpdatePriorities();
            SetPriorities();
            taskEXIT_CRITICAL();
		#endif

        printf("\n%s (SysTime:[%d]): Period:[%d], Deadline:[%d], AbsDeadline:[%d], Priority:[%d], ExecTime:[%d], PrevWakeTime:[%d]\n", childTask->taskName, xTaskGetTickCount() /*/ 1000*/, childTask->period /*/ 1000*/, childTask->softDeadline /*/ 1000*/, childTask->hardDeadline /*/ 1000*/, childTask->priority, childTask->executionTime /*/ 1000*/, childTask->previousWakeTime /*/ 1000*/);
        printf("%s (Job Description): ",childTask->taskName);

        childTask->taskState = RUNNING_STATE;
        TickType_t runInfo[2] = {0, childTask->executionTime};
        childTask->taskFunc((void*)runInfo);

		printf("%s (SysTime: [%d]): COMPLETED!\n\n", childTask->taskName, xTaskGetTickCount() /*/ 1000*/);

        childTask->taskState = BLOCKED_STATE;
        childTask->hardDeadline += childTask->softDeadline;
		#if(SCHEDULING_ALGORITHM == EDF_SCHEDULING || SCHEDULING_ALGORITHM == MUF_SCHEDULING)
            taskENTER_CRITICAL();
            UpdatePriorities();
            SetPriorities();
            taskEXIT_CRITICAL();
		#endif
		vTaskDelayUntil( &childTask->previousWakeTime, childTask->period );
	}
}
//===============================================================================================
static void RegisterSchedulerTasks(void)
{
    SchedulerTaskControlBlock_t *tmpTask;
    for(BaseType_t i = 0; i < gTaskCounter; i++ )
    {
        configASSERT(gTaskControlBlocks[i].bEmptyBlock == pdFALSE);
        tmpTask = &gTaskControlBlocks[i];
        configASSERT(xTaskCreate(SchedulerParentTaskFunc, tmpTask->taskName,
                     tmpTask->stackSize, (void*)tmpTask->taskName,
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
void SchedulerTaskCreate( TaskFunction_t taskFunc, const char *taskName, UBaseType_t stackSize,
                          void *parameters, UBaseType_t priority, TaskHandle_t *taskHandle,
                          TickType_t readyTime, TickType_t period, TickType_t executionTime )
{
    SchedulerTaskControlBlock_t *newTask;
    taskENTER_CRITICAL();
    configASSERT(SetNewTaskControlBlock(&newTask) == pdTRUE);
    *newTask = (SchedulerTaskControlBlock_t) 
                {   .taskFunc = taskFunc, .taskName = taskName, .stackSize = stackSize,
                    .parameters = parameters, .priority = priority, .userPriority = priority, .taskHandle = taskHandle,
                    .readyTime = readyTime, .period = period, .softDeadline = period,
                    .hardDeadline = period + readyTime + gSystemStartTime,
                    .taskState = BLOCKED_STATE, .bEmptyBlock = pdFALSE,
                    .executionTime = executionTime, .bCritical = pdFALSE};
	taskEXIT_CRITICAL();
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
    SetInitialPriorities();

    RegisterSchedulerTasks();

    gSystemStartTime = xTaskGetTickCount();
    PrintTaskControlBlocks();
	vTaskStartScheduler();
}
//===============================================================================================