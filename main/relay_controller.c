#include "relay_controller.h"

gpio_num_t low_speed, high_speed;

void controller_start(gpio_num_t low_speed_relay, gpio_num_t high_speed_relay) {
    uint64_t pin_mask =  ((1ULL<<low_speed_relay) | (1ULL<<high_speed_relay));

    low_speed = low_speed_relay;
    high_speed = high_speed_relay;
    gpio_config_t config = {
        .intr_type = GPIO_INTR_DISABLE,
        .pin_bit_mask = pin_mask,
        .pull_down_en = 0,
        .pull_up_en = 0,
        .mode = GPIO_MODE_OUTPUT
    };
    gpio_config(&config);
    gpio_set_level(low_speed, 1);
    gpio_set_level(high_speed, 1);
}

void handleStateChange(enum MachineStatus newStatus) {
    gpio_set_level(low_speed, 1);
    gpio_set_level(high_speed, 1);
    
    if(newStatus == ON_LOW) {
        gpio_set_level(low_speed, 0);
    } else if (newStatus == ON_HIGH) {
        gpio_set_level(high_speed, 0);
    }
}