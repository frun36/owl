#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_log.h"

#include "onewire_bus.h"
#include "onewire_device.h"
#include "onewire_types.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

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

#define ECHO_TEST_TXD 1
#define ECHO_TEST_RXD 3
#define ECHO_UART_PORT_NUM 0
#define BUF_SIZE 2048
#define ECHO_TEST_RTS UART_PIN_NO_CHANGE
#define ECHO_TEST_CTS UART_PIN_NO_CHANGE
#define ECHO_TASK_STACK_SIZE 4096

static void handle_uart_command(const char *data, size_t n)
{
    if (n == 0) {
        ESP_LOGE(TAG, "Received empty command");
    } else if (strncmp(data, "run", n) == 0) {
        onewire_search();
    } else {
        ESP_LOGE(TAG, "Unsupported command");
    }
}

static void get_line(char *const buf, size_t len)
{
    char *ptr = buf;
    fpurge(stdin);
    while (1) {
        vTaskDelay(100 / portTICK_PERIOD_MS);
        *ptr = getc(stdin);

        switch (*ptr) {
        case '\0':
        case (char) 0xFF:
        case '\r':
            continue;

        case '\n':
            *ptr = '\0';
            return;

        case '\b':
            if (ptr != buf) {
                ptr--;
            }
            break;

        default:
            ptr++;
        }

        if (ptr - buf >= len - 1) {
            ptr++;
            *ptr = '\0';
            return;
        }
    }
}

static void uart_task(void *arg)
{
    char buf[256] = {};
    while (1) {
        get_line(buf, 256);
        handle_uart_command(buf, 256);
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "Helou");
    configure_led();
    configure_onewire();
    led_off();

    xTaskCreate(uart_task, "uart_task", ECHO_TASK_STACK_SIZE, NULL, 10, NULL);
}
