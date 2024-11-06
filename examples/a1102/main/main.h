#ifndef MAIN_H
#define MAIN_H

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "driver/uart.h"
#include "driver/i2c.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"

// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
#include "esp_timer.h"
// #include "esp_log.h"
#include "esp_check.h"

// #include <net/if.h>
// #include "mqtt_client.h"
#include "esp_spiffs.h"
#include "esp_system.h"
// #include "nvs_flash.h"
// #include "esp_event.h"
// #include "esp_wifi.h"
// #include "esp_netif.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include "mbcontroller.h"

// #include "sscma_client_io.h"
// #include "sscma_client_ops.h"


// #include "sscma_client_commands.h"

#include "app_wifi.h"
#include "app_mqtt.h"
#include "app_nvs.h"
#include "app_modbus.h"

#include "app_himax.h"
#include "app_ota.h"

#define TAG "SenseCAP A1102"




#endif // !MAIN_H