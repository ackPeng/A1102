#include "app_himax.h"
#include "app_mqtt.h"
#include "app_nvs.h"
#include "app_wifi.h"

static const char *TAG = "main";

void print_memory_status()
{
    size_t free_mem = heap_caps_get_free_size(MALLOC_CAP_8BIT);       // 当前可用的堆内存
    size_t min_free_mem = heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT); // 启动后最小堆内存
    size_t largest_block = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT); // 最大连续空闲块
    UBaseType_t spaces_available = uxQueueSpacesAvailable(queue);

    ESP_LOGI(TAG, "Free memory: %d bytes", free_mem);
    ESP_LOGI(TAG, "Minimum free memory since startup: %d bytes", min_free_mem);
    ESP_LOGI(TAG, "Largest free block: %d bytes", largest_block);
    ESP_LOGI(TAG, "Available spaces in queue: %d", spaces_available);
}

void app_main(void)
{
    app_nvs_init();
    queue = xQueueCreate(5, sizeof(char *));

    if (queue == NULL) {
        printf("Failed to create queue\n");
    } else {
        printf("Queue created successfully\n");
    }

    himax_sscam_init();

    while (himax_config != true)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    
    ESP_LOGI(TAG, "Init network");
    wifi_init_sta();
    mqtt_initialize();

    while (1)
    {
        print_memory_status();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}