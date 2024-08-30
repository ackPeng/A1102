#include <dirent.h>
#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"

#include "sensecap-a1102.h"

static const char *TAG = "main";

void app_main(void)
{

    while (1)
    {
        ESP_LOGI(TAG, "Hello World!\n");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}