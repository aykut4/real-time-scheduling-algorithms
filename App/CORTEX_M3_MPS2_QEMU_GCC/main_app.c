#include "scheduler.h"

static void prvQueueReceiveTask( void *pvParameters );
static void prvQueueSendTask( void *pvParameters );



#define mainQUEUE_RECEIVE_TASK_PRIORITY     ( tskIDLE_PRIORITY + 2 )
#define mainQUEUE_SEND_TASK_PRIORITY        ( tskIDLE_PRIORITY + 1 )
#define mainQUEUE_LENGTH                    ( 10 )
#define mainQUEUE_SEND_FREQUENCY_MS         ( 1000 / portTICK_PERIOD_MS )

void main_app( void )
{

    xTaskCreate( prvQueueSendTask,
                "TX",
                configMINIMAL_STACK_SIZE,
                NULL,
                mainQUEUE_SEND_TASK_PRIORITY,
                NULL );
        
    xTaskCreate( prvQueueReceiveTask,
                "RX",
                configMINIMAL_STACK_SIZE,
                NULL,
                mainQUEUE_SEND_TASK_PRIORITY,
                NULL );


    vTaskStartScheduler();
}

static void prvQueueSendTask( void *pvParameters )
{

    ( void ) pvParameters;

    TickType_t xNextWakeTime;
    xNextWakeTime = xTaskGetTickCount();

    for( ;; )
    {
        /* Place this task in the blocked state until it is time to run again. */
        vTaskDelayUntil( &xNextWakeTime, mainQUEUE_SEND_FREQUENCY_MS );
        printf("%s\n","blinking1");
    }
}


static void prvQueueReceiveTask( void *pvParameters )
{
    ( void ) pvParameters;
    
    for( ;; )
    {
        printf("%s\n","blinking2");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
/*-----------------------------------------------------------*/
