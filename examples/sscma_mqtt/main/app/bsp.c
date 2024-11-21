#include "bsp.h"


sscma_client_io_handle_t io = NULL;
sscma_client_handle_t client = NULL;

bool himax_config = false;
bool mqtt_connect = false;
QueueHandle_t queue;