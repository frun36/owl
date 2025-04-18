#include "owl_led.h"
#include "esp_log.h"

#include "driver/gpio.h"

static const char* TAG = "owl_led";

#define BOARD_LED_GPIO 2

void configure_led(void)
{
    gpio_reset_pin(BOARD_LED_GPIO);
    gpio_set_direction(BOARD_LED_GPIO, GPIO_MODE_OUTPUT);
    ESP_LOGI(TAG, "Initialized board led (GPIO%d)", BOARD_LED_GPIO);
}

void led_on(void)
{
    gpio_set_level(BOARD_LED_GPIO, 1);
}

void led_off(void)
{
    gpio_set_level(BOARD_LED_GPIO, 0);
}

#undef BOARD_LED_GPIO
