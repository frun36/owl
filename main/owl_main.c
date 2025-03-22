#include <inttypes.h>
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

static void uart_task(void *arg)
{
    uart_config_t uart_config = {
        .baud_rate = 115200, // monitor won't work with other speeds!
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    int intr_alloc_flags = 0;

    ESP_ERROR_CHECK(uart_driver_install(ECHO_UART_PORT_NUM, BUF_SIZE * 2, 0, 0, NULL, intr_alloc_flags));
    ESP_ERROR_CHECK(uart_param_config(ECHO_UART_PORT_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(ECHO_UART_PORT_NUM, ECHO_TEST_TXD, ECHO_TEST_RXD, ECHO_TEST_RTS, ECHO_TEST_CTS));

    uint8_t *data = (uint8_t *) malloc(BUF_SIZE);
    uint32_t data_size = 0;

    while (1) {
        int len =
            uart_read_bytes(ECHO_UART_PORT_NUM, data + data_size, (BUF_SIZE - data_size - 1), 20 / portTICK_PERIOD_MS);
        uart_write_bytes(ECHO_UART_PORT_NUM, (const char *) data + data_size, len);
        data_size += len;
        if (data[data_size - 1] == '\r') {
            data[data_size] = '\0';
            ESP_LOGI(TAG, "Received command: %s", (char *) data);
            handle_uart_command((const char *) data, data_size - 1);
            data_size = 0;
        }
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
