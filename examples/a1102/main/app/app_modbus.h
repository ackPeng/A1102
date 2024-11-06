#ifndef __APP_MODBUS_H
#define __APP_MODBUS_H


#include "mbcontroller.h"       // for mbcontroller defines and api
#include "bsp.h"

#define MB_PAR_INFO_GET_TOUT (10) // Timeout for get parameter info
#define MB_READ_MASK         (MB_EVENT_INPUT_REG_RD | MB_EVENT_HOLDING_REG_RD | MB_EVENT_DISCRETE_RD | MB_EVENT_COILS_RD)
#define MB_WRITE_MASK        (MB_EVENT_HOLDING_REG_WR | MB_EVENT_COILS_WR)
#define MB_READ_WRITE_MASK   (MB_READ_MASK | MB_WRITE_MASK)

typedef struct{
    uint16_t slave_id; // 从站地址  RW
    uint16_t buatrate; //波特率   RW
    uint32_t device_version; //设备版本  R
    uint32_t device_id; // 设备ID  R
    uint32_t modle_id;//模型ID  R
    uint32_t result[10];//结果   R
    uint32_t xy[9];
    uint32_t wh[9];
    // uint32_t result2[18];

}a1102_holding_reg_params_t;

// 两种方式： 1 设置定长的数据结构存储image  2、每次都变长
typedef struct 
{
    uint16_t reg_length;
    uint16_t image[10240]
    /* data */
}holding_image_params;



// void set_up_modbus(uint16_t new_slava_id,uint16_t new_baudrate);

void app_modbus_init();
#endif