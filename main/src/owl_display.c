#include "owl_display.h"
#include "esp_wifi.h"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include "owl_http_server.h"
#include "owl_lcd.h"
#include "owl_wifi.h"
#include "portmacro.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

QueueHandle_t owl_display_event_queue;

void owl_display_update_status()
{
    owl_wifi_status_t wifi_status = owl_wifi_get_status();
    bool is_ws_connected = owl_ws_is_connected();

    wifi_config_t conf;

    char line0[17] = {};
    char line1[17] = {};
    owl_rgb_t color = owl_rgb(OWL_COLOR_WHITE);
    switch (wifi_status) {
    case OWL_WIFI_DISCONNECTED:
        esp_wifi_get_config(WIFI_IF_STA, &conf);
        snprintf(line0, 17, "OWL           NC");
        strncpy(line1, (const char *) conf.sta.ssid, 16);
        color = owl_rgb(OWL_COLOR_RED);
        break;
    case OWL_WIFI_STA:
        esp_wifi_get_config(WIFI_IF_STA, &conf);
        snprintf(line0, 17, "OWL         STA%c", is_ws_connected ? '+' : '-');
        strncpy(line1, owl_wifi_get_ip_str(), 16);
        color = owl_rgb(OWL_COLOR_WHITE);
        break;
    case OWL_WIFI_AP:
        esp_wifi_get_config(WIFI_IF_AP, &conf);
        snprintf(line0, 17, "OWL          AP%c", is_ws_connected ? '+' : '-');
        strncpy(line1, (const char *) conf.ap.password, 16);
        color = owl_rgb(OWL_COLOR_YELLOW);
        break;
    case OWL_WIFI_UNKNOWN:
    default:
        snprintf(line0, 17, "OWL            ?");
        color = owl_rgb(OWL_COLOR_MAGENTA);
    }

    owl_display(line0, line1, color, -1);
}

static void owl_display_task(void *arg)
{
    owl_display_event_t e;

    // clang-format off
    owl_display_event_t status = (owl_display_event_t) {
        .message = { 
            { 'O', 'W', 'L', '\0' },
            { 'H', 'e', 'l', 'o', 'u', '\0' }
        },
        .color = owl_rgb(OWL_COLOR_WHITE),
        .duration_ms = -1,
    };
    // clang-format on

    while (1) {
#ifdef CONFIG_OWL_USE_LCD
        if (xQueueReceive(owl_display_event_queue, &e, portMAX_DELAY)) {
            if (e.duration_ms != -1) {
                owl_lcd_set_backlight(e.color);
                owl_lcd_write(0, e.message[0]);
                owl_lcd_write(1, e.message[1]);
                vTaskDelay(pdMS_TO_TICKS(e.duration_ms));
            } else {
                status = e;
            }

            if (uxQueueMessagesWaiting(owl_display_event_queue) == 0) {
                owl_lcd_set_backlight(status.color);
                owl_lcd_write(0, status.message[0]);
                owl_lcd_write(1, status.message[1]);
            }
        }
#endif
    }
}

void owl_display_init()
{
#ifdef CONFIG_OWL_USE_LCD
    owl_lcd_init();
#endif

    owl_display_event_queue = xQueueCreate(2, sizeof(owl_display_event_t));
    xTaskCreate(owl_display_task, "owl_display_task", 4096, NULL, 5, NULL);
}

void owl_display(const char *line0,
                 const char *line1,
                 owl_rgb_t color,
                 int duration_ms)
{
    owl_display_event_t e = { {}, color, duration_ms };
    strncpy(e.message[0], line0, 17);
    strncpy(e.message[1], line1, 17);
    xQueueSend(owl_display_event_queue, &e, 0);
}
