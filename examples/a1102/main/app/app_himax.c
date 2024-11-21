#include "app_himax.h"
#include "app_modbus.h"
// #include "app_mqtt.h"
#include "cJSON.h"
#include "cJSON_Utils.h"

static const char *TAG = "app_himax";


extern a1102_holding_reg_params_t a1102_msg;
extern holding_image_params modbus_image;
extern bool master_is_reading_flag;


void himax_free_image(struct himax_data_image *p_data){
    p_data->len  = 0;
    p_data->time  = 0;
    if( p_data->p_buf != NULL) {
        free(p_data->p_buf);
    }
    p_data->p_buf = NULL;
}

void himax_data_inference_free(struct himax_data_inference_info *p_inference){
    if( p_inference->p_data != NULL) {
        free(p_inference->p_data);
        
    }
    p_inference->p_data   = NULL;

    // for (int i = 0; p_inference->classes[i] != NULL && i < CONFIG_MODEL_CLASSES_MAX_NUM; i++)
    // {
    //     free(p_inference->classes[i]);
    //     p_inference->classes[i] = NULL;
    // }
    p_inference->cnt      = 0;
    p_inference->is_valid = false;
    p_inference->type     = INFERENCE_TYPE_UNKNOWN;

}


void refresh_modbus_rigester(struct himax_module_ai_camera_preview_info *info){
    portENTER_CRITICAL(&param_lock);

    if(info->inference.type == INFERENCE_TYPE_BOX){
        sscma_client_box_t     *boxes = (sscma_client_box_t *)info->inference.p_data;
        for (int i = 0; i < info->inference.cnt && i < 9; i++)
        {
            mb_set_uint32_abcd((val_32_arr *)&a1102_msg.result[i], boxes[i].target * 1000 + boxes[i].score * 10);
            mb_set_uint32_abcd((val_32_arr *)&a1102_msg.xy[i], boxes[i].x * 1000 + boxes[i].y);
            mb_set_uint32_abcd((val_32_arr *)&a1102_msg.wh[i], boxes[i].w * 1000 + boxes[i].h);
        }
    }else if (info->inference.type == INFERENCE_TYPE_CLASS){
        sscma_client_class_t *classes = (sscma_client_class_t *)info->inference.p_data;
        for (int i = 0; i < info->inference.cnt && i < 9; i++){
            mb_set_uint32_abcd((val_32_arr *)&a1102_msg.result[i], classes[i].target * 1000 + classes[i].score * 10);
        }
        
    }else if (info->inference.type == INFERENCE_TYPE_POINT){
        sscma_client_point_t  *points = (sscma_client_point_t *)info->inference.p_data;
        for (int i = 0; i < info->inference.cnt && i < 9; i++){
            mb_set_uint32_abcd((val_32_arr *)&a1102_msg.result[i], points[i].target * 1000 + points[i].score * 10);
        }
    }
    
    portEXIT_CRITICAL(&param_lock);

}



void on_event(sscma_client_handle_t client, const sscma_client_reply_t *reply, void *user_ctx)
{

    esp_err_t ret = ESP_OK;
 
    struct himax_module_ai_camera_preview_info info;
    sscma_client_class_t *classes = NULL;
    sscma_client_box_t     *boxes = NULL;
    sscma_client_point_t  *points = NULL;

    int class_count = 0;
    int box_count = 0;
    int point_count = 0;
    char *img = NULL;
    int img_size = 0;

    info.img.p_buf = NULL;
    info.img.len = 0;

    info.inference.cnt = 0;
    info.inference.is_valid = false;
    info.inference.p_data = NULL;


    if ( sscma_utils_fetch_image_from_reply(reply, &img, &img_size) == ESP_OK ) {
        info.img.p_buf = (uint8_t *)img;
        info.img.len = img_size;
        info.img.time = time(NULL);
        ESP_LOGD(TAG, "Small img:%.1fk (%d), time: %ld", (float)img_size/1024, img_size, (long)time(NULL));

        if(img_size > sizeof(modbus_image.image)){
            ESP_LOGI("event", "image_size > sizeof(modbus_image.image)\r\n");  
        }else{
  
            if(master_is_reading_flag == false){
                portENTER_CRITICAL(&param_lock);
                if(img_size & 1){
                    modbus_image.reg_length = img_size/2 + 1;
                }else{
                    modbus_image.reg_length = img_size/2;
                }
                memcpy(modbus_image.image,img,img_size);
                portEXIT_CRITICAL(&param_lock);
                ESP_LOGI("event","memcpy img_size %d finish !! register size is %d !!\r\n",img_size,modbus_image.reg_length);
            }
        }
    }

    if (sscma_utils_fetch_boxes_from_reply(reply, &boxes, &box_count) == ESP_OK) {
        info.inference.type = INFERENCE_TYPE_BOX;
        info.inference.p_data = (void *)boxes;
        info.inference.cnt = box_count;
        if (box_count > 0) {
            for (int i = 0; i < box_count; i++) {
                ESP_LOGD(TAG, "[box %d]: x=%d, y=%d, w=%d, h=%d, score=%d, target=%d", i,  \
                        boxes[i].x, boxes[i].y, boxes[i].w, boxes[i].h, boxes[i].score, boxes[i].target);
            }
        }
    }else if (sscma_utils_fetch_classes_from_reply(reply, &classes, &class_count) == ESP_OK) {
        info.inference.type = INFERENCE_TYPE_CLASS;
        info.inference.p_data = (void *)classes;
        info.inference.cnt = class_count;
        if (class_count > 0) {
            for (int i = 0; i < class_count; i++) {
                ESP_LOGD(TAG, "[class %d]: target=%d, score=%d", i, classes[i].target, classes[i].score);
            }
        }
    }else if (sscma_utils_fetch_points_from_reply(reply, &points, &point_count) == ESP_OK ) {
        info.inference.type = INFERENCE_TYPE_POINT;
        info.inference.p_data = (void *)points;
        info.inference.cnt = point_count;
        if (point_count > 0) {
            for (int i = 0; i < point_count; i++) {
                ESP_LOGD(TAG, "[point %d]: x=%d, y=%d, z=%d, score=%d, target=%d", i, points[i].x, points[i].y, points[i].z, points[i].score, points[i].target);
            }
        }
    }else{
        ESP_LOGI(TAG," Unknown type ");
    }


    refresh_modbus_rigester(&info);

    himax_free_image(&info.img);
    himax_data_inference_free(&info.inference);
    


    // sscma_client_invoke(client, -1, false, false);

}



void on_connect(sscma_client_handle_t client, const sscma_client_reply_t *reply, void *user_ctx)
{

    // sscma_client_box_t *boxes = NULL;
    // int box_count = 0;
    // if (sscma_utils_fetch_boxes_from_reply(reply, &boxes, &box_count) == ESP_OK)
    // {
    //     if (box_count > 0)
    //     {
    //         portENTER_CRITICAL(&param_lock);
    //         for (int i = 0; i < box_count && i < 9; i++)
    //         {
    //             temp_data[i] = boxes[i].target * 1000 + boxes[i].score * 10;
    //              ESP_LOGI("event", "target: %d, score: %d", boxes[i].target, boxes[i].score);
    //         }
    //         portEXIT_CRITICAL(&param_lock);
            
    //     }
    //     free(boxes);
    // }

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
            portENTER_CRITICAL(&param_lock);
            mb_set_uint32_abcd((val_32_arr *)&a1102_msg.modle_id, (1000000000 + atoi(model->uuid) * 1000 + 140));
            portEXIT_CRITICAL(&param_lock);
            printf("atoi(model->uuid) * 1000 = : %d\n", atoi(model->uuid) * 1000);
            printf("1000000000 + atoi(model->uuid) * 1000 + 140 : %d\n", (1000000000 + atoi(model->uuid) * 1000 + 140));
        }else{
            printf("model->uuid == NULL\r\n");
        }

        

        printf("Name: %s\n", model->name ? model->name : "N/A");
        g_a1102_param.modle_p.model_name = model->name;
        printf("Version: %s\n", model->ver ? model->ver : "N/A");
        printf("URL: %s\n", model->url ? model->url : "N/A");
        printf("Checksum: %s\n", model->checksum ? model->checksum : "N/A");
        printf("Classes:\n");
        if (model->classes[0] != NULL)
        {
            g_a1102_param.modle_p.classes_count = 0;
            for (int i = 0; model->classes[i] != NULL; i++)
            {
                g_a1102_param.modle_p.classes[i] = model->classes[i];
                g_a1102_param.modle_p.classes_count++;
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

    


}






void set_modle(int modle_number){
    ESP_LOGI(TAG,"change modle to %d",modle_number);

    sscma_client_set_model(client,modle_number);
    vTaskDelay(500);
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
            portENTER_CRITICAL(&param_lock);
            mb_set_uint32_abcd((val_32_arr *)&a1102_msg.modle_id, (1000000000 + atoi(model->uuid) * 1000 + 140));
            portEXIT_CRITICAL(&param_lock);
            printf("atoi(model->uuid) * 1000 = : %d\n", atoi(model->uuid) * 1000);
            printf("1000000000 + atoi(model->uuid) * 1000 + 140 : %d\n", (1000000000 + atoi(model->uuid) * 1000 + 140));
        }else{
            printf("model->uuid == NULL\r\n");
        }

        printf("Name: %s\n", model->name ? model->name : "N/A");
        g_a1102_param.modle_p.model_name = model->name;
        printf("Version: %s\n", model->ver ? model->ver : "N/A");
        printf("URL: %s\n", model->url ? model->url : "N/A");
        printf("Checksum: %s\n", model->checksum ? model->checksum : "N/A");
        printf("Classes:\n");
        if (model->classes[0] != NULL)
        {
            g_a1102_param.modle_p.classes_count = 0;
            for (int i = 0; model->classes[i] != NULL; i++)
            {
                g_a1102_param.modle_p.classes[i] = model->classes[i];
                g_a1102_param.modle_p.classes_count++;
                printf("  - %s\n", model->classes[i]);
            }
        }
        else
        {
            printf("  N/A\n");
        }
    }

}

int set_modle_confidence(int count, ClassData *data){
    int buffer_size = 50 * count + 50;
    char *command = (char *)malloc(buffer_size);
    if (!command) {
        printf("Memory allocation failed.\n");
        return -1;
    }

    strcpy(command, "AT+ACTION=\"(");
    for (int i = 0; i < count; i++) {
        char condition[50];
        sprintf(condition, "(max_score(target,%d)>=%d)", i, data->classes[i].confidence);
        strcat(command, condition);
        if (i < count - 1) {
            strcat(command, "||");
        }
    }

    strcat(command, ")");
    strcat(command, "&&save_jpeg()\"\r\n");
    if (command) {
        printf("%s", command);
        // sscma_client_write(client,command, 15);
        sscma_client_reply_t reply;
        ESP_RETURN_ON_ERROR(sscma_client_request(client,command, &reply, true, CMD_WAIT_DELAY), TAG, "confidence set failed");

        if (reply.payload != NULL)
        {
            cJSON *data = cJSON_GetObjectItem(reply.payload, "data");
            if (data != NULL) {
                char *data_string = cJSON_Print(data);

                if (data_string != NULL) {
                    printf("Data as string: %s\n", data_string);

                    free(data_string);
                } else {
                    printf("Failed to convert 'data' to string.\n");
                }
            } else {
                printf("'data' is NULL or not found in the JSON object.\n");
            }
            sscma_client_reply_clear(&reply);
        }
        free(command);
    }

    if (data->classes != NULL) {
        free(data->classes);
        data->classes = NULL;
    }
    data->class_count = 0;
    
    return 1;
}


void get_model_info(){
    ESP_LOGI(TAG,"get_model_info");
    char at_command[20];
    sprintf(at_command, "AT+INFO?\r\n");
    printf("get_model_info Command: %s\n", at_command);
    sscma_client_write(client,at_command, 15);
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


    
    // sscma_client_invoke(client, -1, false, false);
    vTaskDelay(2000);
    // ESP_LOGI(TAG,"init get pic\n");
    // sscma_client_write(client,"AT+INVOKE=1,0,0", 15);
    // vTaskDelay(500);
    sscma_client_invoke(client, -1, false, false);

    // ESP_LOGI(TAG,"init get pic ok !\n");

}