#include "bsp.h"


sscma_client_io_handle_t io = NULL;
sscma_client_handle_t client = NULL;

portMUX_TYPE param_lock = portMUX_INITIALIZER_UNLOCKED;


