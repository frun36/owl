#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"

#define BOARD_LED 2

static const char* TAG = "HELLO";

static uint8_t s_led_state = 0;

static void configure_led(void)
{
    ESP_LOGI(TAG, "Configuring GPIO LED!");
    gpio_reset_pin(BOARD_LED);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(BOARD_LED, GPIO_MODE_OUTPUT);
}

static void blink_led(void)
{
    gpio_set_level(BOARD_LED, s_led_state);
}

void app_main(void)
{
    configure_led();

    while (1) {
        ESP_LOGI(TAG, "LED %s!", s_led_state == true ? "ON" : "OFF");
        blink_led();
        /* Toggle the LED state */
        s_led_state = !s_led_state;
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}
