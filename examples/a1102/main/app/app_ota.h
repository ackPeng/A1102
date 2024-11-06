

#ifndef _NEW_OTA_H
#define _NEW_OTA_H

#include "freertos/FreeRTOS.h"
#include "freertos/ringbuf.h"
#include "esp_check.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_event.h"
#include "esp_timer.h"
#include "esp_ota_ops.h"
#include "esp_http_client.h"
#include "esp_crt_bundle.h"
#include "esp_https_ota.h"
#include "cJSON.h"
#include "cJSON_Utils.h"
// #include "util.h"
#include "esp_wifi.h"
#include "esp_spiffs.h"

// #include "event_loops.h"
#include "sscma_client_types.h"
#include "sscma_client_ops.h"
#include "sscma_client_io.h"
#include "sscma_client_flasher.h"

#include <dirent.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <inttypes.h>
#include <stdatomic.h>

#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"


#include "nvs.h"
#include "nvs_flash.h"


#include "driver/uart.h"
#include "driver/gpio.h"

#include "esp_partition.h"
#include "driver/spi_master.h"
#include "esp_spiffs.h"

#include "bsp.h"

// extern sscma_client_handle_t sscam_ota_client ;
// extern sscma_client_io_handle_t _sscma_flasher_io ;
// extern sscma_client_flasher_handle_t sscam_ota_flasher ;


// Declare event base only
ESP_EVENT_DECLARE_BASE(OTA_EVENT_BASE);





#if CONFIG_BOOTLOADER_APP_ANTI_ROLLBACK
#include "esp_efuse.h"
#endif



/** Major version number (X.x.x) */
#define SSCMA_CLIENT_OTA_VERSION_MAJOR 0
/** Minor version number (x.X.x) */
#define SSCMA_CLIENT_OTA_VERSION_MINOR 0
/** Patch version number (x.x.X) */
#define SSCMA_CLIENT_OTA_VERSION_PATCH 1

/**
 * Macro to convert version number into an integer
 *
 * To be used in comparisons, such as SSCMA_CLIENT_OTA_VERSION >= SSCMA_CLIENT_OTA_VERSION_VAL(4, 0, 0)
 */
#define SSCMA_CLIENT_OTA_VERSION_VAL(major, minor, patch) ((major << 16) | (minor << 8) | (patch))

/**
 * Current version, as an integer
 *
 * To be used in comparisons, such as SSCMA_CLIENT_OTA_VERSION >= SSCMA_CLIENT_OTA_VERSION_VAL(4, 0, 0)
 */
#define SSCMA_CLIENT_OTA_VERSION SSCMA_CLIENT_OTA_VERSION_VAL(SSCMA_CLIENT_OTA_VERSION_MAJOR, SSCMA_CLIENT_OTA_VERSION_MINOR, SSCMA_CLIENT_OTA_VERSION_PATCH)

#define ESP_ERR_OTA_CONNECTION_FAIL         0x202
#define ESP_ERR_OTA_DOWNLOAD_FAIL           0x204
#define ESP_ERR_OTA_SSCMA_START_FAIL        0x205
#define ESP_ERR_OTA_SSCMA_WRITE_FAIL        0x206
#define ESP_ERR_OTA_SSCMA_INTERNAL_ERR      0x207




extern bool ota_flag ;
extern bool esp32_ota ;
extern bool himax_ota ;

extern char ota_url[256]; 
//OTA event type
typedef enum {
    OTA_ESP32 = 0,
    OTA_HIMAX,
    OTA_AI_MODLE,
    OTA_UNKNOWN,
}ota_event_type_t;

typedef struct {
    ota_event_type_t event_type;
    char url[256];  
} ota_event_data_t;



// used to pass userdata to http client event handler
typedef struct {
    int ota_type;
    int content_len;
    esp_err_t err;
    esp_http_client_handle_t http_client;
} ota_sscma_writer_userdata_t;



void ota_init();

void bsp_sscma_flasher_init_legacy(void);

#endif


