#pragma once

#include "driver/gpio.h"
#include "constants.h"

void button_controller_start(gpio_num_t mode, gpio_num_t status);
