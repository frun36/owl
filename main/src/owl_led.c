#include "owl_led.h"
#include "esp_log.h"

#include "driver/gpio.h"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include "portmacro.h"

static const char *TAG = "owl_led";

#define BOARD_LED_GPIO CONFIG_OWL_LED_GPIO

QueueHandle_t owl_led_command_queue;

// Send either one of the variants, or a positive integer specifying blink
// interval (in ms)
typedef enum {
    LED_BLINK_OFF = -3,
    LED_TOGGLE = -2,
    LED_ON = -1,
    LED_OFF = 0,
} led_command_t;

int led_status = 0;

static inline void led_on(void)
{
    led_status = 1;
    gpio_set_level(BOARD_LED_GPIO, led_status);
}

static inline void led_off(void)
{
    led_status = 0;
    gpio_set_level(BOARD_LED_GPIO, led_status);
}

static inline void led_toggle(void)
{
    led_status = led_status ? 0 : 1;
    gpio_set_level(BOARD_LED_GPIO, led_status);
}

static void owl_led_task(void *arg)
{
    led_command_t cmd = 0;
    TickType_t blink_interval = portMAX_DELAY;

    while (1) {
        if (xQueueReceive(owl_led_command_queue, &cmd, blink_interval)) {
            if (cmd == LED_OFF) {
                led_off();
            } else if (cmd == LED_ON) {
                led_on();
            } else if (cmd == LED_TOGGLE) {
                led_toggle();
            } else if (cmd == LED_BLINK_OFF) {
                blink_interval = portMAX_DELAY;
                led_off();
            } else if (cmd > 0) {
                blink_interval = pdMS_TO_TICKS(cmd);
                led_toggle();
            } else {
                ESP_LOGE(TAG, "Invalid LED command %d", cmd);
            }
        } else {
            led_toggle();
        }
    }
}

void owl_led_init(void)
{
    owl_led_command_queue = xQueueCreate(4, sizeof(int));
    gpio_reset_pin(BOARD_LED_GPIO);
    gpio_set_direction(BOARD_LED_GPIO, GPIO_MODE_OUTPUT);
    ESP_LOGI(TAG, "Initialized board led (GPIO%d)", BOARD_LED_GPIO);
    led_off();
    xTaskCreate(owl_led_task, "owl_led_task", 4096, NULL, 5, NULL);
}

void owl_led_on(void)
{
    led_command_t cmd = LED_ON;
    xQueueSend(owl_led_command_queue, &cmd, portMAX_DELAY);
}

void owl_led_off(void)
{
    led_command_t cmd = LED_OFF;
    xQueueSend(owl_led_command_queue, &cmd, portMAX_DELAY);
}

void owl_led_blink(int ms)
{
    xQueueSend(owl_led_command_queue, &ms, portMAX_DELAY);
}

void owl_led_blink_off(void)
{
    led_command_t cmd = LED_BLINK_OFF;
    xQueueSend(owl_led_command_queue, &cmd, portMAX_DELAY);
}

#undef BOARD_LED_GPIO
