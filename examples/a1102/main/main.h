#ifndef MAIN_H
#define MAIN_H

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

#include <net/if.h>
#include "mqtt_client.h"
#include "esp_spiffs.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_netif.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include "mbcontroller.h"

#include "sscma_client_io.h"
#include "sscma_client_ops.h"


#include "sscma_client_commands.h"

#define TAG "SenseCAP A1102"

/* The event group allows multiple bits for each event, but we only care about one event */
/* - are we connected to the AP with an IP? */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

/* - are we connected to the MQTT Server? */
#define MQTT_CONNECTED_BIT BIT2

esp_mqtt_client_handle_t mqtt_client = NULL;

static int s_retry_num = 0;
static EventGroupHandle_t s_wifi_event_group;
static EventGroupHandle_t s_mqtt_event_group;
static char mqtt_tx_topic[128];
static char mqtt_rx_topic[128];
static char mqtt_client_id[64];

// typedef struct {
//     char ssid[32];
//     char password[64];
// } wifi_credentials_t;

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

#define NVS_NAMESPACE "storage"
#define NVS_KEY_ADDR "modbus_addr"
#define NVS_KEY_BAUD "modbus_baud"

uint16_t modbus_address = 1;
uint32_t modbus_baud = 115200;

char wifi_ssid = NULL;
char wifi_pass = NULL;

volatile int32_t reg[11] = { 0 };
float magic = 0;



#define NVS_NAMESPACE "storage"
#define NVS_KEY_SSID "wifi_ssid"
#define NVS_KEY_PASSWORD "wifi_password"
#define NVS_KEY_MQTT_USERNAME "mqtt_username"
#define NVS_KEY_MQTT_PASSWORD "mqtt_password"
#define NVS_KEY_MQTT_ADDRESS "mqtt_address"
#define NVS_KEY_MQTT_PORT "mqtt_port"
#define NVS_KEY_MQTT_PORT1 "mqtt_port1"
#define NVS_KEY_MQTT_CLIENT_ID "mqtt_client_id"
#define NVS_KEY_MQTT_USE_SSL "mqtt_use_ssl"
#define NVS_KEY_MQTT_USE_SSL1 "mqtt_use_ssl1"


/*
 * SPDX-FileCopyrightText: 2024 Seeed Technology Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

/** Major version number (X.x.x) */
#define SSCMA_CLIENT_MONITOR_VERSION_MAJOR 0
/** Minor version number (x.X.x) */
#define SSCMA_CLIENT_MONITOR_VERSION_MINOR 0
/** Patch version number (x.x.X) */
#define SSCMA_CLIENT_MONITOR_VERSION_PATCH 1

/**
 * Macro to convert version number into an integer
 *
 * To be used in comparisons, such as SSCMA_CLIENT_MONITOR_VERSION >= SSCMA_CLIENT_MONITOR_VERSION_VAL(4, 0, 0)
 */
#define SSCMA_CLIENT_MONITOR_VERSION_VAL(major, minor, patch) ((major << 16) | (minor << 8) | (patch))

/**
 * Current version, as an integer
 *
 * To be used in comparisons, such as SSCMA_CLIENT_MONITOR_VERSION >= SSCMA_CLIENT_MONITOR_VERSION_VAL(4, 0, 0)
 */
#define SSCMA_CLIENT_MONITOR_VERSION SSCMA_CLIENT_MONITOR_VERSION_VAL(SSCMA_CLIENT_MONITOR_VERSION_MAJOR, SSCMA_CLIENT_MONITOR_VERSION_MINOR, SSCMA_CLIENT_MONITOR_VERSION_PATCH)



#endif // !MAIN_H