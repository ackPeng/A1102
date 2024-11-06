#include "main.h"





ESP_EVENT_DEFINE_BASE(OTA_EVENT_BASE);

void app_main(void)
{
    app_nvs_init();
    wifi_init_sta();
    mqtt_initialize();

    himax_sscam_init();

    app_modbus_init();

    ota_init();
    while (1)
    {
        // ESP_LOGI(TAG, "wait !!");
        vTaskDelay(pdMS_TO_TICKS(10000));
        /* code */
    }
    
   
}
