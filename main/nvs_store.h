#pragma once
#include <stdio.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include "string.h"

void set_limit(uint16_t lowLimit, uint16_t highLimit);
uint16_t get_low_limit();
uint16_t get_high_limit();

void set_wlan_credentials(char* ap_name, char* password);
char* get_wlan_ap();
char* get_wlan_pass();

void store_start();
