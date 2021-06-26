#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include <semphr.h>
#include <timers.h>
#include <list.h>
#include "scheduler.h"

void vApplicationTickHook( void );
void vApplicationIdleHook( void );

TaskHandle_t xHandle1 = NULL;
TaskHandle_t xHandle2 = NULL;
TaskHandle_t xHandle3 = NULL;
TaskHandle_t xHandle4 = NULL;

static void dispatcherTask(void *parameters)
{
    TickType_t *executionTime = (TickType_t*)parameters;
    TickType_t startTime = xTaskGetTickCount();
    printf("dispatching item...\n");
    for( ;; )
    {
        //printf("anaisik\n");
        executionTime[0] = xTaskGetTickCount() - startTime;
        if (executionTime[0] >= executionTime[1]) break;
    }
}

void main_app( void )
{

    SchedulerInit();
    
    static TickType_t executionTime1 = pdMS_TO_TICKS(1000);
    static TickType_t executionTime2 = pdMS_TO_TICKS(500);
    static TickType_t executionTime3 = pdMS_TO_TICKS(1000);
    static TickType_t executionTime4 = pdMS_TO_TICKS(800);
    SchedulerTaskCreate(dispatcherTask, "Dispatcher #1", configMINIMAL_STACK_SIZE, NULL, 1, &xHandle1, pdMS_TO_TICKS(0), pdMS_TO_TICKS(5000), executionTime1);
	SchedulerTaskCreate(dispatcherTask, "Dispatcher #2", configMINIMAL_STACK_SIZE, NULL, 2, &xHandle2, pdMS_TO_TICKS(0), pdMS_TO_TICKS(3000), executionTime2);
    SchedulerTaskCreate(dispatcherTask, "Dispatcher #3", configMINIMAL_STACK_SIZE, NULL, 1, &xHandle3, pdMS_TO_TICKS(0), pdMS_TO_TICKS(3000), executionTime3);
	SchedulerTaskCreate(dispatcherTask, "Dispatcher #4", configMINIMAL_STACK_SIZE, NULL, 2, &xHandle4, pdMS_TO_TICKS(0), pdMS_TO_TICKS(4000), executionTime4);
    

    /*static TickType_t executionTime1 = pdMS_TO_TICKS(700);
    static TickType_t executionTime2 = pdMS_TO_TICKS(1000);
    static TickType_t executionTime3 = pdMS_TO_TICKS(1000);
    static TickType_t executionTime4 = pdMS_TO_TICKS(500);
    SchedulerTaskCreate(dispatcherTask1, "Dispatcher #1", configMINIMAL_STACK_SIZE + 5000, &executionTime1, 1, &xHandle1, pdMS_TO_TICKS(0), pdMS_TO_TICKS(7000), pdMS_TO_TICKS(7000));
	SchedulerTaskCreate(dispatcherTask1, "Dispatcher #2", configMINIMAL_STACK_SIZE + 5000, &executionTime2, 2, &xHandle2, pdMS_TO_TICKS(0), pdMS_TO_TICKS(10000), pdMS_TO_TICKS(10000));
    SchedulerTaskCreate(dispatcherTask1, "Dispatcher #3", configMINIMAL_STACK_SIZE + 5000, &executionTime3, 1, &xHandle3, pdMS_TO_TICKS(0), pdMS_TO_TICKS(2500), pdMS_TO_TICKS(2500));
	SchedulerTaskCreate(dispatcherTask1, "Dispatcher #4", configMINIMAL_STACK_SIZE + 5000, &executionTime4, 2, &xHandle4, pdMS_TO_TICKS(0), pdMS_TO_TICKS(5000), pdMS_TO_TICKS(5000));
    */
    SchedulerStart();
}

void vApplicationTickHook( void )
{

}

void vApplicationIdleHook( void )
{
    volatile size_t xFreeHeapSpace;

    /* This is just a trivial example of an idle hook.  It is called on each
    cycle of the idle task.  It must *NOT* attempt to block.  In this case the
    idle task just queries the amount of FreeRTOS heap that remains.  See the
    memory management section on the https://www.FreeRTOS.org web site for memory
    management options.  If there is a lot of heap memory free then the
    configTOTAL_HEAP_SIZE value in FreeRTOSConfig.h can be reduced to free up
    RAM. */
}
