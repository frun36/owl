#include <inttypes.h>

#include "driver/gpio.h"
#include "esp_log.h"
#include "onewire_bus.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "onewire_device.h"
#include "onewire_types.h"

#define BOARD_LED 2
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

static onewire_bus_handle_t configure_onewire()
{
    onewire_bus_handle_t bus;
    onewire_bus_config_t bus_config = {
        .bus_gpio_num = ONEWIRE_BUS,
    };
    onewire_bus_rmt_config_t rmt_config = {
        .max_rx_bytes = 10, // 1byte ROM command + 8byte ROM number + 1byte device command
    };
    ESP_ERROR_CHECK(onewire_new_bus_rmt(&bus_config, &rmt_config, &bus));
    ESP_LOGI(TAG, "1-Wire bus configured on GPIO%d", ONEWIRE_BUS);
    return bus;
}

static void onewire_search(onewire_bus_handle_t bus)
{
    onewire_device_iter_handle_t iter = NULL;
    onewire_device_t next_onewire_device;
    esp_err_t search_result = ESP_OK;
    uint32_t device_count = 0;

    ESP_ERROR_CHECK(onewire_new_device_iter(bus, &iter));
    ESP_LOGI(TAG, "1-Wire search...");
    do {
        search_result = onewire_device_iter_get_next(iter, &next_onewire_device);
        if (search_result == ESP_OK) {
            ESP_LOGI(TAG, "%016llX", next_onewire_device.address);
            device_count++;
        }
    } while (search_result != ESP_ERR_NOT_FOUND);
    ESP_LOGI(TAG, "Found %"PRIu32" devices", device_count);
    ESP_ERROR_CHECK(onewire_del_device_iter(iter));
}

void app_main(void)
{
    ESP_LOGI(TAG, "Helou");
    configure_led();
    led_off();

    onewire_bus_handle_t bus = configure_onewire();

    while (1) {
        led_on();
        onewire_search(bus);
        led_off();
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}
