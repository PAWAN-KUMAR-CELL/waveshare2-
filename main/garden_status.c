#include "garden_status.h"
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include "esp_crt_bundle.h"
#include "esp_websocket_client.h"
#include "nvs_flash.h"
#include "esp_bus.h"
#include "cJSON.h"
#include "lvglui/vars.h"
#include "flow_bridge.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "screen_watcher.h"
#include "esp_lvgl_port.h"

#define HTTP_URL "https://garden-server-j643.onrender.com/api/data?limit=1"
#define WS_URL   "wss://garden-server-j643.onrender.com"
#define WIFI_SSID     "iPhone"
#define WIFI_PASSWORD "11111111"

static const char *TAG = "GARDEN_STATUS";
static bool wifi_connected = false;
static bool garden_screen_active = false;   // NEW: gate flag

static EventGroupHandle_t s_wifi_event_group;
#define WIFI_CONNECTED_BIT BIT0

static esp_websocket_client_handle_t ws_client = NULL;

// ── Parse one reading object and push to LVGL vars ──
static void apply_reading(cJSON *reading) {
    if (!reading) return;
    if (!garden_screen_active) return;

    cJSON *m = cJSON_GetObjectItem(reading, "moisture");
    cJSON *t = cJSON_GetObjectItem(reading, "temp");
    cJSON *h = cJSON_GetObjectItem(reading, "humidity");
    cJSON *p = cJSON_GetObjectItem(reading, "pump");

    int moisture = m ? m->valueint : 0;
    int temp     = t ? (int)t->valuedouble : 0;
    int humidity = h ? h->valueint : 0;
    int pump     = p ? p->valueint : 0;

    ESP_LOGI(TAG, "moisture=%d temp=%d humidity=%d pump=%d",
             moisture, temp, humidity, pump);

    lvgl_port_lock(0);   // lock only around the actual UI/flow variable writes
    eez_set_global_variable_int(FLOW_GLOBAL_VARIABLE_MOISTURE,   moisture);
    eez_set_global_variable_int(FLOW_GLOBAL_VARIABLE_TEMP,       temp);
    eez_set_global_variable_int(FLOW_GLOBAL_VARIABLE_HUMIDITY,   humidity);
    eez_set_global_variable_int(FLOW_GLOBAL_VARIABLE_START_PUMP, pump);
    lvgl_port_unlock();
}

// ── One-time HTTP fetch to get the current value immediately ──
static char http_buf[512];
static int  http_len = 0;

static esp_err_t http_event_handler(esp_http_client_event_t *evt) {
    if (evt->event_id == HTTP_EVENT_ON_DATA) {
        int copy_len = evt->data_len;
        if (http_len + copy_len < (int)sizeof(http_buf) - 1) {
            memcpy(http_buf + http_len, evt->data, copy_len);
            http_len += copy_len;
            http_buf[http_len] = '\0';
        }
    }
    return ESP_OK;
}

static void fetch_initial_reading(void) {
    http_len = 0;
    memset(http_buf, 0, sizeof(http_buf));

    esp_http_client_config_t config = {
        .url               = HTTP_URL,
        .event_handler     = http_event_handler,
        .timeout_ms        = 5000,
        .crt_bundle_attach = esp_crt_bundle_attach,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK && esp_http_client_get_status_code(client) == 200) {
        cJSON *root = cJSON_Parse(http_buf);
        if (root && cJSON_IsArray(root)) {
            apply_reading(cJSON_GetArrayItem(root, 0));
        }
        if (root) cJSON_Delete(root);
    } else {
        ESP_LOGW(TAG, "Initial fetch failed: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
}

// ── WebSocket event handler ──
static void websocket_event_handler(void *handler_args, esp_event_base_t base,
                                     int32_t event_id, void *event_data)
{
    esp_websocket_event_data_t *data = (esp_websocket_event_data_t *)event_data;

    switch (event_id) {
        case WEBSOCKET_EVENT_CONNECTED:
            ESP_LOGI(TAG, "WebSocket connected");
            break;

        case WEBSOCKET_EVENT_DISCONNECTED:
            ESP_LOGW(TAG, "WebSocket disconnected");
            break;

        case WEBSOCKET_EVENT_DATA:
            if (data->op_code == 0x01 && data->data_len > 0) {
                cJSON *reading = cJSON_ParseWithLength(data->data_ptr, data->data_len);
                if (reading) {
                    apply_reading(reading);
                    cJSON_Delete(reading);
                } else {
                    ESP_LOGW(TAG, "Failed to parse WS message");
                }
            }
            break;

        case WEBSOCKET_EVENT_ERROR:
            ESP_LOGW(TAG, "WebSocket error event");
            break;

        default:
            break;
    }
}

// ── WiFi event handler ──
static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                                int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGW(TAG, "WiFi disconnected, retrying...");
        wifi_connected = false;
        vTaskDelay(pdMS_TO_TICKS(1000));
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        wifi_connected = true;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

static void wifi_init_hardcoded(void)
{
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid     = WIFI_SSID,
            .password = WIFI_PASSWORD,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Connecting to %s ...", WIFI_SSID);

    xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT,
                         pdFALSE, pdTRUE, portMAX_DELAY);
}

// ── Task: waits for WiFi once, then just idles — start/stop are called externally ──
static void wifi_wait_task(void *arg) {
    wifi_init_hardcoded();
    ESP_LOGI(TAG, "WiFi ready, garden module armed (waiting for screen activation)");
    vTaskDelete(NULL);
}

// ── PUBLIC: call once from app_main — just brings up WiFi, does NOT start fetching ──
void garden_status_init(void) {
    ESP_ERROR_CHECK(nvs_flash_init());
    esp_bus_init();
    xTaskCreate(wifi_wait_task, "wifi_wait", 8192, NULL, 3, NULL);
}

// ── PUBLIC: call when garden screen becomes active ──
void garden_status_start(void) {
    if (garden_screen_active) return;   // already running
    if (!wifi_connected) {
        ESP_LOGW(TAG, "garden_status_start called before WiFi ready");
        return;
    }

    garden_screen_active = true;
    ESP_LOGI(TAG, "Garden screen active — starting fetch");

    fetch_initial_reading();

    if (ws_client == NULL) {
        esp_websocket_client_config_t ws_cfg = {
            .uri                  = WS_URL,
            .crt_bundle_attach    = esp_crt_bundle_attach,
            .reconnect_timeout_ms = 3000,
            .network_timeout_ms   = 5000,
        };
        ws_client = esp_websocket_client_init(&ws_cfg);
        esp_websocket_register_events(ws_client, WEBSOCKET_EVENT_ANY, websocket_event_handler, NULL);
    }

    if (!esp_websocket_client_is_connected(ws_client)) {
        esp_websocket_client_start(ws_client);
    }
}

// ── PUBLIC: call when garden screen becomes inactive ──
void garden_status_stop(void) {
    if (!garden_screen_active) return;   // already stopped

    garden_screen_active = false;
    ESP_LOGI(TAG, "Garden screen inactive — stopping fetch");

    if (ws_client != NULL && esp_websocket_client_is_connected(ws_client)) {
        esp_websocket_client_stop(ws_client);
    }
}