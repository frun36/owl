#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include "esp_log.h"

#include "owl_button.h"
#include "owl_led.h"
#include "owl_onewire.h"
#include "owl_softap.h"

#include "nvs_flash.h"

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
    char response_buff[MAX_ONEWIRE_DEVICES * 17 + 1]; // 16 char address, newline, null terminator
    size_t count;

    while (1) {
        char *response_ptr = response_buff;
        if (xQueueReceive(button_event_queue, &e, portMAX_DELAY)) {
            switch (e) {
                case OWL_BUTTON_SINGLE_CLICK:
                    count = onewire_search(address_buff, MAX_ONEWIRE_DEVICES);

                    for (size_t i = 0; i < count; i++) {
                        ESP_LOGI(TAG, "Found device #%zu: %llX", i, address_buff[i]);
                        response_ptr += sprintf(response_ptr, "%llX\n", address_buff[i]);
                    }
                    *response_ptr = '\0';
                    ws_send(response_buff);
                    break;
                case OWL_BUTTON_DOUBLE_CLICK:
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

static void init_nvs()
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
}

void app_main(void)
{
    ESP_LOGI(TAG, "Helou");
    configure_led();
    configure_onewire(ONEWIRE_BUS_GPIO);
    led_off();
    owl_init_button(BUTTON_GPIO);
    init_nvs();
    init_softap_server();

    xTaskCreate(owl_task, "owl_task", 4096, NULL, 5, NULL);
}
