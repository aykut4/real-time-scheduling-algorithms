#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include <semphr.h>
#include <timers.h>
#include <list.h>
#include "scheduler.h"

static void prvQueueReceiveTask( void *pvParameters );
static void prvQueueSendTask( void *pvParameters );



#define mainQUEUE_RECEIVE_TASK_PRIORITY     ( tskIDLE_PRIORITY + 2 )
#define mainQUEUE_SEND_TASK_PRIORITY        ( tskIDLE_PRIORITY + 1 )
#define mainQUEUE_LENGTH                    ( 10 )
#define mainQUEUE_SEND_FREQUENCY_MS         ( 1000 / portTICK_PERIOD_MS )


TaskHandle_t xHandle1 = NULL;
TaskHandle_t xHandle2 = NULL;
TaskHandle_t xHandle3 = NULL;
TaskHandle_t xHandle4 = NULL;


static void testFunc1( void *pvParameters ){
char* c = pvParameters;
	printf("Test1 %c\r\n", *c);

int i, a;
	for( i = 0; i < 1000000; i++ )
	{
		a = 1 + i*i*i*i;
	}
}

static void testFunc2( void *pvParameters )
{
char* c = pvParameters;
int i;
	for( i=0; i < 1000000; i++ )
	{
	int a = i * i * i * i;
	}
	printf("TestA2 %c %d\r\n", *c, xTaskGetTickCount());
}

static void testFunc3( void *pvParameters ){
	char* c = pvParameters;
	printf("Test3 %c\r\n", *c);
	int i, a;
	for(i = 0; i < 1000000; i++ )
	{
		a = 1 + a * a * i;
	}
}

static void testFunc4(void *pvParameters)
{
	char* c = pvParameters;
	int i, a;
	for(i = 0; i < 2000000; i++)
	{
		a = 1 + i * i * i * i;
	}
	printf("Test4 %c\r\n", *c);
}

void main_app( void )
{

    char c1 = 'a';
	char c2 = 'b';
	char c3 = 'c';
	char c4 = 'e';


	vSchedulerInit();

	vSchedulerPeriodicTaskCreate(testFunc1, "t1", configMINIMAL_STACK_SIZE, &c1, 1, &xHandle1, pdMS_TO_TICKS(0), pdMS_TO_TICKS(200), pdMS_TO_TICKS(100), pdMS_TO_TICKS(500));
	vSchedulerPeriodicTaskCreate(testFunc2, "t2", configMINIMAL_STACK_SIZE, &c2, 2, &xHandle2, pdMS_TO_TICKS(50), pdMS_TO_TICKS(100), pdMS_TO_TICKS(100), pdMS_TO_TICKS(100));
	vSchedulerPeriodicTaskCreate(testFunc3, "t3", configMINIMAL_STACK_SIZE, &c3, 3, &xHandle3, pdMS_TO_TICKS(0), pdMS_TO_TICKS(300), pdMS_TO_TICKS(100), pdMS_TO_TICKS(200));
	vSchedulerPeriodicTaskCreate(testFunc4, "t4", configMINIMAL_STACK_SIZE, &c4, 4, &xHandle4, pdMS_TO_TICKS(0), pdMS_TO_TICKS(400), pdMS_TO_TICKS(100), pdMS_TO_TICKS(100));

    vSchedulerStart();

    /*xTaskCreate( prvQueueSendTask,
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
    vTaskStartScheduler();*/

}

static void prvQueueSendTask( void *pvParameters )
{

    ( void ) pvParameters;

    TickType_t xNextWakeTime;
    xNextWakeTime = xTaskGetTickCount();

    for( ;; )
    {
        vTaskDelayUntil( &xNextWakeTime, mainQUEUE_SEND_FREQUENCY_MS );
        printf("%s\n","blinking1");
    }
}


static void prvQueueReceiveTask( void *pvParameters )
{
    ( void ) pvParameters;

    TickType_t xNextWakeTime;
    xNextWakeTime = xTaskGetTickCount();

    for( ;; )
    {
        printf("%s\n","blinking2");
        vTaskDelayUntil( &xNextWakeTime, mainQUEUE_SEND_FREQUENCY_MS );
    }
}
