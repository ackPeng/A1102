#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "esp_event.h"
#include <string.h>
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include <stdio.h>
#include <stdint.h>
#include "cJSON.h"

#include "mbcontroller.h"
#include <stdio.h>
#include <stdint.h>
#include "esp_err.h"
#include "esp_log.h"




///////////////////////////////////////////

// extern int g_type;
// extern char g_name[50];
// extern int g_code;
// extern int g_model_id, g_model_type, g_model_address, g_model_size;
// extern int g_algo_type, g_algo_category, g_algo_input_from, g_tscore, g_tiou;
// extern int g_sensor_id, g_sensor_type, g_sensor_state, g_sensor_opt_id;
// extern char g_sensor_opt_detail[50];
// extern int g_count;
// extern int g_perf[10]; 
// extern int g_boxes[10][10]; 
// extern int g_resolution[10];

// globals.c
int g_type;
char g_name[50];
int g_code;
int g_model_id, g_model_type, g_model_address, g_model_size;
int g_algo_type, g_algo_category, g_algo_input_from, g_tscore, g_tiou;
int g_sensor_id, g_sensor_type, g_sensor_state, g_sensor_opt_id;
char g_sensor_opt_detail[50];
int g_count;
int g_perf[10];
int g_boxes[10][10];
int g_resolution[10];


// int sendData_MB(const char* data) {
//     const int len = strlen(data);
//     const int txBytes = uart_write_bytes(UART_MB_NUM, data, len);
//     //ESP_LOGI(TAG, "Wrote %d bytes", txBytes);
//     return txBytes;
// }

void parse_and_log_json(const char *json_str)
{
    cJSON *root = cJSON_Parse(json_str);
    if (root == NULL) {
        return;
    }

    cJSON *type = cJSON_GetObjectItem(root, "type");
    if (cJSON_IsNumber(type)) {
        g_type = type->valueint;
    }

    cJSON *name = cJSON_GetObjectItem(root, "name");
    if (cJSON_IsString(name)) {
        strncpy(g_name, name->valuestring, sizeof(g_name) - 1);
        g_name[sizeof(g_name) - 1] = '\0';
    }

    cJSON *code = cJSON_GetObjectItem(root, "code");
    if (cJSON_IsNumber(code)) {
        g_code = code->valueint;
    }

    cJSON *data = cJSON_GetObjectItem(root, "data");
    if (cJSON_IsObject(data)) {
        cJSON *model = cJSON_GetObjectItem(data, "model");
        if (cJSON_IsObject(model)) {
            cJSON *model_id = cJSON_GetObjectItem(model, "id");
            cJSON *model_type = cJSON_GetObjectItem(model, "type");
            cJSON *model_address = cJSON_GetObjectItem(model, "address");
            cJSON *model_size = cJSON_GetObjectItem(model, "size");
            if (cJSON_IsNumber(model_id)) g_model_id = model_id->valueint;
            if (cJSON_IsNumber(model_type)) g_model_type = model_type->valueint;
            if (cJSON_IsNumber(model_address)) g_model_address = model_address->valueint;
            if (cJSON_IsNumber(model_size)) g_model_size = model_size->valueint;
        }

        cJSON *algorithm = cJSON_GetObjectItem(data, "algorithm");
        if (cJSON_IsObject(algorithm)) {
            cJSON *algo_type = cJSON_GetObjectItem(algorithm, "type");
            cJSON *algo_category = cJSON_GetObjectItem(algorithm, "categroy");
            cJSON *algo_input_from = cJSON_GetObjectItem(algorithm, "input_from");
            cJSON *algo_config = cJSON_GetObjectItem(algorithm, "config");
            if (cJSON_IsObject(algo_config)) {
                cJSON *tscore = cJSON_GetObjectItem(algo_config, "tscore");
                cJSON *tiou = cJSON_GetObjectItem(algo_config, "tiou");
                if (cJSON_IsNumber(algo_type)) g_algo_type = algo_type->valueint;
                if (cJSON_IsNumber(algo_category)) g_algo_category = algo_category->valueint;
                if (cJSON_IsNumber(algo_input_from)) g_algo_input_from = algo_input_from->valueint;
                if (cJSON_IsNumber(tscore)) g_tscore = tscore->valueint;
                if (cJSON_IsNumber(tiou)) g_tiou = tiou->valueint;
            }
        }

        cJSON *sensor = cJSON_GetObjectItem(data, "sensor");
        if (cJSON_IsObject(sensor)) {
            cJSON *sensor_id = cJSON_GetObjectItem(sensor, "id");
            cJSON *sensor_type = cJSON_GetObjectItem(sensor, "type");
            cJSON *sensor_state = cJSON_GetObjectItem(sensor, "state");
            cJSON *sensor_opt_id = cJSON_GetObjectItem(sensor, "opt_id");
            cJSON *sensor_opt_detail = cJSON_GetObjectItem(sensor, "opt_detail");
            if (cJSON_IsNumber(sensor_id)) g_sensor_id = sensor_id->valueint;
            if (cJSON_IsNumber(sensor_type)) g_sensor_type = sensor_type->valueint;
            if (cJSON_IsNumber(sensor_state)) g_sensor_state = sensor_state->valueint;
            if (cJSON_IsNumber(sensor_opt_id)) g_sensor_opt_id = sensor_opt_id->valueint;
            if (cJSON_IsString(sensor_opt_detail)) {
                strncpy(g_sensor_opt_detail, sensor_opt_detail->valuestring, sizeof(g_sensor_opt_detail) - 1);
                g_sensor_opt_detail[sizeof(g_sensor_opt_detail) - 1] = '\0';
            }
        }

        cJSON *count = cJSON_GetObjectItem(data, "count");
        if (cJSON_IsNumber(count)) {
            g_count = count->valueint;
        }

        cJSON *perf = cJSON_GetObjectItem(data, "perf");
        if (cJSON_IsArray(perf)) {
            int perf_size = cJSON_GetArraySize(perf);
            for (int i = 0; i < perf_size && i < 10; i++) { 
                cJSON *perf_item = cJSON_GetArrayItem(perf, i);
                if (cJSON_IsNumber(perf_item)) {
                    g_perf[i] = perf_item->valueint;
                }
            }
        }

        cJSON *boxes = cJSON_GetObjectItem(data, "boxes");
        if (cJSON_IsArray(boxes)) {
            int boxes_size = cJSON_GetArraySize(boxes);
            for (int i = 0; i < boxes_size && i < 10; i++) { 
                cJSON *box = cJSON_GetArrayItem(boxes, i);
                if (cJSON_IsArray(box)) {
                    int box_size = cJSON_GetArraySize(box);
                    for (int j = 0; j < box_size && j < 10; j++) { 
                        cJSON *box_item = cJSON_GetArrayItem(box, j);
                        if (cJSON_IsNumber(box_item)) {
                            g_boxes[i][j] = box_item->valueint;
                        }
                    }
                }
            }
        }

        cJSON *resolution = cJSON_GetObjectItem(data, "resolution");
        if (cJSON_IsArray(resolution)) {
            int resolution_size = cJSON_GetArraySize(resolution);
            for (int i = 0; i < resolution_size && i < 10; i++) { 
                cJSON *resolution_item = cJSON_GetArrayItem(resolution, i);
                if (cJSON_IsNumber(resolution_item)) {
                    g_resolution[i] = resolution_item->valueint;
                }
            }
        }
    } 
    vTaskDelay(pdMS_TO_TICKS(300));
    //send_parsed_json_data();
}



#define UART_NUM UART_NUM_1
#define TXD_PIN  21
#define RXD_PIN  20
#define BAUD_RATE 921600

void init_uart() {
    const uart_config_t uart_config = {
        .baud_rate = BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };
    uart_param_config(UART_NUM, &uart_config);
    uart_set_pin(UART_NUM, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(UART_NUM, 1024 * 2, 0, 0, NULL, 0);
}

void send_at_command() {
    
    const char* at_command = "AT+INVOKE=1,0,1\r";//
    const char* at_command_end = "AT+INVOKE=1,0,1\r@AT+ACTION=((max_score(target,0)>=20)&&save_jpeg()\r";//
    
    uart_write_bytes(UART_NUM, at_command, strlen(at_command));
    vTaskDelay(pdMS_TO_TICKS(700)); 
    uart_write_bytes(UART_NUM, at_command_end, strlen(at_command_end));
    vTaskDelay(pdMS_TO_TICKS(700)); 
    uart_write_bytes(UART_NUM, at_command_end, strlen(at_command_end));
    //ESP_LOGI("UART", "Sent: %s", at_command);
    uint8_t data[1024];
    int length = uart_read_bytes(UART_NUM, data, sizeof(data) - 1, 100 / portTICK_PERIOD_MS);
    if (length > 0) {
        //parse_json(data);
        data[length] = '\0'; // Null-terminate the string
        //ESP_LOGI("UART", "Received: %s", data);
         char *token = strtok((char *)data, "\n");
            while (token != NULL) {
        parse_and_log_json(token);
        token = strtok(NULL, "\n");
        vTaskDelay(pdMS_TO_TICKS(300));
        //himax_read++;
        // send_parsed_json_data();
    }
    
    }
}




#define MB_READ_MASK (MB_EVENT_INPUT_REG_RD | MB_EVENT_HOLDING_REG_RD | MB_EVENT_DISCRETE_RD | MB_EVENT_COILS_RD)
#define MB_WRITE_MASK (MB_EVENT_HOLDING_REG_WR | MB_EVENT_COILS_WR)
#define MB_READ_WRITE_MASK (MB_READ_MASK | MB_WRITE_MASK)

mb_communication_info_t comm_info;
mb_param_info_t reg_info;
//static const char *TAG = "MODBUS";

typedef struct {
    uint16_t device_address; // 设备地址
    uint16_t baud_rate;      // 波特率
    uint32_t device_version; // 设备版本
    float device_id;      // 设备ID
    uint32_t model_id;       // 模型ID
    int32_t result[9];       // 结果1-9
    int32_t coordinates;     // 坐标信息
    int32_t area_info;       // 区域信息
} holding_reg_params_t;

holding_reg_params_t holding_reg_params = {
    .device_address = 1,
    .baud_rate = 96,
    .device_version = 20304, // 示例版本号
    .device_id = 4174101,      // 示例设备ID
    .model_id = 4321,       // 示例模型ID
    .result = {1800, 1800, 1800, 1800, 1800, 1800, 1800, 1800, 1800},// 示例结果
    .coordinates = 12340,// 示例坐标信息
    .area_info = 34560// 示例区域信息
};

void modbus_data() 
{
    holding_reg_params.device_address = 1;
    holding_reg_params.baud_rate = 96;
    holding_reg_params.device_version = g_sensor_opt_id;
    holding_reg_params.device_id = g_sensor_id;
    holding_reg_params.model_id = g_model_id;
    holding_reg_params.result[0] = g_boxes[0][4] * 100 + g_boxes[0][5] * 10;
    holding_reg_params.coordinates = g_boxes[0][0] * 100 + g_boxes[0][1];
    holding_reg_params.area_info = g_boxes[0][2] * 100 + g_boxes[0][3];
}

void modbus_init(void) {
    //modbus_data();
    void *mbc_slave_handler = NULL;
    mbc_slave_init(MB_PORT_SERIAL_SLAVE, &mbc_slave_handler);

    comm_info.mode = MB_MODE_RTU;
    comm_info.slave_addr = 1;
    comm_info.port = UART_NUM_0; // uart 0
    comm_info.baudrate = 115200;
    comm_info.parity = MB_PARITY_NONE;
    ESP_ERROR_CHECK(mbc_slave_setup((void *)&comm_info));

    // 设置保持寄存器区域
    mb_register_area_descriptor_t holding_reg_area;

    // 设备地址
    holding_reg_area.type = MB_PARAM_HOLDING;
    holding_reg_area.start_offset = 0x0000;
    holding_reg_area.address = (void *)&holding_reg_params.device_address;
    holding_reg_area.size = sizeof(holding_reg_params.device_address);
    ESP_ERROR_CHECK(mbc_slave_set_descriptor(holding_reg_area));

    // 波特率
    holding_reg_area.start_offset = 0x0001;
    holding_reg_area.address = (void *)&holding_reg_params.baud_rate;
    holding_reg_area.size = sizeof(holding_reg_params.baud_rate);
    ESP_ERROR_CHECK(mbc_slave_set_descriptor(holding_reg_area));

    // 设备版本
    holding_reg_area.start_offset = 0x0002;
    holding_reg_area.address = (void *)&holding_reg_params.device_version;
    holding_reg_area.size = sizeof(holding_reg_params.device_version);
    ESP_ERROR_CHECK(mbc_slave_set_descriptor(holding_reg_area));

    // 设备ID
    holding_reg_area.start_offset = 0x8000;
    holding_reg_area.address = (void *)&holding_reg_params.device_id;
    holding_reg_area.size = sizeof(holding_reg_params.device_id);
    ESP_ERROR_CHECK(mbc_slave_set_descriptor(holding_reg_area));

    // 模型ID
    holding_reg_area.start_offset = 0x8002;
    holding_reg_area.address = (void *)&holding_reg_params.model_id;
    holding_reg_area.size = sizeof(holding_reg_params.model_id);
    ESP_ERROR_CHECK(mbc_slave_set_descriptor(holding_reg_area));

    // 结果1-9
    for (int i = 0; i < 9; i++) {
        holding_reg_area.start_offset = 0x1000 + i * 2;
        holding_reg_area.address = (void *)&holding_reg_params.result[i];
        holding_reg_area.size = sizeof(holding_reg_params.result[i]);
        ESP_ERROR_CHECK(mbc_slave_set_descriptor(holding_reg_area));
    }


    // 坐标信息
    holding_reg_area.start_offset = 0x2000;
    holding_reg_area.address = (void *)&holding_reg_params.coordinates;
    holding_reg_area.size = sizeof(holding_reg_params.coordinates);
    ESP_ERROR_CHECK(mbc_slave_set_descriptor(holding_reg_area));

    // 区域信息
    holding_reg_area.start_offset = 0x3000;
    holding_reg_area.address = (void *)&holding_reg_params.area_info;
    holding_reg_area.size = sizeof(holding_reg_params.area_info);
    ESP_ERROR_CHECK(mbc_slave_set_descriptor(holding_reg_area));

    ESP_ERROR_CHECK(mbc_slave_start());

    ESP_ERROR_CHECK(uart_set_pin(UART_NUM_0, 6, 7, 4,UART_PIN_NO_CHANGE));
    ESP_ERROR_CHECK(uart_set_mode(UART_NUM_0,  UART_MODE_RS485_HALF_DUPLEX));

    while (1) {
        
        mb_event_group_t event = mbc_slave_check_event(MB_READ_WRITE_MASK);
        if (event & MB_EVENT_HOLDING_REG_RD) 
        {

        } 
        else if (event & MB_EVENT_HOLDING_REG_WR) 
        {

        }
        
    }

    ESP_ERROR_CHECK(mbc_slave_destroy());
}


void app_main() 
{
    init_uart();
    send_at_command();
    modbus_init();
}





