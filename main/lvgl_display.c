#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_lvgl_port.h"
#include "lvgl.h"
#include "ui.h"
#include "lvglui/vars.h"
#include "lvglui/screens.h"
#include "garden_status.h"
#include "screen_watcher.h"
#include "buttons.h"

// --- Pin definitions ---
#define LCD_MOSI     38
#define LCD_SCLK     39
#define LCD_CS       45
#define LCD_DC       42
#define LCD_RST      -1
#define LCD_BL       1

// --- Display resolution ---
#define LCD_H_RES    240
#define LCD_V_RES    320

// --- SPI ---
#define LCD_SPI_HOST    SPI2_HOST
#define LCD_SPI_CLK_HZ  (40 * 1000 * 1000)

static const char *TAG = "MAIN";
static esp_lcd_panel_handle_t panel_handle = NULL;
static esp_lcd_panel_io_handle_t io_handle = NULL;
static lv_display_t *display = NULL;

// --- Drives the EEZ Flow queue (SetVariable, Watch, etc. only run via this) ---
static void flow_tick_task(void *pvParameter)
{
    while (true) {
        lvgl_port_lock(0);
        ui_tick();
        screen_watcher_check();
        lvgl_port_unlock();
        vTaskDelay(pdMS_TO_TICKS(33)); // ~60Hz
    }
}

// --- Display init ---
static void display_init(void)
{
    ESP_LOGI(TAG, "Backlight ON");
    gpio_set_direction(LCD_BL, GPIO_MODE_OUTPUT);
    gpio_set_level(LCD_BL, 1);

    ESP_LOGI(TAG, "SPI bus init");
    spi_bus_config_t buscfg = {
        .mosi_io_num     = LCD_MOSI,
        .miso_io_num     = -1,
        .sclk_io_num     = LCD_SCLK,
        .quadwp_io_num   = -1,
        .quadhd_io_num   = -1,
        .max_transfer_sz = LCD_H_RES * 20 * sizeof(uint16_t),
    };
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_SPI_HOST, &buscfg, SPI_DMA_CH_AUTO));

    ESP_LOGI(TAG, "LCD IO init");
    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num       = LCD_DC,
        .cs_gpio_num       = LCD_CS,
        .pclk_hz           = LCD_SPI_CLK_HZ,
        .lcd_cmd_bits      = 8,
        .lcd_param_bits    = 8,
        .spi_mode          = 0,
        .trans_queue_depth = 10,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(
        (esp_lcd_spi_bus_handle_t)LCD_SPI_HOST,
        &io_config,
        &io_handle
    ));

    ESP_LOGI(TAG, "ST7789 panel init");
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = LCD_RST,
        .rgb_endian     = LCD_RGB_ENDIAN_RGB,
        .bits_per_pixel = 16,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(io_handle, &panel_config, &panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

    ESP_LOGI(TAG, "Display init done");
}

// --- LVGL init via esp_lvgl_port ---
static void lvgl_init(void)
{
    ESP_LOGI(TAG, "LVGL port init");
    lvgl_port_cfg_t port_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    port_cfg.task_priority = 4;
    ESP_ERROR_CHECK(lvgl_port_init(&port_cfg));

    ESP_LOGI(TAG, "Adding display");
    const lvgl_port_display_cfg_t display_cfg = {
        .io_handle     = io_handle,
        .panel_handle  = panel_handle,
        .buffer_size   = LCD_H_RES * 20,
        .double_buffer = false,
        .hres          = LCD_H_RES,
        .vres          = LCD_V_RES,
        .monochrome    = false,
        .rotation = {
            .swap_xy  = false,
            .mirror_x = false,
            .mirror_y = false,
        },
        .flags = {
            .buff_dma  = 1,
            .swap_bytes = 1,
        },
    };
    display = lvgl_port_add_disp(&display_cfg);
    assert(display != NULL);

    ESP_LOGI(TAG, "LVGL init done");
}

void app_main(void)
{
    display_init();
    esp_lcd_panel_invert_color(panel_handle, true);
    lvgl_init();

    lvgl_port_lock(0);
    ui_init();   // flow engine starts here

    lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(lv_screen_active(), LV_OPA_COVER, 0);
    lvgl_port_unlock();

    // Start the Flow engine's tick driver — REQUIRED for SetVariable/Watch/etc. to ever run
    xTaskCreate(flow_tick_task, "flow_tick_task", 4096, NULL, 4, NULL);

    // Buttons (resets CURRENT_BTN, starts physical button poller tasks)
    buttons_init();

    // WiFi comes up in background; fetching only starts once garden screen is active
    garden_status_init();
}