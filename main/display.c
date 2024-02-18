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


#include "esp_lcd_panel_vendor.h"

static const char *TAG = "example";

#define I2C_HOST  0

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////// Please update the following configuration according to your LCD spec //////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define EXAMPLE_LCD_PIXEL_CLOCK_HZ    (400 * 1000)
#define EXAMPLE_PIN_NUM_SDA           6
#define EXAMPLE_PIN_NUM_SCL           7
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

void example_lvgl_demo_ui(lv_disp_t *disp)
{
     /*Create a container with grid*/
    lv_obj_t * cont = lv_obj_create(lv_disp_get_scr_act(disp));
    lv_obj_set_style_grid_column_dsc_array(cont, col_dsc, 0);
    lv_obj_set_style_grid_row_dsc_array(cont, row_dsc, 0);
    lv_obj_set_size(cont, lv_pct(100), lv_pct(100));
    lv_obj_align(cont, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_layout(cont, LV_LAYOUT_GRID);

    lv_obj_t * label;
    lv_obj_t * obj;

    uint32_t i;
    for(i = 0; i < 9; i++) {
        uint8_t col = i % 3;
        uint8_t row = i / 3;

        label = lv_label_create(cont);
        lv_label_set_text_fmt(label, "%ld", i);
        lv_obj_center(label);

        /*Stretch the cell horizontally and vertically too
         *Set span to 1 to make the cell 1 column/row sized*/
        lv_obj_set_grid_cell(label, LV_GRID_ALIGN_STRETCH, col, 1,
                             LV_GRID_ALIGN_STRETCH, row, 1);
    }
}

void example_lvgl_demo_ui2(lv_disp_t *disp)
{
        lv_obj_t *scr = lv_disp_get_scr_act(disp);
    lv_obj_t *label = lv_label_create(scr);
    lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL_CIRCULAR); /* Circular scroll */
    lv_label_set_text(label, "Hello Espressif, Hello LVGL.");
    /* Size of the screen (if you use rotation 90 or 270, please set disp->driver->ver_res) */
    lv_obj_set_width(label, disp->driver->hor_res);
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 0);
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
            .mirror_x = false,
            .mirror_y = false,
        }
    };
    lv_disp_t * disp = lvgl_port_add_disp(&disp_cfg);
    
    /* Rotation of the screen */
    lv_disp_set_rotation(disp, LV_DISP_ROT_NONE);

    ESP_LOGI(TAG, "Display LVGL Scroll Text");
    // Lock the mutex due to the LVGL APIs are not thread-safe
    if (lvgl_port_lock(0)) {
        example_lvgl_demo_ui(disp);
        // Release the mutex
        lvgl_port_unlock();
    }
}