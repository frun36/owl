#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include "esp_log.h"

#include "owl_button.h"
#include "owl_http_server.h"
#include "owl_led.h"
#include "owl_onewire.h"
#include "owl_wifi.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "portmacro.h"

#define BUTTON_GPIO 22
#define ONEWIRE_BUS_GPIO 23
#define MAX_ONEWIRE_DEVICES 4

static const char *TAG = "owl";

static void owl_task(void *arg)
{
    owl_button_event_t e;
    onewire_device_address_t address_buff[MAX_ONEWIRE_DEVICES];
    char response_buff[MAX_ONEWIRE_DEVICES * 17
                       + 1]; // 16 char address, newline, null terminator
    size_t count;

    static int mode = 0;

    while (1) {
        char *response_ptr = response_buff;
        if (xQueueReceive(owl_button_event_queue, &e, portMAX_DELAY)) {
            switch (e) {
            case OWL_BUTTON_SINGLE_CLICK:
                count = owl_onewire_search(address_buff, MAX_ONEWIRE_DEVICES);

                for (size_t i = 0; i < count; i++) {
                    ESP_LOGI(
                        TAG, "Found device #%zu: %" PRIX64, i, address_buff[i]);
                    response_ptr += sprintf(
                        response_ptr, "%" PRIX64 "\n", address_buff[i]);
                }
                *response_ptr = '\0';
                owl_ws_send(response_buff);
                break;
            case OWL_BUTTON_DOUBLE_CLICK:
                if (mode) {
                    owl_start_softap();
                    mode = 0;
                } else {
                    owl_start_station();
                    mode = 1;
                }
                ESP_LOGW(TAG, "Double click");
                break;
            case OWL_BUTTON_LONG_PRESS:
                ESP_LOGW(TAG, "Long press");
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
    owl_init_led();
    owl_init_onewire(ONEWIRE_BUS_GPIO);
    owl_init_button(BUTTON_GPIO);

    owl_init_wifi();
    owl_start_softap();
    // owl_start_station();
    owl_init_http_server();

    xTaskCreate(owl_task, "owl_task", 4096, NULL, 5, NULL);
}
