#include "owl_lcd.h"
#include "driver/i2c_master.h"

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include <string.h>

#define BACKLIGHT_RED 0x01
#define BACKLIGHT_GREEN 0x02
#define BACKLIGHT_BLUE 0x03

#define LCD_CMD_CLEAR 0x01
#define LCD_CMD_SET_DDRAM_ADDR 0x80

#define LCD_ADDR_LINE0 0x00
#define LCD_ADDR_LINE1 0x40

#define LCD_CHAR_WIDTH 16

i2c_master_bus_handle_t bus_handle;
i2c_master_dev_handle_t backlight_handle;
i2c_master_dev_handle_t lcd_handle;

// backlight control

static inline esp_err_t backlight_write_reg(uint8_t reg_addr, uint8_t data)
{
    uint8_t write_buf[2] = { reg_addr, data };
    return i2c_master_transmit(
        backlight_handle, write_buf, sizeof(write_buf), pdMS_TO_TICKS(1000));
}

void owl_lcd_set_backlight(owl_rgb_t color)
{
    backlight_write_reg(BACKLIGHT_RED, color.r);
    backlight_write_reg(BACKLIGHT_GREEN, color.g);
    backlight_write_reg(BACKLIGHT_BLUE, color.b);
}

// lcd control

static inline esp_err_t lcd_command(uint8_t cmd)
{
    uint8_t write_buf[] = { 0x80, cmd };
    return i2c_master_transmit(
        lcd_handle, write_buf, sizeof(write_buf), pdMS_TO_TICKS(1000));
}

static inline esp_err_t lcd_char(uint8_t c)
{
    uint8_t write_buf[] = { 0x40, c };
    return i2c_master_transmit(
        lcd_handle, write_buf, sizeof(write_buf), pdMS_TO_TICKS(1000));
}

esp_err_t owl_lcd_clear()
{
    esp_err_t res = lcd_command(LCD_CMD_CLEAR);
    vTaskDelay(pdMS_TO_TICKS(2));
    return res;
}

esp_err_t owl_lcd_write(uint8_t line, const char *s)
{
    esp_err_t res = ESP_OK;
    res |= lcd_command(LCD_CMD_SET_DDRAM_ADDR
                       | (line & 1 ? LCD_ADDR_LINE1 : LCD_ADDR_LINE0));
    vTaskDelay(pdMS_TO_TICKS(2));
    size_t len = strlen(s);
    for (size_t i = 0; i < LCD_CHAR_WIDTH; i++) {
        if (i < len)
            res |= lcd_char(s[i]);
        else
            res |= lcd_char(' ');
    }
    return res;
}

#define LCD_PFX_FUNCTION 0x20
#define LCD_PFX_DISP 0x08
#define LCD_PFX_ENTRY_MODE 0x04

#define LCD_SET_1LINE 0x00
#define LCD_SET_2LINE 0x08
#define LCD_SET_DISP_OFF 0x00
#define LCD_SET_DISP_ON 0x04
#define LCD_SET_CURSOR_OFF 0x00
#define LCD_SET_CURSOR_ON 0x02
#define LCD_SET_BLINK_OFF 0x00
#define LCD_SET_BLINK_ON 0x01
#define LCD_SET_MOVE_RIGHT 0x20
#define LCD_SET_MOVE_LEFT 0x00
#define LCD_SET_SHIFT 0x01
#define LCD_SET_NO_SHIFT 0x00

void owl_lcd_init()
{
    i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = 21,
        .scl_io_num = 22,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &bus_handle));

    i2c_device_config_t backlight_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = 0x2D,
        .scl_speed_hz = 100000,
    };

    ESP_ERROR_CHECK(i2c_master_bus_add_device(
        bus_handle, &backlight_config, &backlight_handle));

    i2c_device_config_t disp_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = 0x3E,
        .scl_speed_hz = 100000,
    };

    ESP_ERROR_CHECK(
        i2c_master_bus_add_device(bus_handle, &disp_config, &lcd_handle));

    vTaskDelay(pdMS_TO_TICKS(50)); // wait after power up

    lcd_command(LCD_PFX_FUNCTION | LCD_SET_2LINE | LCD_SET_DISP_ON);
    vTaskDelay(pdMS_TO_TICKS(1));
    lcd_command(LCD_PFX_DISP | LCD_SET_DISP_ON | LCD_SET_CURSOR_OFF
                | LCD_SET_BLINK_OFF);
    vTaskDelay(pdMS_TO_TICKS(1));
    lcd_command(LCD_CMD_CLEAR);
    vTaskDelay(pdMS_TO_TICKS(2));
    lcd_command(LCD_PFX_ENTRY_MODE | LCD_SET_MOVE_RIGHT | LCD_SET_SHIFT);
    vTaskDelay(pdMS_TO_TICKS(10));
}
