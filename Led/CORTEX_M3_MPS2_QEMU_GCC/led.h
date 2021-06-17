#ifndef LED_H
#define LED_H

#ifdef __cplusplus
    extern "C" {
#endif

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

LED_t ledInit(  const char *name,
                uint32_t taskPriority,
                QueueHandle_t queue,
                uint32_t stackSize,
                void *parameters);

void ledTaskCreate(LED_t *led);
void ledReceiveToggle( void *params );
void ledPrint(void *params);

#endif /* LED_H */