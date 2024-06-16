#include <stdio.h>
#include "esp_err.h"
#include "esp_zigbee_gateway.h"
#include "wlan.h"
#include "webserver.h"
#include "esp_coexist.h"
#include "relay_controller.h"
#include "button_controller.h"
#include "nvs_store.h"
#include "display.h"
#include "env_sensor.h"
#include "ota_updater.h"
#include "net_logging.h"

void logging_start() {
    if (strlen(store_read_udp_server()) > 0) {
        ESP_LOGI("main", "Starting with logging on %s:%d", store_read_udp_server(), store_read_udp_port());
        unsigned long port = store_read_udp_port();
        udp_logging_init(store_read_udp_server(), port, 1);
    }
}

void app_main(void)
{
    ota_updater_print_info();

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_coex_wifi_i154_enable();
    display_start();
    store_start();
    controller_start(GPIO_NUM_23, GPIO_NUM_22);
    button_controller_start(GPIO_NUM_21, GPIO_NUM_20);
    wlan_start();

    logging_start();
    
    zigbee_start();    
    webserver_start();

    setOpMode(store_read_last_op_mode());
    
    ota_updater_activate_current_partition();
}
