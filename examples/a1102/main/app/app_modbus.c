#include "app_modbus.h"

holding_image_params modbus_image = {0};
a1102_holding_reg_params_t a1102_msg = {0};
int modbus_reset_flage = 0;
mb_param_info_t reg_info; // keeps the Modbus registers access information
mb_communication_info_t comm_info; // Modbus communication parameters
mb_register_area_descriptor_t reg_area; // Modbus register area descriptor structure

uint32_t temp_data[10];
uint32_t temp_data2[18];

static const char *TAG = "app_modbus";

// 寄存器内容初始化
static void init_a1102_msg(uint16_t new_slava_id,uint16_t new_baudrate){
    
    for (int i = 0; i < 9; i++) {
        temp_data[i] = 4294966296;  // 对应于 -1000 的无符号值
        
    }
    temp_data[9] = 1060094140;

    for(int i = 0; i < 18; i++){
        temp_data2[i] = 0;
    }

    memset(&modbus_image.image,0,sizeof(modbus_image.image));
    modbus_image.reg_length = 0;
    portENTER_CRITICAL(&param_lock);
   
    mb_set_uint16_ab((val_16_arr *)&a1102_msg.slave_id, (uint16_t)new_slava_id);
    mb_set_uint16_ab(&a1102_msg.buatrate, (uint16_t)new_baudrate);
    mb_set_uint32_abcd((val_32_arr *)&a1102_msg.device_version, (uint32_t)1);
    mb_set_uint32_abcd((val_32_arr *)&a1102_msg.device_id, (uint32_t)1);
    mb_set_uint32_abcd((val_32_arr *)&a1102_msg.modle_id, (uint32_t)1);


    for (int i = 0; i < 10; i++) {
        mb_set_uint32_abcd((val_32_arr *)&a1102_msg.result[i], (uint32_t)-1000);
    }

    for (int i = 0; i < 9; i++) {
        mb_set_uint32_abcd((val_32_arr *)&a1102_msg.xy[i], (uint32_t)0);
        mb_set_uint32_abcd((val_32_arr *)&a1102_msg.wh[i], (uint32_t)0);
    }

    
    portEXIT_CRITICAL(&param_lock);

}


void refresh_data(void *pvParameters){

    while (1)
    {
        for (int i = 0; i < 10; i++) {
            if(i == 9){
                portENTER_CRITICAL(&param_lock);
                mb_set_uint32_abcd((val_32_arr *)&a1102_msg.modle_id, temp_data[i]);
                
                portEXIT_CRITICAL(&param_lock);
            }else{
                portENTER_CRITICAL(&param_lock);
                mb_set_uint32_abcd((val_32_arr *)&a1102_msg.result[i], temp_data[i]);
                
                portEXIT_CRITICAL(&param_lock);
            }



        }

        for(int i = 0; i < 9; i++){
            portENTER_CRITICAL(&param_lock);
            mb_set_uint32_abcd((val_32_arr *)&a1102_msg.xy[i], temp_data2[i]);
            mb_set_uint32_abcd((val_32_arr *)&a1102_msg.wh[i], temp_data2[9+i]);
            portEXIT_CRITICAL(&param_lock);

        }

        // 延迟1秒（1000毫秒），然后再次更新
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    
}




void set_up_modbus(uint16_t new_slava_id,uint16_t new_baudrate){
    if(modbus_reset_flage != 0){
        printf("reset modbus setting !!!\r\n");
        mbc_slave_destroy();
    }
    

    printf("start modbus setting !!!\r\n");
 // Set UART log level
    esp_log_level_set(TAG, ESP_LOG_INFO);
    void* mbc_slave_handler = NULL;

    
    ESP_ERROR_CHECK(mbc_slave_init(MB_PORT_SERIAL_SLAVE, &mbc_slave_handler)); // Initialization of Modbus controller
    printf("mbc_slave_init end !!!\r\n");
    // Setup communication parameters and start stack



    comm_info.mode = MB_MODE_RTU,
    comm_info.slave_addr = new_slava_id;
    comm_info.port = 0;
    if(new_baudrate == 115){
        comm_info.baudrate = 115200;
    }else{
        comm_info.baudrate = (uint32_t)new_baudrate;
    }
    // comm_info.baudrate = 115200;
    comm_info.parity = MB_PARITY_NONE;
    ESP_ERROR_CHECK(mbc_slave_setup((void*)&comm_info));
    printf("mbc_slave_setup end !!!\r\n");
    /*
        开始映射寄存器区域：
            slave_id  -------------> 0
            buatrate  -------------> 1
            device_version --------> 2
            device_id -------------> 0X8000 ----> 32768
            modle_id --------------> 0X8002 ----> 32770
            result 0~9 ------------> 0x1000 ~ 0x1010 ----> 4096 ~ 4112
            坐标信息xy  -----> 0x2000 ~0x2000 + 9
            坐标信息wh  -----> 0x3000 ~ 0x3000 + 9
            图片：0X8006 + 10241
    */

    // 映射 slave_id  uint16 ---> 占一个寄存器
    reg_area.type = MB_PARAM_HOLDING; // Set type of register area
    reg_area.start_offset = 0x0000; // Offset of register area in Modbus protocol
    reg_area.address = (void*)&a1102_msg.slave_id; // Set pointer to storage instance
    // Set the size of register storage instance in bytes
    reg_area.size = sizeof(a1102_msg.slave_id);
    ESP_ERROR_CHECK(mbc_slave_set_descriptor(reg_area));
    
    //映射 buatrate  uint 16 
    reg_area.type = MB_PARAM_HOLDING; // Set type of register area
    reg_area.start_offset = 0x0001; // Offset of register area in Modbus protocol
    reg_area.address = (void*)&a1102_msg.buatrate; // Set pointer to storage instance
    // Set the size of register storage instance in bytes
    reg_area.size = sizeof(uint32_t);
    ESP_ERROR_CHECK(mbc_slave_set_descriptor(reg_area));

    //映射 device_version
    reg_area.type = MB_PARAM_HOLDING; // Set type of register area
    reg_area.start_offset = 0x0003; // Offset of register area in Modbus protocol
    reg_area.address = (void*)&a1102_msg.device_version; // Set pointer to storage instance
    // Set the size of register storage instance in bytes
    reg_area.size = sizeof(a1102_msg.device_version);
    ESP_ERROR_CHECK(mbc_slave_set_descriptor(reg_area));

    //映射 device_id
    reg_area.type = MB_PARAM_HOLDING; // Set type of register area
    reg_area.start_offset = 0X8000; // Offset of register area in Modbus protocol
    reg_area.address = (void*)&a1102_msg.device_id; // Set pointer to storage instance
    // Set the size of register storage instance in bytes
    reg_area.size = sizeof(a1102_msg.device_id);
    ESP_ERROR_CHECK(mbc_slave_set_descriptor(reg_area));


    // 映射 modle_id
    reg_area.type = MB_PARAM_HOLDING; // Set type of register area
    reg_area.start_offset = 0X8002; // Offset of register area in Modbus protocol
    reg_area.address = (void*)&a1102_msg.modle_id; // Set pointer to storage instance
    // Set the size of register storage instance in bytes
    reg_area.size = sizeof(a1102_msg.modle_id);
    ESP_ERROR_CHECK(mbc_slave_set_descriptor(reg_area));

    //映射 result 0 ~ 9 
    reg_area.type = MB_PARAM_HOLDING; // Set type of register area
    reg_area.start_offset = 0x1000; // Offset of register area in Modbus protocol
    reg_area.address = (void*)&a1102_msg.result; // Set pointer to storage instance
    // Set the size of register storage instance in bytes
    reg_area.size = sizeof(a1102_msg.result);
    ESP_ERROR_CHECK(mbc_slave_set_descriptor(reg_area));

    //映射 坐标信息xy 
    reg_area.type = MB_PARAM_HOLDING; // Set type of register area
    reg_area.start_offset = 0x2000; // Offset of register area in Modbus protocol
    reg_area.address = (void*)&a1102_msg.xy; // Set pointer to storage instance
    // Set the size of register storage instance in bytes
    reg_area.size = sizeof(a1102_msg.xy);
    ESP_ERROR_CHECK(mbc_slave_set_descriptor(reg_area));

        //映射 坐标信息xy 
    reg_area.type = MB_PARAM_HOLDING; // Set type of register area
    reg_area.start_offset = 0x3000; // Offset of register area in Modbus protocol
    reg_area.address = (void*)&a1102_msg.wh; // Set pointer to storage instance
    // Set the size of register storage instance in bytes
    reg_area.size = sizeof(a1102_msg.wh);
    ESP_ERROR_CHECK(mbc_slave_set_descriptor(reg_area));


    //映射 图片长度
    reg_area.type = MB_PARAM_HOLDING; // Set type of register area
    reg_area.start_offset = 0x8006; // Offset of register area in Modbus protocol
    reg_area.address = (void*)&modbus_image.reg_length; // Set pointer to storage instance
    // Set the size of register storage instance in bytes
    reg_area.size = sizeof(modbus_image.reg_length);
    ESP_ERROR_CHECK(mbc_slave_set_descriptor(reg_area));

    //映射 图片
    reg_area.type = MB_PARAM_HOLDING; // Set type of register area
    reg_area.start_offset = 0x8007; // Offset of register area in Modbus protocol
    reg_area.address = (void*)&modbus_image.image; // Set pointer to storage instance
    // Set the size of register storage instance in bytes
    reg_area.size = sizeof(modbus_image.image);
    ESP_ERROR_CHECK(mbc_slave_set_descriptor(reg_area));

    printf("mbc_slave_set_descriptor end !!!\r\n");

        // setup_reg_data(); // Set values into known state
    init_a1102_msg(new_slava_id,new_baudrate);// 设置初始值
    printf("init_a1102_msg end !!!\r\n");
    // Starts of modbus controller and stack
    ESP_ERROR_CHECK(mbc_slave_start());

    printf("mbc_slave_start  !!!\r\n");

    // Set UART pin numbers
    ESP_ERROR_CHECK(uart_set_pin(0, 6,
                            7, 4,
                            UART_PIN_NO_CHANGE));

    printf("uart_set_pin end  !!!\r\n");

    // Set UART driver mode to Half Duplex
    ESP_ERROR_CHECK(uart_set_mode(0, UART_MODE_RS485_HALF_DUPLEX));

    printf("uart_set_mode end  !!!\r\n");

    ESP_LOGI(TAG, "Modbus slave stack initialized.");
    ESP_LOGI(TAG, "Start modbus test...");

}


void modbus_task(void *pvParameter){
    set_up_modbus(1,115);
    while (1)
    {
      // Check for read/write events of Modbus master for certain events
        (void)mbc_slave_check_event(MB_READ_WRITE_MASK);
        ESP_ERROR_CHECK_WITHOUT_ABORT(mbc_slave_get_param_info(&reg_info, MB_PAR_INFO_GET_TOUT));
        const char* rw_str = (reg_info.type & MB_READ_MASK) ? "READ" : "WRITE";

        // Filter events and process them accordingly
        if(reg_info.type & (MB_EVENT_HOLDING_REG_WR | MB_EVENT_HOLDING_REG_RD)) {
            // Get parameter information from parameter queue
            ESP_LOGI(TAG, "HOLDING %s (%" PRIu32 " us), ADDR:%u, TYPE:%u, INST_ADDR:0x%" PRIx32 ", SIZE:%u",
                            rw_str,
                            reg_info.time_stamp,
                            (unsigned)reg_info.mb_offset,
                            (unsigned)reg_info.type,
                            (uint32_t)reg_info.address,
                            (unsigned)reg_info.size);

            // 修改波特率
            if (reg_info.address == (uint8_t*)&a1102_msg.buatrate && reg_info.type &  MB_EVENT_HOLDING_REG_WR) {
                ESP_LOGI(TAG,"change buatrate");
                modbus_reset_flage = 1;
                set_up_modbus(a1102_msg.slave_id,a1102_msg.buatrate);
            }

            // 修改slave id
            if (reg_info.address == (uint8_t*)&a1102_msg.slave_id && reg_info.type &  MB_EVENT_HOLDING_REG_WR) {
                ESP_LOGI(TAG,"change slave_id");
                modbus_reset_flage = 1;
                set_up_modbus(a1102_msg.slave_id,a1102_msg.buatrate);
            }
            // 传图
            if (reg_info.mb_offset == 0x8002)
            {
                
                ESP_LOGI(TAG,"reg_info.mb_offset == 0x8002");
                for (int i = 0; i < 9; i++)
                {  
                    temp_data[i] = -1000;
                }
                
                sscma_client_write(client,"AT+INVOKE=1,0,0", 15);
                sscma_client_invoke(client, -1, false, false);
            }



        }

        
    }
    
    // Destroy of Modbus controller on alarm
    ESP_LOGI(TAG,"Modbus controller destroyed.");
    vTaskDelay(100);
    ESP_ERROR_CHECK(mbc_slave_destroy());
}


void app_modbus_init(){
    xTaskCreate(refresh_data,"Update Data Task", 2048, NULL, 3, NULL);
    xTaskCreate(&modbus_task, "modbus_task", 1024 * 8, NULL, 5,NULL);
}



