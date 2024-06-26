/*
 * SPDX-FileCopyrightText: 2021-2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"
#include "esp_timer.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_io_interface.h"
#include "esp_lcd_panel_ops.h"
#include "esp_err.h"
#include "esp_log.h"
#include "lvgl.h"
#include "esp_lvgl_port.h"
#include "state.h"
#include "esp_heap_caps.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_ili9341.h"

static const char *TAG = "display";

#define TEST_LCD_HOST               SPI2_HOST
#define TEST_LCD_H_RES              (320)
#define TEST_LCD_V_RES              (240)
#define TEST_LCD_BIT_PER_PIXEL      (16)

#define TEST_PIN_NUM_LCD_RST        (GPIO_NUM_5)
#define TEST_PIN_NUM_LCD_CS         (GPIO_NUM_6)
#define TEST_PIN_NUM_LCD_PCLK       (GPIO_NUM_2)
#define TEST_PIN_NUM_LCD_DATA0      (GPIO_NUM_7)
#define TEST_PIN_NUM_LCD_DC         (GPIO_NUM_4)
#define TEST_PIN_NUM_LCD_BL         (GPIO_NUM_3)        

char temp[10];
char hum[10];
char ppm[20];
char centerText[30];
char ip_address[30];

lv_obj_t *tmp_label;
lv_obj_t *hum_label;
lv_obj_t *ppm_label;
lv_obj_t *status_label;
lv_obj_t *opmode_label;
lv_obj_t *ip_address_label;

static SemaphoreHandle_t refresh_finish = NULL;
lv_disp_t *disp;

uint32_t get_background_color() {
    switch (ac400_get_status()) {
        case OFF: 
            return 0x007700;
        case ON_LOW:
            return 0xDC9900;
        case ON_HIGH: 
            return 0x770000;
    }
    return 0x000000;
}

uint32_t get_foreground_color() {
    return 0xffffff;
}

IRAM_ATTR static bool test_notify_refresh_ready(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx)
{
    BaseType_t need_yield = pdFALSE;

    xSemaphoreGiveFromISR(refresh_finish, &need_yield);
    return (need_yield == pdTRUE);
}

static void periodic_timer_callback(void* arg) {

    lvgl_port_lock(0);
    lv_obj_t *scr = lv_disp_get_scr_act(disp);
    lv_obj_set_style_bg_color(scr, lv_color_hex(get_background_color()), LV_PART_MAIN);
    lv_obj_set_style_text_color(scr, lv_color_hex(get_foreground_color()), LV_PART_MAIN);

    measurements_t measurements = getMeasurements();
    sprintf(temp, "%d°C", measurements.temperature);
    sprintf(hum, "%d%%", measurements.humidity);
    sprintf(ppm, "%.1f", measurements.pm25Level);

    lv_label_set_text(tmp_label, temp);
    lv_label_set_text(hum_label, hum);
    lv_label_set_text(ppm_label, ppm);
    
    strcpy(centerText, getOpModeString());
    strcat(centerText, " - ");
    strcat(centerText, getStatusString());
    lv_label_set_text(status_label, centerText);
    
    strcpy(ip_address, get_ip_address());
    lv_label_set_text(ip_address_label, ip_address);
    lvgl_port_unlock();
}

void create_ui_components(lv_disp_t *disp)
{
    
    measurements_t measurements = getMeasurements();
    sprintf(temp, "%d°C", measurements.temperature);
    sprintf(hum, "%d%%", measurements.humidity);
    sprintf(ppm, "%.1f", measurements.pm25Level);

    lv_obj_t *scr = lv_disp_get_scr_act(disp);
    lv_obj_set_style_bg_color(scr, lv_color_hex(get_background_color()), LV_PART_MAIN);
    lv_obj_set_style_text_color(scr, lv_color_hex(get_foreground_color()), LV_PART_MAIN);

    lv_obj_set_style_pad_top(scr, 20, LV_PART_MAIN);
    lv_obj_set_style_pad_bottom(scr, 20, LV_PART_MAIN);
    lv_obj_set_style_pad_left(scr, 20, LV_PART_MAIN);
    lv_obj_set_style_pad_right(scr, 20, LV_PART_MAIN);

    tmp_label = lv_label_create(scr);
    lv_label_set_text(tmp_label, temp);
    lv_obj_align(tmp_label, LV_ALIGN_TOP_LEFT, 0, 0);

    hum_label = lv_label_create(scr);
    lv_label_set_text(hum_label, hum);
    lv_obj_align(hum_label, LV_ALIGN_TOP_RIGHT, 0, 0);

    ppm_label = lv_label_create(scr);
    lv_label_set_text(ppm_label, ppm);
    lv_obj_align(ppm_label, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_text_font(ppm_label, &lv_font_montserrat_36, LV_PART_MAIN);


    strcat(centerText, getOpModeString());
    strcat(centerText, " - ");
    strcat(centerText, getStatusString());
    
    status_label = lv_label_create(scr);
    lv_label_set_text(status_label, centerText);
    lv_obj_set_style_text_font(status_label, &lv_font_montserrat_48, LV_PART_MAIN);

    lv_obj_align(status_label, LV_ALIGN_CENTER, 0, 0);

    strcpy(ip_address, get_ip_address());
    
    ip_address_label = lv_label_create(scr);
    lv_label_set_text(ip_address_label, ip_address);
    lv_obj_align(ip_address_label, LV_ALIGN_BOTTOM_MID, 0, 0);
}

void display_start(void)
{

    ESP_LOGI(TAG, "Turn on the backlight");
    gpio_config_t io_conf = {
        .pin_bit_mask = BIT64(TEST_PIN_NUM_LCD_BL),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);
    gpio_set_level(TEST_PIN_NUM_LCD_BL, 1);

    ESP_LOGI(TAG, "Initialize SPI bus");
    const spi_bus_config_t buscfg = ILI9341_PANEL_BUS_SPI_CONFIG(TEST_PIN_NUM_LCD_PCLK, TEST_PIN_NUM_LCD_DATA0,
                                    TEST_LCD_H_RES * 80 * TEST_LCD_BIT_PER_PIXEL / 8);
    ESP_ERROR_CHECK(spi_bus_initialize(TEST_LCD_HOST, &buscfg, SPI_DMA_CH_AUTO));

    ESP_LOGI(TAG, "Install panel IO");
    esp_lcd_panel_io_handle_t io_handle = NULL;
    const esp_lcd_panel_io_spi_config_t io_config = ILI9341_PANEL_IO_SPI_CONFIG(TEST_PIN_NUM_LCD_CS, TEST_PIN_NUM_LCD_DC,
            test_notify_refresh_ready, NULL);
    // Attach the LCD to the SPI bus
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)TEST_LCD_HOST, &io_config, &io_handle));

    ESP_LOGI(TAG, "Install ili9341 panel driver");
    esp_lcd_panel_handle_t panel_handle = NULL;
    const esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = TEST_PIN_NUM_LCD_RST, // Shared with Touch reset
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR,
        .bits_per_pixel = TEST_LCD_BIT_PER_PIXEL,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_ili9341(io_handle, &panel_config, &panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_handle, true, true));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

    ESP_LOGI(TAG, "Initialize LVGL");
    const lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    lvgl_port_init(&lvgl_cfg);

    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = io_handle,
        .panel_handle = panel_handle,
        .buffer_size = (TEST_LCD_H_RES * TEST_LCD_V_RES) / 10,
        .double_buffer = true,
        .hres = TEST_LCD_H_RES,
        .vres = TEST_LCD_V_RES,
        .monochrome = false,
        .flags.buff_dma = true,
        .rotation = {
            .swap_xy = true,
            .mirror_x = false,
            .mirror_y = false,
        }
    };
    disp = lvgl_port_add_disp(&disp_cfg);
    disp->driver->antialiasing = 0;

    /* Rotation of the screen */
    lv_disp_set_rotation(disp, LV_DISPLAY_ROTATION_0);

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
    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, 100000));
}