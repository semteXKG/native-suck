#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h> //Requires by memset
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "spi_flash_mmap.h"
#include <esp_http_server.h>
#include "esp_spiffs.h"
#include "esp_log.h"
#include "state.h"
#include "nvs_store.h"
#include "esp_ota_ops.h"
#include "ac400_util.h"
void webserver_start();
