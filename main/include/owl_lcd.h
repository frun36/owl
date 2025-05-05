#pragma once

#include "esp_err.h"
#include <stdint.h>

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} owl_rgb_t;

void owl_set_lcd_backlight(owl_rgb_t color);

esp_err_t owl_lcd_clear();

esp_err_t owl_lcd_write(uint8_t line, const char *s);

void owl_init_lcd();
