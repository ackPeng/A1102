#include "app_himax.h"

// #include "app_mqtt.h"
#include "cJSON.h"
#include "cJSON_Utils.h"

static const char *TAG = "app_himax";


void on_log(sscma_client_handle_t client, const sscma_client_reply_t *reply, void *user_ctx)
{
    if(mqtt_connect == true){
        char *data = malloc(reply->len);
        
        if (data) {
            memcpy(data,reply->data,reply->len);
            if (xQueueSend(queue, &data, pdMS_TO_TICKS(100)) != pdPASS) {
                ESP_LOGW(TAG, "Queue is full, dropping data");
                free(data); 
            }
        }
    }else{
        // ESP_LOGI(TAG,"wait mqtt connect");
    }
}


void on_response(sscma_client_handle_t client, const sscma_client_reply_t *reply, void *user_ctx)
{
    if(mqtt_connect == true){
        char *data = malloc(reply->len);
        
        if (data) {
            memcpy(data,reply->data,reply->len);
            if (xQueueSend(queue, &data, pdMS_TO_TICKS(100)) != pdPASS) {
                ESP_LOGW(TAG, "Queue is full, dropping data");
                free(data);
            }
        }
    }else{
        // ESP_LOGI(TAG,"wait mqtt connect");
    }
}


void on_event(sscma_client_handle_t client, const sscma_client_reply_t *reply, void *user_ctx)
{
    if(mqtt_connect == true){
        char *data = malloc(reply->len);
        if (data) {
            memcpy(data,reply->data,reply->len);
            if (xQueueSend(queue, &data, pdMS_TO_TICKS(100)) != pdPASS) {
                ESP_LOGW(TAG, "Queue is full, dropping data");
                free(data);
            }
        }
    }else{
        // ESP_LOGI(TAG,"wait mqtt connect");
    }

}



void on_connect(sscma_client_handle_t client, const sscma_client_reply_t *reply, void *user_ctx)
{

    sscma_client_info_t *info = NULL;
    if (sscma_client_get_info(client, &info, true) == ESP_OK)
    {
        printf("ID: %s\n", (info->id != NULL) ? info->id : "NULL");
        printf("Name: %s\n", (info->name != NULL) ? info->name : "NULL");
        printf("Hardware Version: %s\n", (info->hw_ver != NULL) ? info->hw_ver : "NULL");
        printf("Software Version: %s\n", (info->sw_ver != NULL) ? info->sw_ver : "NULL");
        printf("Firmware Version: %s\n", (info->fw_ver != NULL) ? info->fw_ver : "NULL");
    }
    else
    {
        printf("get info failed\n");
    }

    sscma_client_model_t *model = NULL;
    if (sscma_client_get_model(client, &model, true) == ESP_OK)
    {
        printf("ID: %d\n", model->id ? model->id : -1);
        printf("UUID: %s\n", model->uuid ? model->uuid : "N/A");
        
        if(model->uuid != NULL){
            printf("atoi(model->uuid) * 1000 = : %d\n", atoi(model->uuid) * 1000);
            printf("1000000000 + atoi(model->uuid) * 1000 + 140 : %d\n", (1000000000 + atoi(model->uuid) * 1000 + 140));
        }else{
            printf("model->uuid == NULL\r\n");
        }

        

        printf("Name: %s\n", model->name ? model->name : "N/A");
        printf("Version: %s\n", model->ver ? model->ver : "N/A");
        printf("URL: %s\n", model->url ? model->url : "N/A");
        printf("Checksum: %s\n", model->checksum ? model->checksum : "N/A");
        printf("Classes:\n");
        if (model->classes[0] != NULL)
        {
            for (int i = 0; model->classes[i] != NULL; i++)
            {
                printf("  - %s\n", model->classes[i]);
            }
        }
        else
        {
            printf("  N/A\n");
        }
    }

    sscma_client_wifi_t wifi;
    memset(&wifi, 0, sizeof(wifi));
    if (get_wifi_config(client, &wifi) == ESP_OK)
    {
        ESP_LOGI(TAG, "SSID: %s", wifi.ssid == NULL ? "NULL" : wifi.ssid);
        ESP_LOGI(TAG, "Password: %s", wifi.password == NULL ? "NULL" : wifi.password);

        nvs_handle_t my_handle;
        esp_err_t ret = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &my_handle);

        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Error (%s) opening NVS handle!", esp_err_to_name(ret));
            } else {
            ESP_LOGI(TAG, "NVS handle opened successfully");

            const char *ssid = wifi.ssid ? wifi.ssid : "null";
            const char *password = wifi.password ? wifi.password : "null";

            ret = nvs_set_str(my_handle, NVS_KEY_SSID, ssid);
            ESP_ERROR_CHECK(ret);

            ret = nvs_set_str(my_handle, NVS_KEY_PASSWORD, password);
            ESP_ERROR_CHECK(ret);

            ret = nvs_commit(my_handle);
            ESP_ERROR_CHECK(ret);

            nvs_close(my_handle);
        }
    }

    sscma_client_mqtt_t mqtt;
    memset(&mqtt, 0, sizeof(mqtt));
    if (get_mqtt_config(client, &mqtt) == ESP_OK)
    {
        ESP_LOGI(TAG, "00Username: %s", mqtt.username == NULL ? "NULL" : mqtt.username);
        ESP_LOGI(TAG, "Password: %s", mqtt.password == NULL ? "NULL" : mqtt.password);
        ESP_LOGI(TAG, "Address: %s", mqtt.address == NULL ? "NULL" : mqtt.address);
        ESP_LOGI(TAG, "Client ID: %s", mqtt.client_id == NULL ? "NULL" : mqtt.client_id);
        ESP_LOGI(TAG, "Port1: %d", mqtt.port1 == 0 ? 0 : mqtt.port1);
        ESP_LOGI(TAG, "use_ssl1: %d", mqtt.use_ssl1);

        nvs_handle_t my_handle;
        esp_err_t ret = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &my_handle);

        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Error (%s) opening NVS handle!", esp_err_to_name(ret));
        } else {
            ESP_LOGI(TAG, "NVS handle opened successfully");

            const char *username = mqtt.username ? mqtt.username : "null";
            const char *password = mqtt.password ? mqtt.password : "null";
            const char *address = mqtt.address ? mqtt.address : "null";
            const char *client_id = mqtt.client_id ? mqtt.client_id : "null";
            const int *port1 = mqtt.port1 ? mqtt.port1 : "null";
            const int *use_ssl1 = mqtt.use_ssl1;

            ret = nvs_set_str(my_handle, NVS_KEY_MQTT_USERNAME, username);
            ESP_ERROR_CHECK(ret);
            ESP_LOGI(TAG, "NVS handle opened successfully");
            ret = nvs_set_str(my_handle, NVS_KEY_MQTT_PASSWORD, password);
            ESP_ERROR_CHECK(ret);

            ret = nvs_set_str(my_handle, NVS_KEY_MQTT_ADDRESS, address);
            ESP_ERROR_CHECK(ret);

            ret = nvs_set_i32(my_handle, NVS_KEY_MQTT_PORT1, port1);
            ESP_ERROR_CHECK(ret);

            ret = nvs_set_str(my_handle, NVS_KEY_MQTT_CLIENT_ID, client_id);
            ESP_ERROR_CHECK(ret);

            ret = nvs_set_i32(my_handle, NVS_KEY_MQTT_USE_SSL1, use_ssl1);
            ESP_ERROR_CHECK(ret);

            ret = nvs_commit(my_handle);
            ESP_ERROR_CHECK(ret);

            nvs_close(my_handle);
        }
        // if (read_config == 1)
        // {
        //     //esp_restart();
        // }
    }
    else
    {
        ESP_LOGE(TAG, "get mqtt config failed\n");
    }

    
    himax_config = true;

}




void himax_sscam_init()
{
    uart_config_t uart_config = {
        .baud_rate = 921600,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    int intr_alloc_flags = 0;

    ESP_ERROR_CHECK(uart_driver_install(UART_NUM_1, 8 * 1024, 0, 0, NULL, intr_alloc_flags));
    ESP_ERROR_CHECK(uart_param_config(UART_NUM_1, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_NUM_1, 21, 20, -1, -1));

    sscma_client_io_uart_config_t io_uart_config = {
        .user_ctx = NULL,
    };

    sscma_client_config_t sscma_client_config = SSCMA_CLIENT_CONFIG_DEFAULT();
    sscma_client_config.reset_gpio_num = 5;

    sscma_client_new_io_uart_bus((sscma_client_uart_bus_handle_t)1, &io_uart_config, &io);

    sscma_client_new(io, &sscma_client_config, &client);

    const sscma_client_callback_t callback = {
        .on_connect = on_connect,
        .on_event = on_event,
        .on_response = on_response,
        .on_log = on_log,
    };

    if(client == NULL){
        printf("client null\r\n");
        return;
    }

    printf("client not null\r\n");

    if (sscma_client_register_callback(client, &callback, NULL) != ESP_OK)
    {
        printf("set callback failed\n");
        abort();
    }

    sscma_client_init(client);

    vTaskDelay(2000);

}