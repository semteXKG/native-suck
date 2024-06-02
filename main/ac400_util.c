#include <ac400_util.h>
#include <esp_log.h>

static const char *TAG = "util";

void printBuffer(char* buffer, size_t start, size_t count) {
    for (int i = start; i < start + count; i++) {
        ESP_LOGI(TAG, "%02X", buffer[i]);
    }
}

find_result find_match(char* buffer, size_t len, char* search_string) {
    find_result result;
    result.found_at = NULL;
    result.partial_match = false;
    //ESP_LOGI(TAG, "Searching in %d buffer for a %d long search string", len, strlen(search_string));

    char* legacy_match = strstr(buffer, search_string);     
    if (legacy_match != NULL) {
        result.found_at = legacy_match;
        ESP_LOGI(TAG, "Legacy search found it");
    }
    return result;

    if (strlen(search_string) > len) {
        result.found_at = NULL;
        result.partial_match = false;
        return result;
    }

    int search_idx = 0;
    for (int source_idx = 0; source_idx < len; source_idx++) {
        if (buffer[source_idx] == search_string[search_idx]) {
            search_idx++;
            if (result.found_at == NULL) {
                result.found_at = &buffer[source_idx];
            }
            if (search_string[search_idx] == '\0') {
                ESP_LOGI(TAG, "Proper Search Found it. Own algo: %p, built in: %p", result.found_at, legacy_match);
                return result;
            }
        } else {
            result.found_at = NULL;
            search_idx = 0;
        }
    }

    if  (result.found_at != NULL) {
        result.found_at = NULL;
        result.partial_match = true;
    }

    return result;
}