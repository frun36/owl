#include "esp_log.h"

#include "onewire_bus.h"
#include "onewire_device.h"
#include "onewire_types.h"

#define TAG "owl_onewire"

onewire_bus_handle_t s_bus;

onewire_bus_handle_t owl_init_onewire(int bus_gpio_num)
{
    onewire_bus_config_t bus_config = {
        .bus_gpio_num = bus_gpio_num,
    };
    onewire_bus_rmt_config_t rmt_config = {
        // 1byte ROM command + 8byte ROM number + 1byte device command
        .max_rx_bytes = 10,
    };
    ESP_ERROR_CHECK(onewire_new_bus_rmt(&bus_config, &rmt_config, &s_bus));
    ESP_LOGI(TAG, "1-Wire bus configured on GPIO%d", bus_gpio_num);
    return s_bus;
}

size_t owl_onewire_search(onewire_device_address_t buff[], size_t max_devices)
{
    onewire_device_iter_handle_t iter = NULL;
    onewire_device_t next_onewire_device;
    esp_err_t search_result = ESP_OK;
    size_t device_count = 0;
    onewire_device_address_t *buff_ptr = buff;

    ESP_ERROR_CHECK(onewire_new_device_iter(s_bus, &iter));
    while (1) {
        search_result
            = onewire_device_iter_get_next(iter, &next_onewire_device);
        if (search_result != ESP_OK) {
            break;
        }

        if (device_count >= max_devices) {
            ESP_LOGW(TAG, "Reached max device count: aborting");
            break;
        }

        *buff_ptr++ = next_onewire_device.address;
        device_count++;
    }

    ESP_ERROR_CHECK(onewire_del_device_iter(iter));
    return device_count;
}
