#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include "esp_log.h"

#include "freertos/projdefs.h"
#include "onewire_types.h"
#include "owl_button.h"
#include "owl_display.h"
#include "owl_http_server.h"
#include "owl_led.h"
#include "owl_onewire.h"
#include "owl_wifi.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "portmacro.h"

#define BUTTON_GPIO CONFIG_OWL_BUTTON_GPIO
#define ONEWIRE_BUS_GPIO CONFIG_OWL_ONEWIRE_BUS_GPIO

#define MAX_ONEWIRE_DEVICES 1
#define MAX_STORED_ADDRESSES 32

static const char *TAG = "owl";

static void owl_task(void *arg)
{

    struct {
        onewire_device_address_t address_buff[MAX_STORED_ADDRESSES];
        size_t n;
    } storage = { .n = 0 };

    // 16 char address, newline, null terminator
    char response_buff[17 * MAX_STORED_ADDRESSES + 1];

    while (1) {
        owl_button_event_t e;
        size_t count = 0;
        char disp_line0[17] = {};
        char disp_line1[17] = {};
        bool sent = false;

        if (xQueueReceive(owl_button_event_queue, &e, portMAX_DELAY)) {
            switch (e) {
            case OWL_BUTTON_SINGLE_CLICK:
                // Read OneWire address, store or send them
                if (storage.n + MAX_ONEWIRE_DEVICES >= MAX_STORED_ADDRESSES) {
                    ESP_LOGE(TAG,
                             "No space in storage for next OneWire address");
                    owl_display("NO SPACE", "", owl_rgb(OWL_COLOR_RED), 5000);
                } else {
                    owl_led_on();
                    count = owl_onewire_search(storage.address_buff + storage.n,
                                               MAX_ONEWIRE_DEVICES);
                    owl_led_off();

                    for (size_t i = 0; i < count; i++) {
                        ESP_LOGI(TAG,
                                 "Found device #%zu: %" PRIX64,
                                 i,
                                 storage.address_buff[storage.n]);

                        // If more devices were detected, would only display the
                        // last one
                        snprintf(disp_line0,
                                 17,
                                 "%" PRIX64,
                                 storage.address_buff[storage.n]);

                        storage.n++;
                    }
                }

                if (owl_ws_is_connected()) {
                    char *response_ptr = response_buff;
                    for (size_t i = 0; i < storage.n; i++) {
                        response_ptr += sprintf(response_ptr,
                                                "%" PRIX64 "\n",
                                                storage.address_buff[i]);
                    }
                    *response_ptr = '\0';
                    sent = owl_ws_send(response_buff);
                }

                if (sent) {
                    snprintf(disp_line1, 17, "SENT %zu", storage.n);
                    storage.n = 0;
                } else {
                    snprintf(disp_line1, 17, "STORED %zu", count);
                }

                owl_display(
                    disp_line0, disp_line1, owl_rgb(OWL_COLOR_CYAN), 5000);

                break;
            case OWL_BUTTON_DOUBLE_CLICK:
                // Move to STA WiFi mode
                owl_led_blink(10);
                vTaskDelay(pdMS_TO_TICKS(50));
                owl_wifi_sta();
                owl_led_blink_off();
                break;
            case OWL_BUTTON_LONG_PRESS:
                // Move to AP WiFi mode
                owl_wifi_ap();
                owl_led_blink(500);
                break;
            default:
                ESP_LOGW(TAG, "Unexpected button event");
            }
        }
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "Helou");
    owl_led_init();
    owl_display_init();
    owl_onewire_init(ONEWIRE_BUS_GPIO);
    owl_button_init(BUTTON_GPIO);

    owl_wifi_init();
    owl_wifi_configure();
    owl_wifi_sta();
    owl_http_server_init();

    xTaskCreate(owl_task, "owl_task", 4096, NULL, 5, NULL);
}
