#pragma once

#include "freertos/idf_additions.h"
#include <stdint.h>

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} owl_rgb_t;

typedef struct {
    char message[2][17]; // one for each line, with null terminator
    owl_rgb_t color;
    int duration_ms; // if <= 0: display indefinitely
} owl_display_event_t;

extern QueueHandle_t owl_display_event_queue;

void owl_display_init();
void owl_display(owl_display_event_t e);
