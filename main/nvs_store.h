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
#include "constants.h"

void set_limit(uint16_t low_limit, uint16_t high_limit, uint16_t afterrun_seconds);
uint16_t get_low_limit();
uint16_t get_high_limit();
uint16_t get_afterrun_seconds();

void store_save_last_op_mode(enum OpMode opMode);
enum OpMode store_read_last_op_mode();


void set_wlan_credentials(char* ap_name, char* password);
void store_set_udp_server(char* store, uint16_t port);

char* store_read_wlan_ap();
char* store_read_wlan_pass();
char* store_read_udp_server();

uint16_t store_read_udp_port();

void store_start();
