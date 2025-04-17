#include <assert.h>
#include <inttypes.h>

#include "esp_log.h"

#include "owl_onewire.h"

#include "owl_led.h"

#include "button_gpio.h"
#include "iot_button.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define BUTTON_GPIO 22
#define ONEWIRE_BUS_GPIO 23

static const char *TAG = "owl";

static void button_single_click_cb(void *arg, void *usr_data)
{
    onewire_device_address_t buff[16];
    size_t count = onewire_search(buff, 16);

    for (size_t i = 0; i < count; i++) {
        ESP_LOGI(TAG, "Found device #%zu: %llX", i, buff[i]);
    }
}

static void button_double_click_cb(void *arg, void *usr_data)
{
    ESP_LOGW(TAG, "BUTTON_DOUBLE_CLICK");
}

static void button_long_press_cb(void *arg, void *usr_data)
{
    ESP_LOGW(TAG, "BUTTON_LONG_PRESS_START");
}

void app_main(void)
{
    ESP_LOGI(TAG, "Helou");
    configure_led();
    configure_onewire(ONEWIRE_BUS_GPIO);
    led_off();

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
}
