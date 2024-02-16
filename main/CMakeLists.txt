idf_component_register(SRCS "button_controller.c" "relay_controller.c" "main.c" "esp_zigbee_gateway.c" "webserver.c" "wlan.c" "state.c"
                    INCLUDE_DIRS ".")
spiffs_create_partition_image(storage ../data FLASH_IN_PROJECT)