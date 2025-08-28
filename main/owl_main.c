#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include "esp_log.h"

#include "freertos/projdefs.h"
#include "owl_button.h"
#include "owl_display.h"
#include "owl_http_server.h"
#include "owl_lcd.h"
#include "owl_led.h"
#include "owl_onewire.h"
#include "owl_wifi.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "portmacro.h"

#define BUTTON_GPIO CONFIG_OWL_BUTTON_GPIO
#define ONEWIRE_BUS_GPIO CONFIG_OWL_ONEWIRE_BUS_GPIO

#define MAX_ONEWIRE_DEVICES 1

static const char *TAG = "owl";

static void owl_task(void *arg)
{
    owl_button_event_t e;
    onewire_device_address_t address_buff[MAX_ONEWIRE_DEVICES];
    // 16 char address, newline, null terminator
    char response_buff[MAX_ONEWIRE_DEVICES * 17 + 1];
    size_t count;

    while (1) {
        char *response_ptr = response_buff;
        if (xQueueReceive(owl_button_event_queue, &e, portMAX_DELAY)) {
            switch (e) {
            case OWL_BUTTON_SINGLE_CLICK:
                owl_led_on();
                count = owl_onewire_search(address_buff, MAX_ONEWIRE_DEVICES);
                owl_led_off();

                for (size_t i = 0; i < count; i++) {
                    ESP_LOGI(
                        TAG, "Found device #%zu: %" PRIX64, i, address_buff[i]);
                    response_ptr += sprintf(
                        response_ptr, "%" PRIX64 "\n", address_buff[i]);

                    char disp_buff[17];
                    snprintf(disp_buff, 17, "%" PRIX64, address_buff[i]);
                    owl_display(
                        "OneWire:", disp_buff, owl_rgb(OWL_COLOR_WHITE), 5000);
                }

                *response_ptr = '\0';
                owl_ws_send(response_buff);
                break;
            case OWL_BUTTON_DOUBLE_CLICK:
                owl_led_blink(10);
                vTaskDelay(pdMS_TO_TICKS(50));
                owl_wifi_sta();
                owl_led_blink_off();
                break;
            case OWL_BUTTON_LONG_PRESS:
                owl_apsta();
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
