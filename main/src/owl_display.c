#include "owl_display.h"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include "owl_lcd.h"
#include "portmacro.h"
#include <stdbool.h>
#include <string.h>

QueueHandle_t owl_display_event_queue;

static void owl_display_task(void *arg)
{
    owl_display_event_t e;

    owl_display_event_t parent = (owl_display_event_t) {
        .message = { { "OWL" }, { "Helou" } },
        .color = (owl_rgb_t) { .r = 255, .g = 255, .b = 255 },
        .duration_ms = -1,
    };

    owl_lcd_set_backlight(parent.color);
    owl_lcd_write(0, parent.message[0]);
    owl_lcd_write(1, parent.message[1]);

    TickType_t time = portMAX_DELAY;
    bool to_show = false;
    while (1) {
#ifdef CONFIG_OWL_USE_LCD
        if (to_show) {
            owl_lcd_set_backlight(e.color);
            owl_lcd_write(0, e.message[0]);
            owl_lcd_write(1, e.message[1]);
            time = pdMS_TO_TICKS(e.duration_ms);
        }

        to_show = false;
        if (xQueueReceive(owl_display_event_queue, &e, time)) {
            to_show = true;

            if (e.duration_ms <= 0) {
                memcpy(&parent, &e, sizeof(owl_display_event_t));
                to_show = false;
            }
        }
        time = portMAX_DELAY;
        owl_lcd_set_backlight(parent.color);
        owl_lcd_write(0, parent.message[0]);
        owl_lcd_write(1, parent.message[1]);
#endif
    }
}

void owl_display_init()
{
#ifdef CONFIG_OWL_USE_LCD
    owl_lcd_init();
#endif

    owl_display_event_queue = xQueueCreate(2, sizeof(owl_display_event_t));
    xTaskCreate(owl_display_task, "owl_display_task", 4096, NULL, 5, NULL);
}

void owl_display(owl_display_event_t e)
{
    xQueueSend(owl_display_event_queue, &e, 0);
}
