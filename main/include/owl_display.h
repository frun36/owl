#pragma once

#include "freertos/idf_additions.h"
#include "owl_color.h"
#include <stdint.h>

typedef struct {
    char message[2][17]; // one for each line, with null terminator
    owl_rgb_t color;
    int duration_ms; // if <= 0: display indefinitely
} owl_display_event_t;

void owl_display_init();
void owl_display(const char *line0,
                 const char *line1,
                 owl_rgb_t color,
                 int duration_ms);
void owl_display_update_status();
