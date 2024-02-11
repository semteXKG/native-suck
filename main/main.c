#include <stdio.h>
#include "esp_err.h"
#include "esp_zigbee_gateway.h"
#include "wlan.h"
#include "webserver.h"

void app_main(void)
{
    ESP_LOGI("APP", "hallo welt");
    
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    wlan_start();
    //zigbee_start();    
    vTaskDelay(10000 / portTICK_PERIOD_MS);
    webserver_start();
}