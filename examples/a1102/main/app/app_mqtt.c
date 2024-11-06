#include "app_mqtt.h"
#include "app_ota.h"

static const char *TAG = "mqtt_app";

esp_mqtt_client_handle_t mqtt_client;
EventGroupHandle_t s_mqtt_event_group;



char mqtt_tx_topic[128];
char mqtt_rx_topic[128];
char mqtt_client_id[64];

esp_err_t read_mqtt_config(sscma_client_mqtt_t *mqtt)
{
    nvs_handle_t my_handle;
    esp_err_t ret = nvs_open(NVS_NAMESPACE, NVS_READONLY, &my_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error (%s) opening NVS handle!", esp_err_to_name(ret));
        mqtt->username = "testusername";
        mqtt->password = "testpassword";
        mqtt->address = "testaddress";
        mqtt->port1 = 1883;
        mqtt->use_ssl1 = 0;
        mqtt->client_id = "testclient_id";
        // read_config = 1;
        return ret;
    }

    size_t required_size;
    ret = nvs_get_str(my_handle, NVS_KEY_MQTT_USERNAME, NULL, &required_size);
    if (ret == ESP_OK) {
        mqtt->username = malloc(required_size);
        ret = nvs_get_str(my_handle, NVS_KEY_MQTT_USERNAME, mqtt->username, &required_size);
    } else {
        mqtt->username = NULL;
    }

    ret = nvs_get_str(my_handle, NVS_KEY_MQTT_PASSWORD, NULL, &required_size);
    if (ret == ESP_OK) {
        mqtt->password = malloc(required_size);
        ret = nvs_get_str(my_handle, NVS_KEY_MQTT_PASSWORD, mqtt->password, &required_size);
    } else {
        mqtt->password = NULL;
    }

    ret = nvs_get_str(my_handle, NVS_KEY_MQTT_ADDRESS, NULL, &required_size);
    if (ret == ESP_OK) {
        mqtt->address = malloc(required_size);
        ret = nvs_get_str(my_handle, NVS_KEY_MQTT_ADDRESS, mqtt->address, &required_size);
    } else {
        mqtt->address = NULL;
    }

    ret = nvs_get_i32(my_handle, NVS_KEY_MQTT_PORT1, &mqtt->port1);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Port1 read successfully: %d", mqtt->port1);
    } else {
        mqtt->port1 = 0; 
    }
    
    ret = nvs_get_i32(my_handle, NVS_KEY_MQTT_USE_SSL1, &mqtt->use_ssl1);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Port1 read successfully: %d", mqtt->use_ssl1);
    } else {
        mqtt->use_ssl1 = 0; 
    }    

    ret = nvs_get_str(my_handle, NVS_KEY_MQTT_CLIENT_ID, NULL, &required_size);
    if (ret == ESP_OK) {
        mqtt->client_id = malloc(required_size);
        ret = nvs_get_str(my_handle, NVS_KEY_MQTT_CLIENT_ID, mqtt->client_id, &required_size);
    } else {
        mqtt->client_id = NULL;
    }

    nvs_close(my_handle);
    return ESP_OK;
}



static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;
    switch (event->event_id)
    {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            xEventGroupSetBits(s_mqtt_event_group, MQTT_CONNECTED_BIT);
            int msg_id = esp_mqtt_client_subscribe(mqtt_client, mqtt_rx_topic, 0);
            ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGW(TAG, "MQTT_EVENT_DISCONNECTED");
            xEventGroupClearBits(s_mqtt_event_group, MQTT_CONNECTED_BIT);
            break;
        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "MQTT_EVENT_DATA");
                // Make sure the data is handled as a string
            event->data[event->data_len] = '\0';  // Add string terminator

            // Parse JSON
            cJSON *json = cJSON_Parse((const char *)event->data);
            if (json == NULL) {
                printf("JSON parse error\n");
                return;
            }

            cJSON *type = cJSON_GetObjectItem(json, "type");
            cJSON *url = cJSON_GetObjectItem(json, "url");

            if (type && url) {
                printf("Type: %s\n", type->valuestring);
                printf("URL: %s\n", url->valuestring);

                ota_event_type_t event_type = OTA_UNKNOWN;
                if (strcmp(type->valuestring, "esp32_ota") == 0) {
                    event_type = OTA_ESP32;
                } else if (strcmp(type->valuestring, "himax_ota") == 0) {
                    event_type = OTA_HIMAX;
                } else if (strcmp(type->valuestring, "ai_model_ota") == 0) {
                    event_type = OTA_AI_MODLE;
                }

                // Create event data
                ota_event_data_t event_data;
                event_data.event_type = event_type;
                strncpy(event_data.url, url->valuestring, sizeof(event_data.url) - 1);
                event_data.url[sizeof(event_data.url) - 1] = '\0';  // Make sure the string ends

                if(event_type != OTA_UNKNOWN){
                    // Send event
                    esp_err_t err = esp_event_post(OTA_EVENT_BASE, event_type, &event_data, sizeof(event_data), portMAX_DELAY);
                    if (err != ESP_OK) {
                        printf("Failed to post event: %s\n", esp_err_to_name(err));
                    }
                }



            }
            cJSON_Delete(json);

            // sscma_client_write(client,"AT+INVOKE=1,0,0", 15);
            // sscma_client_invoke(client, -1, false, false);
            // sscma_client_write(client, event->data, event->data_len);
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
            //esp_restart();
            break;
        default:
            ESP_LOGI(TAG, "Other event id:%d", event->event_id);
            break;
    }
}




void mqtt_initialize()
{
    s_mqtt_event_group = xEventGroupCreate();
    
    sscma_client_mqtt_t mqtt;
    memset(&mqtt, 0, sizeof(mqtt));
    if (read_mqtt_config(&mqtt) == ESP_OK)
    {
        ESP_LOGI(TAG, "Username: %s", mqtt.username == NULL ? "NULL" : mqtt.username);
        ESP_LOGI(TAG, "Password: %s", mqtt.password == NULL ? "NULL" : mqtt.password);
        ESP_LOGI(TAG, "Address: %s", mqtt.address == NULL ? "NULL" : mqtt.address);
        ESP_LOGI(TAG, "Client ID: %s", mqtt.client_id == NULL ? "NULL" : mqtt.client_id);
        ESP_LOGI(TAG, "port: %d", mqtt.port1 == 0 ? 0 : mqtt.port1);
        ESP_LOGI(TAG, "read_use_ssl: %d", mqtt.use_ssl1);
    }

    snprintf(mqtt_tx_topic, sizeof(mqtt_tx_topic), "sscma/v0/%s/tx", mqtt.client_id);
    snprintf(mqtt_rx_topic, sizeof(mqtt_rx_topic), "sscma/v0/%s/rx", mqtt.client_id);

    ESP_LOGI(TAG, "MQTT client id: %s", mqtt_client_id);

    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = mqtt.address,
        .broker.address.port = mqtt.port1,
        .credentials.client_id = mqtt.client_id,
        .credentials.username = mqtt.username,
        .credentials.authentication.password = mqtt.password,
        .broker.verification.certificate = mqtt.use_ssl1
    };

    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(mqtt_client, MQTT_EVENT_ANY, mqtt_event_handler, NULL);
    esp_mqtt_client_start(mqtt_client);
}