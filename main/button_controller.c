#include "button_controller.h"
#include "iot_button.h"
#include "esp_log.h"
#include "state.h"

gpio_num_t mode_selector, status_selector;

static const char* TAG = "BUTTON_CONTROLLER";
static const u_int8_t STATUS = 0;
static const u_int8_t MODE = 1;

button_config_t createConfig(gpio_num_t pin) {
    button_config_t btn_cfg = {
        .type = BUTTON_TYPE_GPIO,
        .short_press_time = 50,
        .gpio_button_config = {
            .gpio_num = pin,
            .active_level = 1,
        },
    };
    return btn_cfg;
}

static void button_pressed(void *button_handle, void *usr_data) {
    u_int8_t* data = (u_int8_t*) usr_data;
    switch (*data) {
        case MODE:
            ESP_LOGI(TAG, "MODE touched");
            advanceOpMode();
        break;
        case STATUS:
            ESP_LOGI(TAG, "STATUS touched");
            setOpMode(LOCAL);
            advanceStatus();
        break;
    }
}

void button_controller_start(gpio_num_t mode, gpio_num_t status) {
    button_config_t status_conf = createConfig(status);
    button_config_t mode_conf = createConfig(mode);
    button_handle_t status_btn = iot_button_create(&status_conf);
    button_handle_t mode_btn = iot_button_create(&mode_conf);
    
    iot_button_register_cb(status_btn, BUTTON_PRESS_DOWN, button_pressed, &STATUS);
    iot_button_register_cb(mode_btn, BUTTON_PRESS_DOWN, button_pressed, &MODE);
}

