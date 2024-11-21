#include "app_nvs.h"



static const char *TAG = "app_nvs";

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


modbus_parm nvs_modbus_param()
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    modbus_parm current_param;

    nvs_handle_t my_handle;
    ret = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &my_handle);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error (%s) opening NVS handle!", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "NVS handle opened successfully");
    }
    
    current_param.modbus_address = 1; 
    ret = nvs_get_u16(my_handle, NVS_KEY_ADDR, &current_param.modbus_address);
    if (ret == ESP_ERR_NVS_NOT_FOUND) {
        //modbus_address = 1;
        ret = nvs_set_u16(my_handle, NVS_KEY_ADDR, current_param.modbus_address);
        ESP_ERROR_CHECK(ret);
    } else {
        ESP_ERROR_CHECK(ret);
    }

    current_param.modbus_baud = 115200;
    ret = nvs_get_u16(my_handle, NVS_KEY_BAUD, &current_param.modbus_baud);
    if (ret == ESP_ERR_NVS_NOT_FOUND) {
        //modbus_baud = 115200;
        ret = nvs_set_u16(my_handle, NVS_KEY_BAUD, current_param.modbus_baud);
        ESP_ERROR_CHECK(ret);
    } else {
        ESP_ERROR_CHECK(ret);
    }

    ret = nvs_commit(my_handle);
    ESP_ERROR_CHECK(ret);

    printf("current param is %d  %ld\r\n",current_param.modbus_address,current_param.modbus_baud);
    return current_param;
}


void app_nvs_init(){
    // esp_err_t ret = nvs_flash_init();
    // if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    //     ESP_ERROR_CHECK(nvs_flash_erase());
    //     ret = nvs_flash_init();
    // }
    // ESP_ERROR_CHECK(ret);
    g_a1102_param.modbus_p = nvs_modbus_param();
    g_a1102_param.modle_p.current_modle = Prefab_Models;
    

}