#include "app_ota.h"




static const char *TAG = "app_ota";
// extern const uint8_t server_cert_pem_start[] asm("_binary_ca_cert_pem_start");
// extern const uint8_t server_cert_pem_end[] asm("_binary_ca_cert_pem_end");
const uint8_t server_cert_pem_start[] = "132154654";
const uint8_t server_cert_pem_end[];

// 任务句柄
TaskHandle_t OTA_taskHandle = NULL;
static TaskHandle_t g_task_sscma_writer;
#define OTA_URL_SIZE 256
// 定义缓冲区大小
#define HIMAX_OTA_RINGBUFF_SIZE 20480 // 例如0.3MB

#define OTA_BUFF_SIZE 1024*1024 - 512 // 例如0.3MB

// sscma_client_handle_t sscam_ota_client = NULL;
// sscma_client_io_handle_t _sscma_flasher_io = NULL;
sscma_client_flasher_handle_t sscam_ota_flasher = NULL;


bool ota_flag = false;
bool esp32_ota = false;
bool himax_ota = false;
bool ai_modle_ota = false;


char ota_url[256]; 
int ota_type = OTA_UNKNOWN;

static RingbufHandle_t himax_ota_ringbuffer;
static volatile atomic_bool g_sscma_writer_abort = ATOMIC_VAR_INIT(false);
static SemaphoreHandle_t g_sem_sscma_writer_done;
static ota_sscma_writer_userdata_t g_sscma_writer_userdata;



/*     HIMAX start      */



esp_err_t bsp_spiffs_init(char *mount_point, size_t max_files)
{
    static bool inited = false;
    if (inited)
    {
        return ESP_OK;
    }
    esp_vfs_spiffs_conf_t conf = {
        .base_path = mount_point,
        .partition_label = "storage",
        .max_files = max_files,
        .format_if_mount_failed = false,
    };
    esp_vfs_spiffs_register(&conf);

    size_t total = 0, used = 0;
    esp_err_t ret_val = esp_spiffs_info(conf.partition_label, &total, &used);
    if (ret_val != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret_val));
    }
    else
    {
        ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    }
    return ret_val;
}




void bsp_sscma_flasher_init_legacy(void)
{

        const sscma_client_flasher_we2_config_t flasher_config = {
        .reset_gpio_num = GPIO_NUM_5,
        .io_expander = NULL,
        .flags.reset_use_expander = false,
        .flags.reset_high_active = false,
        .user_ctx = NULL,
    };

    sscma_client_new_flasher_we2_uart(io, &flasher_config, &sscam_ota_flasher);
    sscma_client_set_model(client, 1);
}

static const char *ota_type_str(int ota_type)
{
    if (ota_type == OTA_ESP32) return "esp32 firmware";
    else if (ota_type == OTA_HIMAX) return "himax firmware";
    else if (ota_type == OTA_AI_MODLE) return "ai model";
    else return "unknown ota";
}

static esp_err_t __http_event_handler(esp_http_client_event_t *evt)
{
    static int content_len, written_len, step_bytes, last_report_bytes;
    ota_sscma_writer_userdata_t *userdata = evt->user_data;

    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
            content_len = 0;
            written_len = 0;
            //clear the ringbuffer

            size_t len;
            void* item;
            // 使用 xRingbufferReceiveUpTo 来接收并清空缓冲区
            while ((item = xRingbufferReceiveUpTo(himax_ota_ringbuffer, &len, 0, HIMAX_OTA_RINGBUFF_SIZE))) {
                // 返回 item 到环形缓冲区
                vRingbufferReturnItem(himax_ota_ringbuffer, item);
            }
            
            atomic_store(&g_sscma_writer_abort, false);
            xSemaphoreTake(g_sem_sscma_writer_done, 0);  //clear the semaphore if there's dirty one

            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            break;
        case HTTP_EVENT_ON_DATA:
            // ESP_LOGV(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            // ESP_LOGW(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);

            if (content_len == 0) {
                content_len = esp_http_client_get_content_length(evt->client);
                ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, content_len=%d", content_len);
                userdata->content_len = content_len;
                userdata->err = ESP_OK;
                xTaskNotifyGive(g_task_sscma_writer);

                step_bytes = (int)(content_len / 10);
                last_report_bytes = step_bytes;
            }


            if (userdata->err != ESP_OK) {
                ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, userdata->err != ESP_OK (0x%x), error happened in sscma writer", userdata->err);
                break;  // don't waste time on ringbuffer send
            }

            //push to ringbuffer
            //himax will move the written bytes into flash every 1MB, it will take pretty long.
            //we give it 1min?
            int i = 0;
            for (i = 0; i < 60; i++)
            {
                if (xRingbufferSend(himax_ota_ringbuffer, evt->data, evt->data_len, pdMS_TO_TICKS(1000)) == pdTRUE)
                    break;
    
            }
            if (i == 60) {
                ESP_LOGW(TAG, "HTTP_EVENT_ON_DATA, ringbuffer full? this should never happen!");
            }
            written_len += evt->data_len;
            if (written_len >= last_report_bytes) {
                ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, http get len: %d, %d%%", written_len, (int)(100 * written_len / content_len));
                last_report_bytes += step_bytes;
            }
            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_DISCONNECTED");
            break;
        case HTTP_EVENT_REDIRECT:
            ESP_LOGD(TAG, "HTTP_EVENT_REDIRECT, auto redirection disabled?");
            break;
    }
    return ESP_OK;
}


void https_request(void) {
    esp_http_client_config_t http_client_config = {0}; // 确保结构体零初始化
    esp_http_client_handle_t http_client = NULL;


    http_client_config.url = "http://192.168.124.43/firmware/human_pose.tflite";
    http_client_config.method = HTTP_METHOD_GET;
    http_client_config.timeout_ms = 5000;
    http_client_config.crt_bundle_attach = esp_crt_bundle_attach;
    http_client_config.user_data = &g_sscma_writer_userdata;
    http_client_config.buffer_size = 512;
    http_client_config.event_handler = __http_event_handler;

#ifdef CONFIG_SKIP_COMMON_NAME_CHECK
    http_client_config.skip_cert_common_name_check = true;
#endif

    http_client = esp_http_client_init(&http_client_config);
    if (http_client == NULL) {
        printf("HTTP client init failed\n");
        return;
    }

    esp_err_t err = esp_http_client_perform(http_client);
    if (err == ESP_OK) {
        printf("HTTP GET request successful\n");

    } else {
        printf("HTTP GET request failed: %s\n", esp_err_to_name(err));
    }

    esp_http_client_cleanup(http_client);
}


static void sscma_ota_process()
{
    ESP_LOGI(TAG, "starting sscma ota download");
    ESP_LOGI(TAG, "ota_url = %s\r\n",ota_url);
    

    esp_err_t ret = ESP_OK;
    
    esp_http_client_config_t http_client_config  = {0};
    esp_http_client_handle_t http_client = NULL;

    http_client_config.url = ota_url;
    http_client_config.method = HTTP_METHOD_GET;
    http_client_config.timeout_ms = 5000;
    http_client_config.crt_bundle_attach = esp_crt_bundle_attach;
    http_client_config.user_data = &g_sscma_writer_userdata;
    http_client_config.buffer_size = 512;
    http_client_config.event_handler = __http_event_handler;
#ifdef CONFIG_SKIP_COMMON_NAME_CHECK
    http_client_config.skip_cert_common_name_check = true;
#endif

    http_client = esp_http_client_init(&http_client_config);
    ESP_GOTO_ON_FALSE(http_client != NULL, ESP_ERR_OTA_CONNECTION_FAIL, sscma_ota_end,
                      TAG, "sscma ota, http client init fail");

    g_sscma_writer_userdata.ota_type = ota_type;
    g_sscma_writer_userdata.http_client = http_client;
    g_sscma_writer_userdata.err = ESP_OK;

    int64_t start = esp_timer_get_time();
    esp_err_t err = esp_http_client_perform(http_client);

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "sscma ota, HTTP GET Status = %d, content_length = %"PRId64", take %lld us",
                esp_http_client_get_status_code(http_client),
                esp_http_client_get_content_length(http_client),
                esp_timer_get_time() - start);
     if (!esp_http_client_is_complete_data_received(http_client)) {
            ESP_LOGW(TAG, "sscma ota, HTTP finished but incompleted data received");
            // maybe network connection broken?
            ret = ESP_ERR_OTA_DOWNLOAD_FAIL;
            atomic_store(&g_sscma_writer_abort, true);
            xSemaphoreTake(g_sem_sscma_writer_done, pdMS_TO_TICKS(60000));
        } else {
            xSemaphoreTake(g_sem_sscma_writer_done, pdMS_TO_TICKS(60000));
            ret = g_sscma_writer_userdata.err;  //here may be OK, but might have errors in sscma writer task.
        }
    } else {
        ESP_LOGE(TAG, "sscma ota, HTTP GET error happened: %s", esp_err_to_name(err));
        //error defines:
        //https://docs.espressif.com/projects/esp-idf/zh_CN/v5.2.1/esp32s3/api-reference/protocols/esp_http_client.html#macros
        ret = ESP_ERR_OTA_DOWNLOAD_FAIL;  //we sum all these errors as download failure, easier for upper caller

        atomic_store(&g_sscma_writer_abort, true);
        xSemaphoreTake(g_sem_sscma_writer_done, pdMS_TO_TICKS(10000));
    }

sscma_ota_end:
    
    if (http_client) esp_http_client_close(http_client);
    if (http_client) esp_http_client_cleanup(http_client);
    g_sscma_writer_userdata.http_client = NULL;


}



static void __sscma_writer_task(void *p_arg)
{
    ota_sscma_writer_userdata_t *userdata = &g_sscma_writer_userdata;
    int content_len;
    
    while (1) {                                     
        assert(sscam_ota_flasher != NULL);
        assert(client != NULL);
        
        int sscma_flasher_chunk_size_decided = 128;
        
        sscma_client_info_t *info;
        if (sscma_client_get_info(client, &info, true) == ESP_OK)
        {
            ESP_LOGI(TAG, "--------------------------------------------");
            ESP_LOGI(TAG, "           sscma client info");
            ESP_LOGI(TAG, "ID: %s", (info->id != NULL) ? info->id : "NULL");
            ESP_LOGI(TAG, "Name: %s", (info->name != NULL) ? info->name : "NULL");
            ESP_LOGI(TAG, "Hardware Version: %s", (info->hw_ver != NULL) ? info->hw_ver : "NULL");
            ESP_LOGI(TAG, "Software Version: %s", (info->sw_ver != NULL) ? info->sw_ver : "NULL");
            ESP_LOGI(TAG, "Firmware Version: %s", (info->fw_ver != NULL) ? info->fw_ver : "NULL");
            ESP_LOGI(TAG, "--------------------------------------------");

        }
        else
        {
            ESP_LOGW(TAG, "sscma client get info failed, will try to flash anyway\n");
            // userdata->err = ESP_ERR_OTA_SSCMA_START_FAIL;
            // goto sscma_writer_end;
        }
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        content_len = userdata->content_len;
        
        if (content_len <= 0) continue;

        ESP_LOGI(TAG, "starting sscma writer, content_len=%d ...", content_len);

        bool is_abort = false;
        uint32_t flash_addr = 0x0;
        if (ota_type == OTA_HIMAX) {
            
            flash_addr = 0x0;
            ESP_LOGI(TAG, "flash Himax firmware ...");
        } else if(ota_type == OTA_AI_MODLE){
            
            flash_addr = 0xA00000;
            ESP_LOGI(TAG, "flash Himax 4th ai model ...");
        }

        
        int64_t start = esp_timer_get_time();
        //sscma_client_ota_start
        if (sscma_client_ota_start(client, sscam_ota_flasher, flash_addr) != ESP_OK) {
            ESP_LOGE(TAG, "sscma writer, sscma_client_ota_start failed");
            userdata->err = ESP_ERR_OTA_SSCMA_START_FAIL;
            goto sscma_writer_end;
        }

        ESP_LOGI(TAG, "sscma_client_ota_start OK\n");


        // drain the ringbuffer and write to himax
        int written_len = 0;
        int remain_len = content_len - written_len;
        int step_bytes = (int)(content_len / 10);
        int last_report_bytes = step_bytes;
        int target_bytes;
        int retry_cnt = 0;
        void *chunk = malloc(sscma_flasher_chunk_size_decided);
        void *tmp;
        UBaseType_t available_bytes = 0;
        size_t rcvlen, rcvlen2;

        while (remain_len > 0 && !atomic_load(&g_sscma_writer_abort)) {
            target_bytes = MIN(sscma_flasher_chunk_size_decided, remain_len);
            vRingbufferGetInfo(himax_ota_ringbuffer, NULL, NULL, NULL, NULL, &available_bytes);
            if ((int)available_bytes < target_bytes) {
                vTaskDelay(pdMS_TO_TICKS(1));
                retry_cnt++;
                if (retry_cnt > 60000) {
                    ESP_LOGE(TAG, "sscma writer reach timeout on ringbuffer!!! want_bytes: %d", target_bytes);
                    userdata->err = ESP_ERR_OTA_SSCMA_WRITE_FAIL;
                    goto sscma_writer_end0;
                }
                continue;
            }
            retry_cnt = 0;

            rcvlen = 0, rcvlen2 = 0;
            tmp = xRingbufferReceiveUpTo(himax_ota_ringbuffer, &rcvlen, 0, target_bytes);
            if (!tmp) {
                ESP_LOGW(TAG, "sscma writer, ringbuffer insufficient? this should never happen!");
                userdata->err = ESP_ERR_OTA_SSCMA_INTERNAL_ERR;
                goto sscma_writer_end0;
            }
            memset(chunk, 0, sscma_flasher_chunk_size_decided);
            memcpy(chunk, tmp, rcvlen);
            target_bytes -= rcvlen;
            vRingbufferReturnItem(himax_ota_ringbuffer, tmp);
            //rollover?
            if (target_bytes > 0) {
                tmp = xRingbufferReceiveUpTo(himax_ota_ringbuffer, &rcvlen2, 0, target_bytes);  //receive the 2nd part
                if (!tmp) {
                    ESP_LOGW(TAG, "sscma writer, ringbuffer insufficient? this should never happen [2]!");
                    userdata->err = ESP_ERR_OTA_SSCMA_INTERNAL_ERR;
                    goto sscma_writer_end0;
                }
                memcpy(chunk + rcvlen, tmp, rcvlen2);
                target_bytes -= rcvlen2;
                vRingbufferReturnItem(himax_ota_ringbuffer, tmp);
            }

            if (target_bytes != 0) {
                ESP_LOGW(TAG, "sscma writer, this really should never happen!");
                userdata->err = ESP_ERR_OTA_SSCMA_INTERNAL_ERR;
                goto sscma_writer_end0;
            }

            //write to sscma client
            // ESP_LOGD(TAG, "sscma writer, sscma_client_ota_write");
            if (sscma_client_ota_write(client, chunk, sscma_flasher_chunk_size_decided) != ESP_OK)
            {
                ESP_LOGW(TAG, "sscma writer, sscma_client_ota_write failed\n");
                userdata->err = ESP_ERR_OTA_SSCMA_WRITE_FAIL;
                goto sscma_writer_end0;
            } else {
                written_len += (rcvlen + rcvlen2);
                if (written_len >= last_report_bytes) {
                    last_report_bytes += step_bytes;
                    ESP_LOGI(TAG, "%s ota, bytes written: %d, %d%%", ota_type_str(ota_type), written_len, (int)(100 * written_len / content_len));
                }
            }

            remain_len -= (rcvlen + rcvlen2);
        }  //while

        if (atomic_load(&g_sscma_writer_abort)) {
            is_abort = true;
            ESP_LOGW(TAG, "%s sscma writer, abort, take %lld us", ota_type_str(ota_type), esp_timer_get_time() - start);
        } else {
            ESP_LOGD(TAG, "%s sscma writer, write done, take %lld us", ota_type_str(ota_type), esp_timer_get_time() - start);
            sscma_client_ota_finish(client);
            ESP_LOGI(TAG, "%s sscma writer, finish, take %lld us, speed %d KB/s", ota_type_str(ota_type), esp_timer_get_time() - start,
                            (int)(1000 * content_len / (esp_timer_get_time() - start)));
            sscma_client_invoke(client, -1, false, false);
        }
sscma_writer_end0:
        free(chunk);
sscma_writer_end:

        if (is_abort || userdata->err != ESP_OK) {
            ESP_LOGW(TAG, "sscma_client_ota_abort !!!");
            sscma_client_ota_abort(client);
        }
        xSemaphoreGive(g_sem_sscma_writer_done);
    }  //while(1)
}



/*     HIMAX end      */


/* Event handler for catching system events */
static void event_handler(void* arg, esp_event_base_t event_base,
                          int32_t event_id, void* event_data)
{
    if (event_base == ESP_HTTPS_OTA_EVENT) {
        switch (event_id) {
            case ESP_HTTPS_OTA_START:
                ESP_LOGI(TAG, "OTA started");
                break;
            case ESP_HTTPS_OTA_CONNECTED:
                ESP_LOGI(TAG, "Connected to server");
                break;
            case ESP_HTTPS_OTA_GET_IMG_DESC:
                ESP_LOGI(TAG, "Reading Image Description");
                break;
            case ESP_HTTPS_OTA_VERIFY_CHIP_ID:
                ESP_LOGI(TAG, "Verifying chip id of new image: %d", *(esp_chip_id_t *)event_data);
                break;
            case ESP_HTTPS_OTA_DECRYPT_CB:
                ESP_LOGI(TAG, "Callback to decrypt function");
                break;
            case ESP_HTTPS_OTA_WRITE_FLASH:
                ESP_LOGD(TAG, "Writing to flash: %d written", *(int *)event_data);
                break;
            case ESP_HTTPS_OTA_UPDATE_BOOT_PARTITION:
                ESP_LOGI(TAG, "Boot partition updated. Next Partition: %d", *(esp_partition_subtype_t *)event_data);
                break;
            case ESP_HTTPS_OTA_FINISH:
                ESP_LOGI(TAG, "OTA finish");
                break;
            case ESP_HTTPS_OTA_ABORT:
                ESP_LOGI(TAG, "OTA abort");
                break;
        }
    }
}

static esp_err_t validate_image_header(esp_app_desc_t *new_app_info)
{
    if (new_app_info == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    const esp_partition_t *running = esp_ota_get_running_partition();
    esp_app_desc_t running_app_info;
    if (esp_ota_get_partition_description(running, &running_app_info) == ESP_OK) {
        ESP_LOGI(TAG, "Running firmware version: %s", running_app_info.version);
    }

#ifndef CONFIG_EXAMPLE_SKIP_VERSION_CHECK
    if (memcmp(new_app_info->version, running_app_info.version, sizeof(new_app_info->version)) == 0) {
        ESP_LOGW(TAG, "Current running version is the same as a new. We will not continue the update.");
        return ESP_FAIL;
    }
#endif

#ifdef CONFIG_BOOTLOADER_APP_ANTI_ROLLBACK
    /**
     * Secure version check from firmware image header prevents subsequent download and flash write of
     * entire firmware image. However this is optional because it is also taken care in API
     * esp_https_ota_finish at the end of OTA update procedure.
     */
    const uint32_t hw_sec_version = esp_efuse_read_secure_version();
    if (new_app_info->secure_version < hw_sec_version) {
        ESP_LOGW(TAG, "New firmware security version is less than eFuse programmed, %"PRIu32" < %"PRIu32, new_app_info->secure_version, hw_sec_version);
        return ESP_FAIL;
    }
#endif

    return ESP_OK;
}

static esp_err_t _http_client_init_cb(esp_http_client_handle_t http_client)
{
    esp_err_t err = ESP_OK;
    /* Uncomment to add custom headers to HTTP request */
    // err = esp_http_client_set_header(http_client, "Custom-Header", "Value");
    return err;
}



static void __app_event_handler(void *handler_args, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    ota_event_data_t *event_data_ptr = (ota_event_data_t *)event_data;
        // 打印事件类型
    printf("Received OTA event type: %d\n", event_data_ptr->event_type);
    // 打印 URL
    printf("Received URL: %s\n", event_data_ptr->url);


    strcpy(ota_url,event_data_ptr->url);
    ota_type = event_data_ptr->event_type;

    if (event_base == OTA_EVENT_BASE) {
        switch (event_id) {
            case OTA_ESP32:
                printf("recive OTA_ESP32 oder\r\n");
                ota_flag = true;
                esp32_ota = true;
                
            break;
            case OTA_HIMAX:
                printf("recive OTA_HIMAX oder\r\n");
                ota_flag = true;
                himax_ota = true;

            break;
            case OTA_AI_MODLE:
                printf("recive OTA_AI_MODLE oder\r\n");
                ota_flag = true;
                ai_modle_ota = true;
            default:
                break;
        }
    }
    else{
        printf("other event\r\n");
    }
}




void ota_task(void *pvParameter)
{
    while (1)
    {
        if(ota_flag == true && esp32_ota == true){
            ESP_LOGI(TAG, "Starting Advanced OTA ");

            esp_err_t ota_finish_err = ESP_OK;
            esp_http_client_config_t config = {
                .url = ota_url,
                .cert_pem = (char *)"",
                .timeout_ms = 2000,
                .keep_alive_enable = true,
            };


            esp_https_ota_config_t ota_config = {
                .http_config = &config,
                .http_client_init_cb = _http_client_init_cb, // Register a callback to be invoked after esp_http_client is initialized
        #ifdef CONFIG_EXAMPLE_ENABLE_PARTIAL_HTTP_DOWNLOAD
                .partial_http_download = true,
                .max_http_request_size = CONFIG_EXAMPLE_HTTP_REQUEST_SIZE,
        #endif
            };

            esp_https_ota_handle_t https_ota_handle = NULL;
            esp_err_t err = esp_https_ota_begin(&ota_config, &https_ota_handle);
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "ESP HTTPS OTA Begin failed");
                goto ota_end;
            }

            esp_app_desc_t app_desc;
            err = esp_https_ota_get_img_desc(https_ota_handle, &app_desc);
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "esp_https_ota_get_img_desc failed");
                goto ota_end;
            }
            err = validate_image_header(&app_desc);
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "image header verification failed");
                goto ota_end;
            }

            while (1) {
                err = esp_https_ota_perform(https_ota_handle);
                if (err != ESP_ERR_HTTPS_OTA_IN_PROGRESS) {
                    break;
                }
                // esp_https_ota_perform returns after every read operation which gives user the ability to
                // monitor the status of OTA upgrade by calling esp_https_ota_get_image_len_read, which gives length of image
                // data read so far.
                ESP_LOGD(TAG, "Image bytes read: %d", esp_https_ota_get_image_len_read(https_ota_handle));
            }

            if (esp_https_ota_is_complete_data_received(https_ota_handle) != true) {
                // the OTA image was not completely received and user can customise the response to this situation.
                ESP_LOGE(TAG, "Complete data was not received.");
            } else {
                ota_finish_err = esp_https_ota_finish(https_ota_handle);
                if ((err == ESP_OK) && (ota_finish_err == ESP_OK)) {
                    ESP_LOGI(TAG, "ESP_HTTPS_OTA upgrade successful. Rebooting ...");
                    vTaskDelay(1000 / portTICK_PERIOD_MS);
                    esp_restart();
                } else {
                    if (ota_finish_err == ESP_ERR_OTA_VALIDATE_FAILED) {
                        ESP_LOGE(TAG, "Image validation failed, image is corrupted");
                    }
                    ESP_LOGE(TAG, "ESP_HTTPS_OTA upgrade failed 0x%x", ota_finish_err);
                    
                }
            }

        ota_end:
            ota_flag = false;
            esp32_ota = false;
            esp_https_ota_abort(https_ota_handle);
            ESP_LOGE(TAG, "ESP_HTTPS_OTA upgrade failed");
            
        }else if (himax_ota && ota_flag)
        {
            himax_ota = false;
            ota_flag = false;
            sscma_ota_process();

        }else if(ota_flag && ai_modle_ota){
            ota_flag = false;
            ai_modle_ota = false;
            printf("task ai_modle_ota will run\r\n");
            sscma_ota_process();
        }
        
        
        vTaskDelay(2000);
        
    }
    
    
}








void ota_init()
{
    ESP_LOGI(TAG, "OTA init start");
    g_sem_sscma_writer_done = xSemaphoreCreateBinary();

    // ESP_ERROR_CHECK(esp_event_loop_create_default());

    // // 定义静态结构体和存储
    // StaticRingbuffer_t himax_ota_buffer_struct;
    // uint8_t himax_ota_buffer_storage[HIMAX_OTA_RINGBUFF_SIZE];  // 使用栈上分配的存储空间


 
    StaticRingbuffer_t *himax_ota_buffer_struct = (StaticRingbuffer_t *)malloc(sizeof(StaticRingbuffer_t));
    uint8_t *himax_ota_buffer_storage = (uint8_t *)malloc(HIMAX_OTA_RINGBUFF_SIZE);

    // 初始化环形缓冲区
    himax_ota_ringbuffer = xRingbufferCreateStatic(HIMAX_OTA_RINGBUFF_SIZE, RINGBUF_TYPE_BYTEBUF, himax_ota_buffer_storage, himax_ota_buffer_struct);

    if (himax_ota_ringbuffer != NULL) {
        printf("himax_ota_ringbuffer init successfully.\n");
    } else {
        printf("Failed to initialize himax_ota_ringbuffer.\n");
    }



//     StaticRingbuffer_t himax_ota_buffer_struct;
//    // 获取数据区域的地址
//     uint8_t *himax_ota_buffer_storage = (uint8_t *)(((uintptr_t)DATA_AREA_ADDR + 3) & ~3);

//     // 确保存储区和环形缓冲区大小匹配
//     if (HIMAX_OTA_RINGBUFF_SIZE > DATA_AREA_SIZE) {
//         ESP_LOGE(TAG, "Buffer size exceeds storage size!");
//     }


//     // 初始化环形缓冲区
//     himax_ota_ringbuffer = xRingbufferCreateStatic(HIMAX_OTA_RINGBUFF_SIZE, RINGBUF_TYPE_BYTEBUF, himax_ota_buffer_storage, &himax_ota_buffer_struct);

    // if (himax_ota_ringbuffer != NULL) {
    //     ESP_LOGI("RINGBUFFER", "himax_ota_ringbuffer init successfully.");
    // } else {
    //     ESP_LOGE("RINGBUFFER", "Failed to initialize himax_ota_ringbuffer.");
    // }





    ESP_ERROR_CHECK(esp_event_handler_register(ESP_HTTPS_OTA_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(OTA_EVENT_BASE, ESP_EVENT_ANY_ID, &__app_event_handler, NULL));
/*

 esp_err_t ret = esp_event_post(OTA_EVENT_BASE, OTA_ESP32 || OTA_HIMAX || AI, &event_data, sizeof(event_data), pdMS_TO_TICKS(10000));
*/
    bsp_sscma_flasher_init_legacy();

    xTaskCreate(&ota_task, "ota_task", 1024 * 8, NULL, 5, &OTA_taskHandle);
    xTaskCreate(&__sscma_writer_task, "__sscma_writer_task", 1024 * 10, NULL, 9, &g_task_sscma_writer);
    ESP_LOGI("g_task_sscma_writer", "g_task_sscma_writer creat successfully.");
}



