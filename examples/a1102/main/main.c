#include "main.h"

void on_event(sscma_client_handle_t client, const sscma_client_reply_t *reply, void *user_ctx)
{
     EventBits_t mqttConnectBits = xEventGroupGetBits(s_mqtt_event_group);
    if (mqttConnectBits & MQTT_CONNECTED_BIT)
    {
        int msg_id = esp_mqtt_client_publish(mqtt_client, mqtt_tx_topic, reply->data, reply->len, 0, 0);
        if (msg_id < 0)
        {
            ESP_LOGE(TAG, "Failed to publish: %d", msg_id);
            vTaskDelay(2000 / portTICK_PERIOD_MS);
        }
        else
        {
            ESP_LOGI(TAG, "Publish success: %d", reply->len);
            vTaskDelay(2000 / portTICK_PERIOD_MS);
        }
        return;
        
    }

    char *img = NULL;
    int img_size = 0;
    if (sscma_utils_fetch_image_from_reply(reply, &img, &img_size) == ESP_OK)
    {
        ESP_LOGI(TAG, "image_size: %d\n", img_size);
        free(img);
    }

    vPortEnterCritical();
    sscma_client_box_t *boxes = NULL;
    int box_count = 0;
    for (int i = 0; i < 9; i++)
    {
        reg[i] = -1000;
    }
    if (sscma_utils_fetch_boxes_from_reply(reply, &boxes, &box_count) == ESP_OK)
    {
        if (box_count > 0)
        {
            for (int i = 0; i < box_count && i < 9; i++)
            {
                reg[i] = boxes[i].target * 1000 + boxes[i].score * 10;
            }
        }
        free(boxes);
    }

    sscma_client_class_t *classes = NULL;
    int class_count = 0;
    if (sscma_utils_fetch_classes_from_reply(reply, &classes, &class_count) == ESP_OK)
    {
        if (class_count > 0)
        {
            for (int i = 0; i < class_count && i < 9; i++)
            {
                reg[i] = classes[i].target * 1000 + classes[i].score * 10;
            }
        }
        free(classes);
    }
        is_captured = true;
    vPortExitCritical();
    return;
}

void on_log(sscma_client_handle_t client, const sscma_client_reply_t *reply, void *user_ctx)
{
    EventBits_t mqttConnectBits = xEventGroupGetBits(s_mqtt_event_group);
    if (mqttConnectBits & MQTT_CONNECTED_BIT)
    {
        int msg_id = esp_mqtt_client_publish(mqtt_client, mqtt_tx_topic, reply->data, reply->len, 0, 0);
        if (msg_id < 0)
        {
            ESP_LOGE(TAG, "Failed to publish: %d", msg_id);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
        else
        {
            ESP_LOGI(TAG, "Publish success: %d", reply->len);
        }
        return;
    }

    if (reply->len >= 100)
    {
        strcpy(&reply->data[100 - 4], "...");
    }

    ESP_LOGI(TAG, "log: %s\n", reply->data);
}

void on_response(sscma_client_handle_t client, const sscma_client_reply_t *reply, void *user_ctx)
{
    EventBits_t mqttConnectBits = xEventGroupGetBits(s_mqtt_event_group);
    if (mqttConnectBits & MQTT_CONNECTED_BIT)
    {
        int msg_id = esp_mqtt_client_publish(mqtt_client, mqtt_tx_topic, reply->data, reply->len, 0, 0);
        if (msg_id < 0)
        {
            ESP_LOGE(TAG, "Failed to publish: %d", msg_id);
        }
        else
        {
            ESP_LOGI(TAG, "Publish success: %d", reply->len);
        }
        return;
    }

    if (reply->len >= 100)
    {
        strcpy(&reply->data[100 - 4], "...");
    }

    ESP_LOGI(TAG, "response: %s\n", reply->data);
}

esp_err_t read_wifi_config(sscma_client_wifi_t *wifi)
{
    nvs_handle_t my_handle;
    esp_err_t ret = nvs_open(NVS_NAMESPACE, NVS_READONLY, &my_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error (%s) opening NVS handle!", esp_err_to_name(ret));
        return ret;
    }

    size_t required_size;
    ret = nvs_get_str(my_handle, NVS_KEY_SSID, NULL, &required_size);
    if (ret == ESP_OK) {
        wifi->ssid = malloc(required_size);
        ret = nvs_get_str(my_handle, NVS_KEY_SSID, wifi->ssid, &required_size);
    } else {
        wifi->ssid = NULL;
    }

    ret = nvs_get_str(my_handle, NVS_KEY_PASSWORD, NULL, &required_size);
    if (ret == ESP_OK) {
        wifi->password = malloc(required_size);
        ret = nvs_get_str(my_handle, NVS_KEY_PASSWORD, wifi->password, &required_size);
    } else {
        wifi->password = NULL;
    }

    nvs_close(my_handle);
    return ESP_OK;
}

esp_err_t read_mqtt_config(sscma_client_mqtt_t *mqtt)
{
    nvs_handle_t my_handle;
    esp_err_t ret = nvs_open(NVS_NAMESPACE, NVS_READONLY, &my_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error (%s) opening NVS handle!", esp_err_to_name(ret));
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

    //ret = nvs_get_str(my_handle, NVS_KEY_MQTT_PORT, NULL, &required_size);
    // if (ret == ESP_OK) {
    //     mqtt->port = malloc(required_size);
    //     ret = nvs_get_str(my_handle, NVS_KEY_MQTT_PORT, mqtt->port, &required_size);
    // } else {
    //     mqtt->port = NULL;
    // }

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
        reg[9] = 1000000000 + atoi(model->uuid) * 1000 + 101;
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
        //ESP_LOGI(TAG, "Port: %s", mqtt.port == NULL ? "NULL" : mqtt.port);
        ESP_LOGI(TAG, "Client ID: %s", mqtt.client_id == NULL ? "NULL" : mqtt.client_id);
        //ESP_LOGI(TAG, "Use SSL: %s", mqtt.use_ssl == NULL ? "NULL" : mqtt.use_ssl);
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
            //const char *port = mqtt.port ? mqtt.port : "null";
            const char *client_id = mqtt.client_id ? mqtt.client_id : "null";
            //const char *use_ssl = mqtt.use_ssl ? mqtt.use_ssl : "null";
            const int *port1 = mqtt.port1 ? mqtt.port1 : "null";
            const int *use_ssl1 = mqtt.use_ssl1;

            ret = nvs_set_str(my_handle, NVS_KEY_MQTT_USERNAME, username);
            ESP_ERROR_CHECK(ret);
            ESP_LOGI(TAG, "NVS handle opened successfully");
            ret = nvs_set_str(my_handle, NVS_KEY_MQTT_PASSWORD, password);
            ESP_ERROR_CHECK(ret);

            ret = nvs_set_str(my_handle, NVS_KEY_MQTT_ADDRESS, address);
            ESP_ERROR_CHECK(ret);

            // ret = nvs_set_str(my_handle, NVS_KEY_MQTT_PORT, port);
            // ESP_ERROR_CHECK(ret);

            ret = nvs_set_i32(my_handle, NVS_KEY_MQTT_PORT1, port1);
            ESP_ERROR_CHECK(ret);

            ret = nvs_set_str(my_handle, NVS_KEY_MQTT_CLIENT_ID, client_id);
            ESP_ERROR_CHECK(ret);

            // ret = nvs_set_str(my_handle, NVS_KEY_MQTT_USE_SSL, use_ssl);
            // ESP_ERROR_CHECK(ret);

            ret = nvs_set_i32(my_handle, NVS_KEY_MQTT_USE_SSL1, use_ssl1);
            ESP_ERROR_CHECK(ret);

            ret = nvs_commit(my_handle);
            ESP_ERROR_CHECK(ret);

            nvs_close(my_handle);
        }
    }
    else
    {
        ESP_LOGE(TAG, "get mqtt config failed\n");
    }

    sscma_client_invoke(client, -1, false, false);
}

static void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        if (s_retry_num < CONFIG_ESP_MAXIMUM_RETRY)
        {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        }
        else
        {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG, "connect to the AP fail");
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
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
            sscma_client_write(client, event->data, event->data_len);
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
            break;
        default:
            ESP_LOGI(TAG, "Other event id:%d", event->event_id);
            break;
    }
}

void wifi_init_sta()
{
    s_wifi_event_group = xEventGroupCreate();
    s_mqtt_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_t *netif = esp_netif_create_default_wifi_sta();
    assert(netif);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));

    sscma_client_wifi_t wifi;
    memset(&wifi, 0, sizeof(wifi));
    if (read_wifi_config(&wifi) == ESP_OK)
    {
        ESP_LOGI(TAG, "readSSID: %s", wifi.ssid == NULL ? "NULL" : wifi.ssid);
        ESP_LOGI(TAG, "Password: %s", wifi.password == NULL ? "NULL" : wifi.password);
    }

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = "",
            .password = ""
        },
    };
    strncpy((char *)wifi_config.sta.ssid, wifi.ssid, sizeof(wifi_config.sta.ssid) - 1);
    strncpy((char *)wifi_config.sta.password, wifi.password, sizeof(wifi_config.sta.password) - 1);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_ERROR_CHECK(esp_netif_set_hostname(netif, "A1102"));//tcpip name is A1102

    ESP_LOGI(TAG, "wifi_init_sta finished.");

    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT, pdFALSE, pdFALSE, portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT)
    {
        ESP_LOGI(TAG, "connected to ap SSID: %s", wifi.ssid);
    }
    else if (bits & WIFI_FAIL_BIT)
    {
        ESP_LOGI(TAG, "Failed to connect to SSID: %s", wifi.ssid);
    }
    else
    {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }
    vEventGroupDelete(s_wifi_event_group);
}

void mqtt_initialize()
{
    sscma_client_mqtt_t mqtt;
    memset(&mqtt, 0, sizeof(mqtt));
    if (read_mqtt_config(&mqtt) == ESP_OK)
    {
        ESP_LOGI(TAG, "Username: %s", mqtt.username == NULL ? "NULL" : mqtt.username);
        ESP_LOGI(TAG, "Password: %s", mqtt.password == NULL ? "NULL" : mqtt.password);
        ESP_LOGI(TAG, "Address: %s", mqtt.address == NULL ? "NULL" : mqtt.address);
        //ESP_LOGI(TAG, "Port: %s", mqtt.port == NULL ? "NULL" : mqtt.port);
        ESP_LOGI(TAG, "Client ID: %s", mqtt.client_id == NULL ? "NULL" : mqtt.client_id);
        //ESP_LOGI(TAG, "Use SSL: %s", mqtt.use_ssl == NULL ? "NULL" : mqtt.use_ssl);
        ESP_LOGI(TAG, "port: %d", mqtt.port1 == 0 ? 0 : mqtt.port1);
        ESP_LOGI(TAG, "read_use_ssl: %d", mqtt.use_ssl1);
    }

    uint8_t mac[8] = { 0 };
    esp_wifi_get_mac(ESP_IF_WIFI_STA, mac);

    snprintf(mqtt_client_id, sizeof(mqtt_client_id), "watcher-%02x%02x%02x%02x%02x%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    snprintf(mqtt_tx_topic, sizeof(mqtt_tx_topic), "sscma/v0/%s/tx", mqtt_client_id);
    snprintf(mqtt_rx_topic, sizeof(mqtt_rx_topic), "sscma/v0/%s/rx", mqtt_client_id);

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

esp_err_t write_modbus_address(uint16_t modbus_address)
{
    nvs_handle_t my_handle;
    esp_err_t ret;

    ret = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (ret != ESP_OK) {
        return ret;
    }

    ret = nvs_set_u16(my_handle, NVS_KEY_ADDR, modbus_address);
    if (ret != ESP_OK) {
        nvs_close(my_handle);
        return ret;
    }

    ret = nvs_commit(my_handle);
    if (ret != ESP_OK) {
        nvs_close(my_handle);
        return ret;
    }

    nvs_close(my_handle);

    return ESP_OK;
}

esp_err_t write_modbus_baud(uint32_t modbus_baud)
{
    nvs_handle_t my_handle;
    esp_err_t ret;

    ret = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (ret != ESP_OK) {
        return ret;
    }

    ret = nvs_set_u32(my_handle, NVS_KEY_BAUD, modbus_baud);
    if (ret != ESP_OK) {
        nvs_close(my_handle);
        return ret;
    }

    ret = nvs_commit(my_handle);
    if (ret != ESP_OK) {
        nvs_close(my_handle);
        return ret;
    }

    nvs_close(my_handle);

    return ESP_OK;
}

void nvs_modbus()
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    nvs_handle_t my_handle;
    ret = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &my_handle);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error (%s) opening NVS handle!", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "NVS handle opened successfully");
    }
    
    modbus_address = 1; 
    ret = nvs_get_u16(my_handle, NVS_KEY_ADDR, &modbus_address);
    if (ret == ESP_ERR_NVS_NOT_FOUND) {
        //modbus_address = 1;
        ret = nvs_set_u16(my_handle, NVS_KEY_ADDR, modbus_address);
        ESP_ERROR_CHECK(ret);
    } else {
        ESP_ERROR_CHECK(ret);
    }

    modbus_baud = 115200;
    ret = nvs_get_u16(my_handle, NVS_KEY_BAUD, &modbus_baud);
    if (ret == ESP_ERR_NVS_NOT_FOUND) {
        //modbus_baud = 115200;
        ret = nvs_set_u16(my_handle, NVS_KEY_BAUD, modbus_baud);
        ESP_ERROR_CHECK(ret);
    } else {
        ESP_ERROR_CHECK(ret);
    }

    ret = nvs_commit(my_handle);
    ESP_ERROR_CHECK(ret);
    
}

// uint16_t read_modbus_value(uint16_t reg_address)
// {
//     uint16_t modbus_value = 0;
//     mb_param_info_t reg_info;
//     esp_err_t err;

//     err = mbc_slave_get_param_info(&reg_info, 1000); 
//     if (err == ESP_OK) {

//         if (reg_info.type == MB_PARAM_HOLDING && reg_info.start_offset == reg_address) {
//             modbus_value = *(uint16_t*)reg_info.address;
//             ESP_LOGI(TAG, "Modbus value at 0x%04X: %d", reg_address, modbus_value);
//         } else {
//             ESP_LOGE(TAG, "Mismatched register type or address");
//         }
//     } else {
//         ESP_LOGE(TAG, "Failed to read Modbus value at 0x%04X, error: %d", reg_address, err);
//         modbus_value = 0; 
//     }

//     return modbus_value;
// }

void app_main(void)
{
        esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    //nvs_modbus();
    wifi_init_sta();
    mqtt_initialize();
    
    ESP_LOGI("main", "start");
    mbc_slave_init(MB_PORT_SERIAL_SLAVE, &mbc_slave_handler);

    comm_info.mode = MB_MODE_RTU;
    comm_info.slave_addr = 1;//modbus_address;//modbus address
    comm_info.port = UART_NUM_0; // uart 0
    comm_info.baudrate = 115200;//modbus_baud;
    comm_info.parity = MB_PARITY_NONE;
    ESP_ERROR_CHECK(mbc_slave_setup((void *)&comm_info));

    // 设置保持寄存器区域
    mb_register_area_descriptor_t holding_reg_area;
    for (int i = 0; i < 9; i++)
    {
        holding_reg_area.type = MB_PARAM_HOLDING;
        holding_reg_area.start_offset = 0x1000 + i * 2;
        holding_reg_area.address = (void *)&(reg[i]);
        holding_reg_area.size = sizeof(int32_t);
        ESP_ERROR_CHECK(mbc_slave_set_descriptor(holding_reg_area));
    }

    holding_reg_area.type = MB_PARAM_HOLDING;
    holding_reg_area.start_offset = 0x8002;
    holding_reg_area.address = (void *)&(reg[9]);
    holding_reg_area.size = sizeof(int32_t);
    ESP_ERROR_CHECK(mbc_slave_set_descriptor(holding_reg_area));

    //reg[10] = 22;
    holding_reg_area.type = MB_PARAM_HOLDING;//modbus地址
    holding_reg_area.start_offset = 0x0000;
    holding_reg_area.address = (void *)&(modbus_address);
    holding_reg_area.size = sizeof(uint16_t);
    ESP_ERROR_CHECK(mbc_slave_set_descriptor(holding_reg_area));
    
    holding_reg_area.type = MB_PARAM_HOLDING;//波特率
    holding_reg_area.start_offset = 0x0001;
    holding_reg_area.address = (void *)&(modbus_baud);
    holding_reg_area.size = sizeof(uint32_t);
    ESP_ERROR_CHECK(mbc_slave_set_descriptor(holding_reg_area));    

    // holding_reg_area.type = MB_PARAM_HOLDING;//设备版本
    // holding_reg_area.start_offset = 0x0001;
    // holding_reg_area.address = (void *)&(reg[11]);
    // holding_reg_area.size = sizeof(int32_t);
    // ESP_ERROR_CHECK(mbc_slave_set_descriptor(holding_reg_area));  

    ESP_ERROR_CHECK(mbc_slave_start());

    ESP_ERROR_CHECK(uart_set_pin(UART_NUM_0, 6, 7, 4, UART_PIN_NO_CHANGE));
    ESP_ERROR_CHECK(uart_set_mode(UART_NUM_0, UART_MODE_RS485_HALF_DUPLEX));

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
    sscma_client_config.reset_gpio_num = GPIO_NUM_5;

    sscma_client_new_io_uart_bus((sscma_client_uart_bus_handle_t)1, &io_uart_config, &io);

    sscma_client_new(io, &sscma_client_config, &client);

    const sscma_client_callback_t callback = {
        .on_connect = on_connect,
        .on_event = on_event,
        .on_log = on_log,
        .on_response = on_response,
    };

    if (sscma_client_register_callback(client, &callback, NULL) != ESP_OK)
    {
        printf("set callback failed\n");
        abort();
    }

    sscma_client_init(client);

    for (int i = 0; i < 9; i++)
    {
        reg[i] = -1000;
    }
    ESP_LOGI("MB", "Listening for events...");
    while (1)
    {
        (void)mbc_slave_check_event(MB_READ_WRITE_MASK);
        ESP_ERROR_CHECK_WITHOUT_ABORT(mbc_slave_get_param_info(&reg_info, MB_PAR_INFO_GET_TOUT));
        if (reg_info.type & (MB_EVENT_HOLDING_REG_WR | MB_EVENT_HOLDING_REG_RD))
        {
            // Get parameter information from parameter queue
            ESP_LOGI(TAG, "HOLDING  (%" PRIu32 " us), ADDR:%u, TYPE:%u, INST_ADDR:0x%" PRIx32 ", SIZE:%u", reg_info.time_stamp, (unsigned)reg_info.mb_offset, (unsigned)reg_info.type,
                (uint32_t)reg_info.address, (unsigned)reg_info.size);
            if (reg_info.mb_offset == 0x1000)
            {
                sscma_client_invoke(client, -1, false, false);
            }
            else if (reg_info.mb_offset == 0x8002)
            {
                for (int i = 0; i < 9; i++)
                {
                    reg[i] = -1000;
                }
            }
            // else if (reg_info.mb_offset == 0x0000)
            // {
            //      ESP_LOGI(TAG, "HOLDING  (%" PRIu32 " us), ADDR:%u, TYPE:%u, INST_ADDR:0x%" PRIx32 ", SIZE:%u", reg_info.time_stamp, (unsigned)reg_info.mb_offset, (unsigned)reg_info.type,(uint32_t)reg_info.address, (unsigned)reg_info.size);

            //     uint16_t new_modbus_addr =  read_modbus_value(0x0000);
            //     esp_err_t result = write_modbus_baud(new_modbus_addr);
            //     if (result == ESP_OK) {
            //         ESP_LOGI(TAG, "Modbus baud rate written successfully");
            //     } else {
            //         ESP_LOGE(TAG, "Failed to write Modbus baud rate");
            //     }
                 
            // }
            // else if (reg_info.mb_offset == 0x0001)
            // {
            //      ESP_LOGI(TAG, "HOLDING  (%" PRIu32 " us), ADDR:%u, TYPE:%u, INST_ADDR:0x%" PRIx32 ", SIZE:%u", reg_info.time_stamp, (unsigned)reg_info.mb_offset, (unsigned)reg_info.type,(uint32_t)reg_info.address, (unsigned)reg_info.size);

            //     uint16_t new_modbus_baud =  read_modbus_value(0x0001);
            //     esp_err_t result = write_modbus_baud(new_modbus_baud);
            //     if (result == ESP_OK) {
            //         ESP_LOGI(TAG, "Modbus baud rate written successfully");
            //     } else {
            //         ESP_LOGE(TAG, "Failed to write Modbus baud rate");
            //     }
                 
            // }            
        }
    }
}
