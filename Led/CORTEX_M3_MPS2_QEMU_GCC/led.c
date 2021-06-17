#include "led.h"

LED_t ledInit( const char *name,
                     uint32_t taskPriority,
                     QueueHandle_t queue,
                     uint32_t stackSize,
                     void *parameters )
{

}

void ledTaskCreate(LED_t *led)
{
    xTaskCreate( ledReceiveToggle,            /* The function that implements the task. */
                 led->ledName,                            /* The text name assigned to the task - for debug only as it is not used by the kernel. */
                 led->ledStackSize,        /* The size of the stack to allocate to the task. */
                 led->ledParameters,                            /* The parameter passed to the task - not used in this case. */
                 led->ledTaskPriority, /* The priority assigned to the task. */
                 NULL );

    xTaskCreate( ledPrint,            /* The function that implements the task. */
                 NULL,                            /* The text name assigned to the task - for debug only as it is not used by the kernel. */
                 led->ledStackSize,        /* The size of the stack to allocate to the task. */
                 led->ledParameters,                            /* The parameter passed to the task - not used in this case. */
                 tskIDLE_PRIORITY + 100, /* The priority assigned to the task. */
                 NULL );
}

void ledReceiveToggle( void *params )
{
    LED_t *led = (LED_t*) params;
    void *dummyBuffer;

    for( ;; )
    {
        xQueueReceive( led->ledQueue, dummyBuffer, portMAX_DELAY );
        led->ledState = LED_ON;
        vTaskDelay(1000);
        led->ledState = LED_OFF;
    }
}

void ledPrint( void *params )
{
    LED_t *led = (LED_t*) params;
    for ( ;; )
    {
        if (led->ledState == LED_ON)
        {
            printf("%s is ON!\n", led->ledName);
        }
        else
        {
            printf("%s is OFF\n", led->ledName);
        }
        vTaskDelay(500);
    }
}