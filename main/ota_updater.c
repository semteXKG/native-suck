#include "ota_updater.h"
#include "esp_ota_ops.h"
#include "esp_log.h"

static const char* TAG = "OTA";
esp_ota_handle_t handle;


void ota_updater_print_info() {
    const esp_partition_t* partition = esp_ota_get_running_partition();
    esp_app_desc_t app_desc;
    esp_ota_get_partition_description(partition, &app_desc);
    ESP_LOGI(TAG, "Running %s in %s from %s", app_desc.project_name, app_desc.version, app_desc.date);
}

void ota_updater_activate_current_partition() {
    ESP_LOGI(TAG, "Marking image as runnable");
    esp_ota_mark_app_valid_cancel_rollback();
}

void ota_updater_rollback() {
    ESP_LOGI(TAG, "Scheduling a rollback of current image");
    esp_ota_check_rollback_is_possible();
}

void partition_and_restart(void *parameters) {
    ESP_LOGI(TAG, "Running from inside the task"); 
    esp_partition_t* partition = (esp_partition_t*) parameters;
    vTaskDelay(5000 / portTICK_PERIOD_MS);
    ESP_LOGI(TAG, "Switching boot partitions."); 
    ESP_ERROR_CHECK(esp_ota_set_boot_partition(partition));
    ESP_LOGI(TAG, "Switched boot partitions, restarting."); 
    esp_restart();
}

int write_part_data(multipart_parser* parser, const char *at, size_t length) {
    ESP_ERROR_CHECK(esp_ota_write(handle, at, length));  
    return 0;
}

void ac400_firmware_updater(httpd_req_t *req) {
    int bufSize = 1000;
    char buffer[bufSize];

    esp_partition_t* partition = esp_ota_get_next_update_partition(esp_ota_get_running_partition());
    ESP_LOGI(TAG, "STARTING upload to partition %s", partition->label);

    multipart_parser_settings callbacks;
    memset(&callbacks, 0, sizeof(multipart_parser_settings));

    callbacks.on_part_data = write_part_data;
    char* needle = find_delimiter(req);

    multipart_parser* parser = multipart_parser_init(needle, &callbacks);
    ESP_ERROR_CHECK(esp_ota_begin(partition, req->content_len, &handle));

    while (true) {
        size_t data_received = httpd_req_recv(req, buffer, bufSize);
        if(data_received == 0) {
            break;
        }
        multipart_parser_execute(parser, buffer, data_received);
    }
    multipart_parser_free(parser);
    ESP_ERROR_CHECK(esp_ota_end(handle));

    xTaskCreate(partition_and_restart, "restart", 4096, partition, 5, NULL);

    ESP_LOGI(TAG, "DONE upload");
}