#pragma once

#include "driver/gpio.h"
#include "constants.h"

void controller_start(gpio_num_t low_speed_relay, gpio_num_t high_speed_relay);
void handleStateChange(enum MachineStatus machineState);
