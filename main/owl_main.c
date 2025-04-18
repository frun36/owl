#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include "esp_log.h"

#include "owl_led.h"
#include "owl_onewire.h"
#include "owl_softap.h"

#include "button_gpio.h"
#include "iot_button.h"

#include "nvs_flash.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "portmacro.h"

#define BUTTON_GPIO 22
#define ONEWIRE_BUS_GPIO 23
#define MAX_ONEWIRE_DEVICES 4

static const char *TAG = "owl";

static QueueHandle_t button_event_queue;

static void button_single_click_cb(void *arg, void *usr_data)
{
    int code = 1;
    xQueueSend(button_event_queue, &code, portMAX_DELAY);
}

static void button_double_click_cb(void *arg, void *usr_data)
{
    ESP_LOGW(TAG, "BUTTON_DOUBLE_CLICK");
}

static void button_long_press_cb(void *arg, void *usr_data)
{
    ESP_LOGW(TAG, "BUTTON_LONG_PRESS_START");
}

static void owl_task(void *arg)
{
    int code;
    onewire_device_address_t address_buff[MAX_ONEWIRE_DEVICES];
    char response_buff[MAX_ONEWIRE_DEVICES * 17 + 1]; // 16 char address, newline, null terminator

    while (1) {
        char *response_ptr = response_buff;
        if (xQueueReceive(button_event_queue, &code, portMAX_DELAY)) {
            size_t count = onewire_search(address_buff, MAX_ONEWIRE_DEVICES);

            for (size_t i = 0; i < count; i++) {
                ESP_LOGI(TAG, "Found device #%zu: %llX", i, address_buff[i]);
                response_ptr += sprintf(response_ptr, "%llX\n", address_buff[i]);
            }
            *response_ptr = '\0';
            ws_send(response_buff);
        }
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "Helou");
    configure_led();
    configure_onewire(ONEWIRE_BUS_GPIO);
    led_off();

    button_event_queue = xQueueCreate(4, sizeof(int));
    button_config_t btn_cfg = {0};
    button_gpio_config_t gpio_cfg = {
        .gpio_num = BUTTON_GPIO,
        .active_level = 0,
        .enable_power_save = false,
    };

    button_handle_t btn;
    esp_err_t ret = iot_button_new_gpio_device(&btn_cfg, &gpio_cfg, &btn);
    assert(ret == ESP_OK);

    iot_button_register_cb(btn, BUTTON_SINGLE_CLICK, NULL, button_single_click_cb, NULL);
    iot_button_register_cb(btn, BUTTON_DOUBLE_CLICK, NULL, button_double_click_cb, NULL);
    iot_button_register_cb(btn, BUTTON_LONG_PRESS_START, NULL, button_long_press_cb, NULL);

    //Initialize NVS
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "ESP_WIFI_MODE_AP");
    init_softap_server();

    xTaskCreate(owl_task, "owl_task", 4096, NULL, 5, NULL);
}
