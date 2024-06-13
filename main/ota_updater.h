#pragma once

#include <ac400_util.h>
#include "multipart_parser.h"
#include <esp_err.h>

void ota_updater_print_info();

void ota_updater_activate_current_partition();

void ota_updater_rollback();

void ac400_firmware_updater(httpd_req_t *req);