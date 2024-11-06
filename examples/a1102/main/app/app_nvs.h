#ifndef _APP_NVS_H
#define _APP_NVS_H

#include "bsp.h"


#define NVS_KEY_ADDR "modbus_addr"
#define NVS_KEY_BAUD "modbus_baud"


esp_err_t write_modbus_address(uint16_t modbus_address);
esp_err_t write_modbus_baud(uint32_t modbus_baud);

void app_nvs_init();

#endif