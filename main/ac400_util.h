#pragma once

#include <string.h>
#include <constants.h>
#include <stdbool.h>
#include <esp_http_server.h>


void printBuffer(char* buffer, size_t start, size_t count);
char* find_delimiter(httpd_req_t* req);

