#pragma once

#include "freertos/idf_additions.h"

extern QueueHandle_t owl_button_event_queue;

typedef enum {
    OWL_BUTTON_SINGLE_CLICK,
    OWL_BUTTON_DOUBLE_CLICK,
    OWL_BUTTON_LONG_PRESS,
} owl_button_event_t;

void owl_init_button(int32_t gpio_num);
