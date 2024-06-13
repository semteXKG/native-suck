#include <ac400_util.h>
#include <esp_log.h>

static const char *TAG = "util";

void printBuffer(char* buffer, size_t start, size_t count) {
    for (int i = start; i < start + count; i++) {
        ESP_LOGI(TAG, "%02X", buffer[i]);
    }
}

char* find_delimiter(httpd_req_t* req) {
    char* field = "Content-Type";
    char* boundary = "boundary=";
    size_t len = httpd_req_get_hdr_value_len(req, field);
    if(len == 0) {
        return NULL;
    }

    char header_value[len+1];
    httpd_req_get_hdr_value_str(req, field, header_value, len+1);
    char* new_start = strstr(header_value, boundary);
    if (new_start == NULL) {
        return NULL;
    }

    new_start += strlen(boundary);

    char* buffer_with_padding = malloc(3 + strlen(new_start));

    strcpy(buffer_with_padding, "--");
    strcat(buffer_with_padding, new_start);

    ESP_LOGI(TAG, "Header: %s with len %d", buffer_with_padding, strlen(buffer_with_padding));    
    return buffer_with_padding  ;
}
