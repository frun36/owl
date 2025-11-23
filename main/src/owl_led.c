#include "owl_led.h"
#include "esp_log.h"

#include "driver/gpio.h"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include "led_strip.h"
#include "owl_display.h"
#include "portmacro.h"

static const char *TAG = "owl_led";

#define BOARD_LED_GPIO CONFIG_OWL_LED_GPIO

static led_strip_handle_t led_strip;

void owl_led_init(void)
{

    // LED strip for testing
    led_strip_config_t strip_config = {
        .strip_gpio_num = BOARD_LED_GPIO, // 38
        .max_leds = 1,
        .led_pixel_format = LED_PIXEL_FORMAT_GRB,
        .led_model = LED_MODEL_WS2812,
        .flags.invert_out = false,
    };

    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 10 * 1000 * 1000,
        .flags.with_dma = false,
    };
    ESP_ERROR_CHECK(
        led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
    ESP_ERROR_CHECK(led_strip_clear(led_strip));
}

void owl_led_set(owl_rgb_t color)
{
    led_strip_set_pixel(led_strip, 0, color.g, color.r, color.b); // GRB
    led_strip_refresh(led_strip);
}

void owl_led_clear(void)
{
    led_strip_clear(led_strip);
}

#undef BOARD_LED_GPIO
