/*
 * SPDX-FileCopyrightText: 2021-2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "driver/i2c.h"
#include "esp_err.h"
#include "esp_log.h"
#include "lvgl.h"
#include "esp_lvgl_port.h"
#include "state.h"

#include "esp_lcd_panel_vendor.h"

static const char *TAG = "display";

#define I2C_HOST  0

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////// Please update the following configuration according to your LCD spec //////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define EXAMPLE_LCD_PIXEL_CLOCK_HZ    (400 * 1000)
#define EXAMPLE_PIN_NUM_SDA           5
#define EXAMPLE_PIN_NUM_SCL           6
#define EXAMPLE_PIN_NUM_RST           -1
#define EXAMPLE_I2C_HW_ADDR           0x3C

// The pixel number in horizontal and vertical
#define EXAMPLE_LCD_H_RES              128
#define EXAMPLE_LCD_V_RES              64

// Bit number used to represent command and parameter
#define EXAMPLE_LCD_CMD_BITS           8
#define EXAMPLE_LCD_PARAM_BITS         8

static int16_t col_dsc[] = {42, 42, 43, LV_GRID_TEMPLATE_LAST};
static int16_t row_dsc[] = {8, 8, 8, LV_GRID_TEMPLATE_LAST};

char temp[10];
char hum[10];
char ppm[20];
char preassure_diff[30];
char centerText[30];
char ip_address[30];

lv_obj_t *tmp_label;
lv_obj_t *hum_label;
lv_obj_t *ppm_label;
lv_obj_t *status_label;
lv_obj_t *opmode_label;
lv_obj_t *ip_address_label;
lv_obj_t *preassure_diff_label;

static void periodic_timer_callback(void* arg) {
    lvgl_port_lock(0);

    measurements_t measurements = getMeasurements();
    sprintf(temp, "%d°C", measurements.temperature);
    sprintf(hum, "%d%%", measurements.humidity);
    sprintf(ppm, "%.1f", measurements.pm25Level);
    sprintf(preassure_diff, "%ldPa", ac400_get_preasure_diff());

    lv_label_set_text(tmp_label, temp);
    lv_label_set_text(hum_label, hum);
    lv_label_set_text(ppm_label, ppm);
    
    strcpy(centerText, getOpModeString());
    strcat(centerText, " - ");
    strcat(centerText, getStatusString());
    lv_label_set_text(status_label, centerText);
    
    strcpy(ip_address, get_ip_address());
    lv_label_set_text(ip_address_label, ip_address);
    lv_label_set_text(preassure_diff_label, preassure_diff);
    lvgl_port_unlock();
}

void create_ui_components(lv_disp_t *disp)
{
    measurements_t measurements = getMeasurements();
    sprintf(temp, "%d°C", measurements.temperature);
    sprintf(hum, "%d%%", measurements.humidity);
    sprintf(ppm, "%.1f", measurements.pm25Level);
    sprintf(preassure_diff, "%ldPa", ac400_get_preasure_diff());

    lv_obj_t *scr = lv_disp_get_scr_act(disp);
    tmp_label = lv_label_create(scr);
    lv_label_set_text(tmp_label, temp);
    lv_obj_align(tmp_label, LV_ALIGN_TOP_LEFT, 0, 0);

    hum_label = lv_label_create(scr);
    lv_label_set_text(hum_label, hum);
    lv_obj_align(hum_label, LV_ALIGN_TOP_RIGHT, 0, 0);

    ppm_label = lv_label_create(scr);
    lv_label_set_text(ppm_label, ppm);
    lv_obj_align(ppm_label, LV_ALIGN_TOP_MID, 0, 0);

    strcat(centerText, getOpModeString());
    strcat(centerText, " - ");
    strcat(centerText, getStatusString());
    
    status_label = lv_label_create(scr);
    lv_label_set_text(status_label, centerText);
    lv_obj_set_style_text_font(status_label, &lv_font_montserrat_22, LV_PART_MAIN);

    lv_obj_align(status_label, LV_ALIGN_CENTER, 0, 0);

    strcpy(ip_address, get_ip_address());
    
    ip_address_label = lv_label_create(scr);
    lv_label_set_text(ip_address_label, ip_address);
    lv_obj_align(ip_address_label, LV_ALIGN_BOTTOM_RIGHT, 0, 0);

    preassure_diff_label = lv_label_create(scr);
    lv_label_set_text(preassure_diff_label, preassure_diff);
    lv_obj_align(preassure_diff_label, LV_ALIGN_BOTTOM_LEFT, 0, 0);
}

void display_start(void)
{
    ESP_LOGI(TAG, "Initialize I2C bus");
    i2c_config_t i2c_conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = EXAMPLE_PIN_NUM_SDA,
        .scl_io_num = EXAMPLE_PIN_NUM_SCL,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = EXAMPLE_LCD_PIXEL_CLOCK_HZ,
    };
    ESP_ERROR_CHECK(i2c_param_config(I2C_HOST, &i2c_conf));
    ESP_ERROR_CHECK(i2c_driver_install(I2C_HOST, I2C_MODE_MASTER, 0, 0, 0));

    ESP_LOGI(TAG, "Install panel IO");
    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_i2c_config_t io_config = {
        .dev_addr = EXAMPLE_I2C_HW_ADDR,
        .control_phase_bytes = 1,               // According to SSD1306 datasheet
        .lcd_cmd_bits = EXAMPLE_LCD_CMD_BITS,   // According to SSD1306 datasheet
        .lcd_param_bits = EXAMPLE_LCD_CMD_BITS, // According to SSD1306 datasheet
        .dc_bit_offset = 6,                     // According to SSD1306 datasheet
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c(I2C_HOST, &io_config, &io_handle));

    ESP_LOGI(TAG, "Install SSD1306 panel driver");
    esp_lcd_panel_handle_t panel_handle = NULL;
    esp_lcd_panel_dev_config_t panel_config = {
        .bits_per_pixel = 1,
        .reset_gpio_num = EXAMPLE_PIN_NUM_RST,
    };

    ESP_ERROR_CHECK(esp_lcd_new_panel_ssd1306(io_handle, &panel_config, &panel_handle));

    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

    ESP_LOGI(TAG, "Initialize LVGL");
    const lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    lvgl_port_init(&lvgl_cfg);

    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = io_handle,
        .panel_handle = panel_handle,
        .buffer_size = EXAMPLE_LCD_H_RES * EXAMPLE_LCD_V_RES,
        .double_buffer = true,
        .hres = EXAMPLE_LCD_H_RES,
        .vres = EXAMPLE_LCD_V_RES,
        .monochrome = true,
        .rotation = {
            .swap_xy = false,
            .mirror_x = true,
            .mirror_y = true,
        }
    };
    lv_disp_t * disp = lvgl_port_add_disp(&disp_cfg);
    
    /* Rotation of the screen */
    lv_disp_set_rotation(disp, LV_DISP_ROT_NONE);

    ESP_LOGI(TAG, "Display LVGL Scroll Text");
    // Lock the mutex due to the LVGL APIs are not thread-safe
    if (lvgl_port_lock(0)) {
        create_ui_components(disp);
        // Release the mutex
        lvgl_port_unlock();
    }

    const esp_timer_create_args_t periodic_timer_args = {
            .callback = &periodic_timer_callback,
            .name = "periodic"
    };

    esp_timer_handle_t periodic_timer;
    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, 500000));
}