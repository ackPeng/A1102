#include "bsp.h"


sscma_client_io_handle_t io = NULL;
sscma_client_handle_t client = NULL;

portMUX_TYPE param_lock = portMUX_INITIALIZER_UNLOCKED;
esp_event_loop_handle_t change_param_event_loop;
QueueHandle_t ble_msg_queue;
ESP_EVENT_DEFINE_BASE(PARAM_CHANGE_EVENT_BASE);

A1102_param g_a1102_param;

const char* model_to_string(modle_type model) {
    switch (model) {
    case Prefab_Models:
        return "Prefab_Models";
    case Human_Body_Detection:
        return "Human_Body_Detection";
    case Meter_Identification:
        return "Meter_Identification";
    case People_Counting:
        return "People_Counting";
    default:
        return "Unknown";
    }
}