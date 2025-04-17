#include <assert.h>
#include <inttypes.h>

#include "driver/gpio.h"
#include "esp_log.h"

#include "onewire_bus.h"
#include "onewire_device.h"
#include "onewire_types.h"

#include "button_gpio.h"
#include "iot_button.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define BOARD_LED 2
#define BUTTON 22
#define ONEWIRE_BUS 23

static const char *TAG = "owl";

static void configure_led(void)
{
    ESP_LOGI(TAG, "Configuring GPIO LED!");
    gpio_reset_pin(BOARD_LED);
    gpio_set_direction(BOARD_LED, GPIO_MODE_OUTPUT);
}

static void led_on(void)
{
    gpio_set_level(BOARD_LED, 1);
}

static void led_off(void)
{
    gpio_set_level(BOARD_LED, 0);
}

static onewire_bus_handle_t s_bus;

static onewire_bus_handle_t configure_onewire()
{
    onewire_bus_config_t bus_config = {
        .bus_gpio_num = ONEWIRE_BUS,
    };
    onewire_bus_rmt_config_t rmt_config = {
        .max_rx_bytes = 10, // 1byte ROM command + 8byte ROM number + 1byte device command
    };
    ESP_ERROR_CHECK(onewire_new_bus_rmt(&bus_config, &rmt_config, &s_bus));
    ESP_LOGI(TAG, "1-Wire bus configured on GPIO%d", ONEWIRE_BUS);
    return s_bus;
}

static void onewire_search()
{
    led_on();
    onewire_device_iter_handle_t iter = NULL;
    onewire_device_t next_onewire_device;
    esp_err_t search_result = ESP_OK;
    uint32_t device_count = 0;

    ESP_ERROR_CHECK(onewire_new_device_iter(s_bus, &iter));
    ESP_LOGI(TAG, "1-Wire search...");
    do {
        search_result = onewire_device_iter_get_next(iter, &next_onewire_device);
        if (search_result == ESP_OK) {
            ESP_LOGI(TAG, "%016llX", next_onewire_device.address);
            device_count++;
        }
    } while (search_result != ESP_ERR_NOT_FOUND);
    ESP_LOGI(TAG, "Found %" PRIu32 " devices", device_count);
    ESP_ERROR_CHECK(onewire_del_device_iter(iter));
    led_off();
}

static void button_single_click_cb(void *arg, void *usr_data)
{
    onewire_search();
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
    configure_onewire();
    led_off();

    button_config_t btn_cfg = {0};
    button_gpio_config_t gpio_cfg = {
        .gpio_num = BUTTON,
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
