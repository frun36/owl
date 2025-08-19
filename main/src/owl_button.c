#include "owl_button.h"

#include <inttypes.h>

#include "button_gpio.h"
#include "esp_log.h"
#include "iot_button.h"

static const char *TAG = "owl_button";

QueueHandle_t owl_button_event_queue;

static void button_single_click_cb(void *arg, void *usr_data)
{
    owl_button_event_t e = OWL_BUTTON_SINGLE_CLICK;
    xQueueSend(owl_button_event_queue, &e, portMAX_DELAY);
}

static void button_double_click_cb(void *arg, void *usr_data)
{
    owl_button_event_t e = OWL_BUTTON_DOUBLE_CLICK;
    xQueueSend(owl_button_event_queue, &e, portMAX_DELAY);
}

static void button_long_press_cb(void *arg, void *usr_data)
{
    owl_button_event_t e = OWL_BUTTON_LONG_PRESS;
    xQueueSend(owl_button_event_queue, &e, portMAX_DELAY);
}

void owl_button_init(int32_t gpio_num)
{
    owl_button_event_queue = xQueueCreate(4, sizeof(int));
    button_config_t btn_cfg = { 0 };
    button_gpio_config_t gpio_cfg = {
        .gpio_num = gpio_num,
        .active_level = 0,
        .enable_power_save = false,
    };

    button_handle_t btn;
    ESP_ERROR_CHECK(iot_button_new_gpio_device(&btn_cfg, &gpio_cfg, &btn));

    ESP_ERROR_CHECK(iot_button_register_cb(
        btn, BUTTON_SINGLE_CLICK, NULL, button_single_click_cb, NULL));
    ESP_ERROR_CHECK(iot_button_register_cb(
        btn, BUTTON_DOUBLE_CLICK, NULL, button_double_click_cb, NULL));
    ESP_ERROR_CHECK(iot_button_register_cb(
        btn, BUTTON_LONG_PRESS_START, NULL, button_long_press_cb, NULL));

    ESP_LOGI(TAG, "Initialized button (GPIO%" PRIi32 ")", gpio_num);
}
