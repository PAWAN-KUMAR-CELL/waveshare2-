#include "buttons.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_lvgl_port.h"
#include "lvgl.h"
#include "ui.h"
#include "lvglui/vars.h"
#include "lvglui/screens.h"
#include "flow_bridge.h"
#include "screen_watcher.h"

static const char *TAG = "BUTTONS";

// --- Physical buttons ---
#define PHYSICAL_NEXT_BTN_GPIO  GPIO_NUM_0   // BOOT button (active LOW) -> cycles highlight
#define PHYSICAL_OK_BTN_GPIO    GPIO_NUM_10  // TTP223 touch module (active HIGH) -> selects

// ─────────────────────────────────────────────────────────────
// Per-screen button mapping.
//   appdrawer1  -> nextbtn / okbtn
//   appdrawer2  -> next_1  / ok_1
//   gardenscreen -> gardennext / gardenok
// All other screens: NULL (no button behavior wired yet)
// ─────────────────────────────────────────────────────────────

static lv_obj_t *resolve_next_obj(void)
{
    switch (screen_watcher_get_current()) {
        case SCREEN_ID_APPDRAWER1:
            return objects.nextbtn;

        case SCREEN_ID_APPDRAWER2:
            return objects.next_1;

        case SCREEN_ID_GARDENSCREEN:
            return objects.gardennext;

        default:
            return NULL;
    }
}

static lv_obj_t *resolve_ok_obj(void)
{
    switch (screen_watcher_get_current()) {
        case SCREEN_ID_APPDRAWER1:
            return objects.okbtn;

        case SCREEN_ID_APPDRAWER2:
            return objects.ok_1;

        case SCREEN_ID_GARDENSCREEN:
            return objects.gardenok;

        default:
            return NULL;
    }
}

// --- NEXT: fires a click event on whichever object the active screen maps to ---
static void trigger_next_button(void)
{
    lvgl_port_lock(0);

    lv_obj_t *target = resolve_next_obj();
    if (target != NULL) {
        lv_obj_send_event(target, LV_EVENT_SHORT_CLICKED, NULL);
        // Flow's SetVariable node runs on the next ui_tick(), so any variable
        // it updates (e.g. CURRENT_BTN) may lag this call by one tick.
    }

    lvgl_port_unlock();
}

// --- OK: fires a click event on whichever object the active screen maps to ---
static void trigger_ok_button(void)
{
    lvgl_port_lock(0);

    lv_obj_t *target = resolve_ok_obj();
    if (target != NULL) {
        lv_obj_send_event(target, LV_EVENT_SHORT_CLICKED, NULL);
    }

    lvgl_port_unlock();
}

typedef void (*button_cb_t)(void);

typedef struct {
    gpio_num_t gpio;
    button_cb_t on_press;
    bool active_high; // true = TTP223 touch module, false = simple push button to GND
} button_task_arg_t;

// --- Generic debounced button poller, supports both active-low and active-high inputs ---
static void physical_button_task(void *pvParameter)
{
    button_task_arg_t *arg = (button_task_arg_t *)pvParameter;
    gpio_num_t gpio = arg->gpio;
    button_cb_t on_press = arg->on_press;
    bool active_high = arg->active_high;

    gpio_config_t io_conf = {
        .pin_bit_mask = 1ULL << gpio,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = active_high ? GPIO_PULLUP_DISABLE : GPIO_PULLUP_ENABLE,
        .pull_down_en = active_high ? GPIO_PULLDOWN_ENABLE : GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&io_conf));

    int idle_level = active_high ? 0 : 1;
    int pressed_level = active_high ? 1 : 0;
    int last_level = idle_level;

    while (true) {
        int level = gpio_get_level(gpio);

        if (last_level == idle_level && level == pressed_level) {
            vTaskDelay(pdMS_TO_TICKS(30)); // debounce
            if (gpio_get_level(gpio) == pressed_level) {
                ESP_LOGI(TAG, "GPIO %d press detected", gpio);
                on_press();
                while (gpio_get_level(gpio) == pressed_level) {
                    vTaskDelay(pdMS_TO_TICKS(10));
                }
            }
        }

        last_level = level;
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void buttons_init(void)
{
    eez_set_global_variable_int(FLOW_GLOBAL_VARIABLE_CURRENT_BTN, 0);

    static button_task_arg_t next_btn_arg = {
        .gpio = PHYSICAL_NEXT_BTN_GPIO,
        .on_press = trigger_next_button,
        .active_high = false,
    };
    static button_task_arg_t ok_btn_arg = {
        .gpio = PHYSICAL_OK_BTN_GPIO,
        .on_press = trigger_ok_button,
        .active_high = true,
    };

    xTaskCreate(physical_button_task, "next_btn_task", 4096, &next_btn_arg, 5, NULL);
    xTaskCreate(physical_button_task, "ok_btn_task", 4096, &ok_btn_arg, 5, NULL);
}