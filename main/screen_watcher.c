#include "screen_watcher.h"
#include "garden_status.h"
#include "lvglui/screens.h"
#include "lvgl.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static int current_screen_id = 0;

static int detect_active_screen(void)
{
    lv_obj_t *active = lv_screen_active();

    if (active == objects.appdrawer1)    return SCREEN_ID_APPDRAWER1;
    if (active == objects.appdrawer2)    return SCREEN_ID_APPDRAWER2;
    if (active == objects.camera_screen) return SCREEN_ID_CAMERA_SCREEN;
    if (active == objects.stopwatchscreen) return SCREEN_ID_STOPWATCHSCREEN;
    if (active == objects.gardenscreen)  return SCREEN_ID_GARDENSCREEN;
    if (active == objects.musicscreen)   return SCREEN_ID_MUSICSCREEN;
    if (active == objects.settingscreen) return SCREEN_ID_SETTINGSCREEN;
    if (active == objects.clockscreen)   return SCREEN_ID_CLOCKSCREEN;

    return 0;
}

// --- Run blocking start/stop OUTSIDE the LVGL lock, in their own short-lived task ---
static void garden_start_task(void *arg) {
    garden_status_start();
    vTaskDelete(NULL);
}

static void garden_stop_task(void *arg) {
    garden_status_stop();
    vTaskDelete(NULL);
}

static void on_screen_enter(int screen_id)
{
    switch (screen_id) {
        case SCREEN_ID_GARDENSCREEN:
            xTaskCreate(garden_start_task, "garden_start", 8192, NULL, 3, NULL);
            break;

        case SCREEN_ID_APPDRAWER1:
        case SCREEN_ID_APPDRAWER2:
        case SCREEN_ID_CAMERA_SCREEN:
        case SCREEN_ID_STOPWATCHSCREEN:
        case SCREEN_ID_MUSICSCREEN:
        case SCREEN_ID_SETTINGSCREEN:
        case SCREEN_ID_CLOCKSCREEN:
            break;

        default:
            break;
    }
}

static void on_screen_exit(int screen_id)
{
    switch (screen_id) {
        case SCREEN_ID_GARDENSCREEN:
            xTaskCreate(garden_stop_task, "garden_stop", 4096, NULL, 3, NULL);
            break;

        case SCREEN_ID_APPDRAWER1:
        case SCREEN_ID_APPDRAWER2:
        case SCREEN_ID_CAMERA_SCREEN:
        case SCREEN_ID_STOPWATCHSCREEN:
        case SCREEN_ID_MUSICSCREEN:
        case SCREEN_ID_SETTINGSCREEN:
        case SCREEN_ID_CLOCKSCREEN:
            break;

        default:
            break;
    }
}

void screen_watcher_check(void)
{
    int detected = detect_active_screen();

    if (detected != current_screen_id) {
        if (current_screen_id != 0) on_screen_exit(current_screen_id);
        if (detected != 0) on_screen_enter(detected);
        current_screen_id = detected;
    }
}

int screen_watcher_get_current(void)
{
    return current_screen_id;
}