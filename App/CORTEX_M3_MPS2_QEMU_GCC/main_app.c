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
    SchedulerTaskCreate(dispatcherTask, "Dispatcher #1", configMINIMAL_STACK_SIZE, NULL, 1, &xHandle1, pdMS_TO_TICKS(0), pdMS_TO_TICKS(5000), executionTime1);
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
