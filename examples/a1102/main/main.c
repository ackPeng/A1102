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

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_check.h"

#include "mbcontroller.h"

#include "sscma_client_io.h"
#include "sscma_client_ops.h"

#include "sscma_client_commands.h"

#define TAG "SenseCAP A1102"

static sscma_client_io_handle_t io = NULL;
static sscma_client_handle_t client = NULL;
static void *mbc_slave_handler = NULL;
mb_communication_info_t comm_info;
mb_param_info_t reg_info;
volatile bool is_captured = false;

#define MB_PAR_INFO_GET_TOUT (10) // Timeout for get parameter info
#define MB_READ_MASK         (MB_EVENT_INPUT_REG_RD | MB_EVENT_HOLDING_REG_RD | MB_EVENT_DISCRETE_RD | MB_EVENT_COILS_RD)
#define MB_WRITE_MASK        (MB_EVENT_HOLDING_REG_WR | MB_EVENT_COILS_WR)
#define MB_READ_WRITE_MASK   (MB_READ_MASK | MB_WRITE_MASK)

volatile int32_t reg[10] = { 0 };
float magic = 0;

void on_event(sscma_client_handle_t client, const sscma_client_reply_t *reply, void *user_ctx)
{
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

void on_log(sscma_client_handle_t client, const sscma_client_reply_t *reply, void *user_ctx) { }

void on_connect(sscma_client_handle_t client, const sscma_client_reply_t *reply, void *user_ctx)
{
    sscma_client_info_t *info;
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
    sscma_client_model_t *model;
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
    else
    {
        ESP_LOGE("ai", "get model failed\n");
    }
    sscma_client_invoke(client, -1, false, false);
}

void app_main(void)
{
    ESP_LOGI("main", "start");
    mbc_slave_init(MB_PORT_SERIAL_SLAVE, &mbc_slave_handler);

    comm_info.mode = MB_MODE_RTU;
    comm_info.slave_addr = 1;
    comm_info.port = UART_NUM_0; // uart 0
    comm_info.baudrate = 115200;
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
            // // Get parameter information from parameter queue
            // ESP_LOGI(TAG, "HOLDING  (%" PRIu32 " us), ADDR:%u, TYPE:%u, INST_ADDR:0x%" PRIx32 ", SIZE:%u", reg_info.time_stamp, (unsigned)reg_info.mb_offset, (unsigned)reg_info.type,
            //     (uint32_t)reg_info.address, (unsigned)reg_info.size);
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
        }
    }
}
