#pragma once

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs_store.h"
#include "state.h"

#include "lwip/inet.h"
#include "lwip/err.h"
#include "lwip/sys.h"

void wlan_start();