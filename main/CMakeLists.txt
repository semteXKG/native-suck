idf_component_register(SRCS "ota_updater.c" "ac400_util.c" "display.c" "nvs_store.c" "button_controller.c" "relay_controller.c" "main.c" "esp_zigbee_gateway.c" "webserver.c" "wlan.c" "state.c" "nvs_store.c" "display.c" "env_sensor.c" "ac400_util.c"
                    INCLUDE_DIRS ".")
spiffs_create_partition_image(storage ../data FLASH_IN_PROJECT)
nvs_create_partition_image(nvs ../nvs.csv FLASH_IN_PROJECT)