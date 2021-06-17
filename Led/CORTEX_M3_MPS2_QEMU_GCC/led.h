#ifndef LED_H
#define LED_H

#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include <stdio.h>

#define LED_ON 1
#define LED_OFF 0

struct LED {

    void *ledParameters;
    const char *ledName;

    QueueHandle_t ledQueue;

    uint32_t ledStackSize;
    uint32_t ledState;
    uint32_t ledTaskPriority;

};
typedef struct LED LED_t;

LED_t* ledInit(  const char *name,
                        QueueHandle_t queue,
                        uint32_t taskPriority,
                        uint32_t stackSize );

void ledTaskCreate(LED_t *led);
void ledPrint( void *params );
void ledReceiveToggle( void *params );

#endif /* LED_H */