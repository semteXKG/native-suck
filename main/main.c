#include <stdio.h>
#include "esp_err.h"
#include "esp_zigbee_gateway.h"
#include "wlan.h"
#include "webserver.h"
#include "esp_coexist.h"
#include "relay_controller.h"
#include "button_controller.h"
#include "nvs_store.h"

void app_main(void)
{
    ESP_LOGI("APP", "hallo welt");
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_coex_wifi_i154_enable();

    store_start();
    controller_start(GPIO_NUM_23, GPIO_NUM_22);
    button_controller_start(GPIO_NUM_21, GPIO_NUM_20);
    wlan_start();
    zigbee_start();    
    webserver_start();
}