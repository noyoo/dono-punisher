#include <stdio.h>
#include <App.h>
#include "freertos/freeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

void app_main(void)
{
    for (;;)
    {
        stateMachine();
        vTaskDelay(1);
    }
    
}
