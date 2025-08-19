#pragma once

#include "esp_err.h"
#include <stdint.h>

#include "owl_display.h"

void owl_lcd_set_backlight(owl_rgb_t color);

esp_err_t owl_lcd_clear();

esp_err_t owl_lcd_write(uint8_t line, const char *s);

void owl_lcd_init();
