#pragma once

#include "freertos/idf_additions.h"
#include <stdint.h>

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} owl_rgb_t;

typedef enum {
    OWL_COLOR_RED,
    OWL_COLOR_GREEN,
    OWL_COLOR_BLUE,
    OWL_COLOR_CYAN,
    OWL_COLOR_MAGENTA,
    OWL_COLOR_YELLOW,
    OWL_COLOR_BLACK,
    OWL_COLOR_WHITE
} owl_color_t;

static inline owl_rgb_t owl_rgb(owl_color_t color)
{
    switch (color) {
    case OWL_COLOR_RED:
        return (owl_rgb_t) { .r = 255, .g = 0, .b = 0 };
    case OWL_COLOR_GREEN:
        return (owl_rgb_t) { .r = 0, .g = 255, .b = 0 };
    case OWL_COLOR_BLUE:
        return (owl_rgb_t) { .r = 0, .g = 0, .b = 255 };
    case OWL_COLOR_CYAN:
        return (owl_rgb_t) { .r = 0, .g = 255, .b = 255 };
    case OWL_COLOR_MAGENTA:
        return (owl_rgb_t) { .r = 255, .g = 0, .b = 255 };
    case OWL_COLOR_YELLOW:
        return (owl_rgb_t) { .r = 255, .g = 255, .b = 0 };
    case OWL_COLOR_BLACK:
        return (owl_rgb_t) { .r = 0, .g = 0, .b = 0 };
    case OWL_COLOR_WHITE:
    default:
        return (owl_rgb_t) { .r = 255, .g = 255, .b = 255 };
    }
}

typedef struct {
    const char *message[2]; // one for each line, with null terminator
    owl_rgb_t color;
    int duration_ms; // if <= 0: display indefinitely
} owl_display_event_t;

extern QueueHandle_t owl_display_event_queue;

void owl_display_init();
void owl_display(const char *line0,
                 const char *line1,
                 owl_rgb_t color,
                 int duration_ms);
