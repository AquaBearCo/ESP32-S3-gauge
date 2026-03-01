#include "board_display.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "driver/ledc.h"
#include "driver/spi_master.h"
#include "esp_heap_caps.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_rgb.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl.h"
#include "sdkconfig.h"

#include "board_exio.h"
#include "board_touch.h"

static const char *TAG = "board_display";

#define LCD_CTRL_MOSI_GPIO              1
#define LCD_CTRL_CLK_GPIO               2

#define LCD_H_RES                       480
#define LCD_V_RES                       480
#define LCD_PIXEL_CLOCK_HZ              (10 * 1000 * 1000)

#define LCD_RGB_HSYNC_GPIO              38
#define LCD_RGB_VSYNC_GPIO              39
#define LCD_RGB_DE_GPIO                 40
#define LCD_RGB_PCLK_GPIO               41
#define LCD_RGB_DATA0_GPIO              5
#define LCD_RGB_DATA1_GPIO              45
#define LCD_RGB_DATA2_GPIO              48
#define LCD_RGB_DATA3_GPIO              47
#define LCD_RGB_DATA4_GPIO              21
#define LCD_RGB_DATA5_GPIO              14
#define LCD_RGB_DATA6_GPIO              13
#define LCD_RGB_DATA7_GPIO              12
#define LCD_RGB_DATA8_GPIO              11
#define LCD_RGB_DATA9_GPIO              10
#define LCD_RGB_DATA10_GPIO             9
#define LCD_RGB_DATA11_GPIO             46
#define LCD_RGB_DATA12_GPIO             3
#define LCD_RGB_DATA13_GPIO             8
#define LCD_RGB_DATA14_GPIO             18
#define LCD_RGB_DATA15_GPIO             17

#define LCD_BACKLIGHT_GPIO              6
#define LCD_LEDC_TIMER                  LEDC_TIMER_0
#define LCD_LEDC_CHANNEL                LEDC_CHANNEL_0
#define LCD_LEDC_MODE                   LEDC_LOW_SPEED_MODE
#define LCD_LEDC_BITS                   LEDC_TIMER_13_BIT

#define LVGL_TICK_MS                    2
#define LVGL_BUFFER_LINES               80

static spi_device_handle_t s_spi = NULL;
static esp_lcd_panel_handle_t s_panel = NULL;

static lv_disp_draw_buf_t s_lvgl_draw_buf;
static lv_disp_drv_t s_lvgl_disp_drv;
static lv_indev_drv_t s_lvgl_indev_drv;
static esp_timer_handle_t s_lvgl_tick_timer = NULL;
static void *s_draw_buf1 = NULL;
static void *s_draw_buf2 = NULL;

static void st7701_write_cmd(uint8_t cmd)
{
    spi_transaction_t tx = {
        .cmd = 0,
        .addr = cmd,
        .length = 0,
        .rxlength = 0,
    };
    ESP_ERROR_CHECK(spi_device_transmit(s_spi, &tx));
}

static void st7701_write_data(uint8_t data)
{
    spi_transaction_t tx = {
        .cmd = 1,
        .addr = data,
        .length = 0,
        .rxlength = 0,
    };
    ESP_ERROR_CHECK(spi_device_transmit(s_spi, &tx));
}

static void st7701_reset(void)
{
    ESP_ERROR_CHECK(board_exio_set_pin(BOARD_EXIO_LCD_RST, false));
    vTaskDelay(pdMS_TO_TICKS(10));
    ESP_ERROR_CHECK(board_exio_set_pin(BOARD_EXIO_LCD_RST, true));
    vTaskDelay(pdMS_TO_TICKS(50));
}

static void st7701_select(bool select)
{
    ESP_ERROR_CHECK(board_exio_set_pin(BOARD_EXIO_LCD_CS, !select));
}

static void st7701_init_sequence(void)
{
    st7701_write_cmd(0xFF); st7701_write_data(0x77); st7701_write_data(0x01); st7701_write_data(0x00); st7701_write_data(0x00); st7701_write_data(0x10);
    st7701_write_cmd(0xC0); st7701_write_data(0x3B); st7701_write_data(0x00);
    st7701_write_cmd(0xC1); st7701_write_data(0x0B); st7701_write_data(0x02);
    st7701_write_cmd(0xC2); st7701_write_data(0x07); st7701_write_data(0x02);
    st7701_write_cmd(0xCC); st7701_write_data(0x10);
    st7701_write_cmd(0xCD); st7701_write_data(0x08);

    st7701_write_cmd(0xB0);
    st7701_write_data(0x00); st7701_write_data(0x11); st7701_write_data(0x16); st7701_write_data(0x0E);
    st7701_write_data(0x11); st7701_write_data(0x06); st7701_write_data(0x05); st7701_write_data(0x09);
    st7701_write_data(0x08); st7701_write_data(0x21); st7701_write_data(0x06); st7701_write_data(0x13);
    st7701_write_data(0x10); st7701_write_data(0x29); st7701_write_data(0x31); st7701_write_data(0x18);

    st7701_write_cmd(0xB1);
    st7701_write_data(0x00); st7701_write_data(0x11); st7701_write_data(0x16); st7701_write_data(0x0E);
    st7701_write_data(0x11); st7701_write_data(0x07); st7701_write_data(0x05); st7701_write_data(0x09);
    st7701_write_data(0x09); st7701_write_data(0x21); st7701_write_data(0x05); st7701_write_data(0x13);
    st7701_write_data(0x11); st7701_write_data(0x2A); st7701_write_data(0x31); st7701_write_data(0x18);

    st7701_write_cmd(0xFF); st7701_write_data(0x77); st7701_write_data(0x01); st7701_write_data(0x00); st7701_write_data(0x00); st7701_write_data(0x11);
    st7701_write_cmd(0xB0); st7701_write_data(0x6D);
    st7701_write_cmd(0xB1); st7701_write_data(0x37);
    st7701_write_cmd(0xB2); st7701_write_data(0x81);
    st7701_write_cmd(0xB3); st7701_write_data(0x80);
    st7701_write_cmd(0xB5); st7701_write_data(0x43);
    st7701_write_cmd(0xB7); st7701_write_data(0x85);
    st7701_write_cmd(0xB8); st7701_write_data(0x20);
    st7701_write_cmd(0xC1); st7701_write_data(0x78);
    st7701_write_cmd(0xC2); st7701_write_data(0x78);
    st7701_write_cmd(0xD0); st7701_write_data(0x88);

    st7701_write_cmd(0xE0); st7701_write_data(0x00); st7701_write_data(0x00); st7701_write_data(0x02);
    st7701_write_cmd(0xE1);
    st7701_write_data(0x03); st7701_write_data(0xA0); st7701_write_data(0x00); st7701_write_data(0x00);
    st7701_write_data(0x04); st7701_write_data(0xA0); st7701_write_data(0x00); st7701_write_data(0x00);
    st7701_write_data(0x00); st7701_write_data(0x20); st7701_write_data(0x20);

    st7701_write_cmd(0xE2);
    for (int i = 0; i < 13; i++) {
        st7701_write_data(0x00);
    }

    st7701_write_cmd(0xE3); st7701_write_data(0x00); st7701_write_data(0x00); st7701_write_data(0x11); st7701_write_data(0x00);
    st7701_write_cmd(0xE4); st7701_write_data(0x22); st7701_write_data(0x00);

    st7701_write_cmd(0xE5);
    st7701_write_data(0x05); st7701_write_data(0xEC); st7701_write_data(0xA0); st7701_write_data(0xA0);
    st7701_write_data(0x07); st7701_write_data(0xEE); st7701_write_data(0xA0); st7701_write_data(0xA0);
    for (int i = 0; i < 8; i++) {
        st7701_write_data(0x00);
    }

    st7701_write_cmd(0xE6); st7701_write_data(0x00); st7701_write_data(0x00); st7701_write_data(0x11); st7701_write_data(0x00);
    st7701_write_cmd(0xE7); st7701_write_data(0x22); st7701_write_data(0x00);

    st7701_write_cmd(0xE8);
    st7701_write_data(0x06); st7701_write_data(0xED); st7701_write_data(0xA0); st7701_write_data(0xA0);
    st7701_write_data(0x08); st7701_write_data(0xEF); st7701_write_data(0xA0); st7701_write_data(0xA0);
    for (int i = 0; i < 8; i++) {
        st7701_write_data(0x00);
    }

    st7701_write_cmd(0xEB);
    st7701_write_data(0x00); st7701_write_data(0x00); st7701_write_data(0x40); st7701_write_data(0x40);
    st7701_write_data(0x00); st7701_write_data(0x00); st7701_write_data(0x00);

    st7701_write_cmd(0xED);
    st7701_write_data(0xFF); st7701_write_data(0xFF); st7701_write_data(0xFF); st7701_write_data(0xBA);
    st7701_write_data(0x0A); st7701_write_data(0xBF); st7701_write_data(0x45); st7701_write_data(0xFF);
    st7701_write_data(0xFF); st7701_write_data(0x54); st7701_write_data(0xFB); st7701_write_data(0xA0);
    st7701_write_data(0xAB); st7701_write_data(0xFF); st7701_write_data(0xFF); st7701_write_data(0xFF);

    st7701_write_cmd(0xEF); st7701_write_data(0x10); st7701_write_data(0x0D); st7701_write_data(0x04);
    st7701_write_data(0x08); st7701_write_data(0x3F); st7701_write_data(0x1F);

    st7701_write_cmd(0xFF); st7701_write_data(0x77); st7701_write_data(0x01); st7701_write_data(0x00); st7701_write_data(0x00); st7701_write_data(0x13);
    st7701_write_cmd(0xEF); st7701_write_data(0x08);
    st7701_write_cmd(0xFF); st7701_write_data(0x77); st7701_write_data(0x01); st7701_write_data(0x00); st7701_write_data(0x00); st7701_write_data(0x00);

    st7701_write_cmd(0x36); st7701_write_data(0x00);
    st7701_write_cmd(0x3A); st7701_write_data(0x66);
    st7701_write_cmd(0x11); vTaskDelay(pdMS_TO_TICKS(120));
    st7701_write_cmd(0x20); vTaskDelay(pdMS_TO_TICKS(120));
    st7701_write_cmd(0x29);
}

static void lvgl_tick_cb(void *arg)
{
    (void)arg;
    lv_tick_inc(LVGL_TICK_MS);
}

static void lvgl_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map)
{
    esp_lcd_panel_handle_t panel = (esp_lcd_panel_handle_t)drv->user_data;
    esp_lcd_panel_draw_bitmap(panel, area->x1, area->y1, area->x2 + 1, area->y2 + 1, color_map);
    lv_disp_flush_ready(drv);
}

void board_display_set_backlight(uint8_t percent)
{
    if (percent > 100) {
        percent = 100;
    }
    const uint32_t max_duty = (1U << LCD_LEDC_BITS) - 1U;
    const uint32_t duty = (max_duty * percent) / 100U;
    ledc_set_duty(LCD_LEDC_MODE, LCD_LEDC_CHANNEL, duty);
    ledc_update_duty(LCD_LEDC_MODE, LCD_LEDC_CHANNEL);
}

static void backlight_init(void)
{
    const ledc_timer_config_t timer_cfg = {
        .speed_mode = LCD_LEDC_MODE,
        .duty_resolution = LCD_LEDC_BITS,
        .timer_num = LCD_LEDC_TIMER,
        .freq_hz = 5000,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer_cfg));

    const ledc_channel_config_t ch_cfg = {
        .gpio_num = LCD_BACKLIGHT_GPIO,
        .speed_mode = LCD_LEDC_MODE,
        .channel = LCD_LEDC_CHANNEL,
        .timer_sel = LCD_LEDC_TIMER,
        .duty = 0,
        .hpoint = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ch_cfg));
    board_display_set_backlight(CONFIG_GAUGE_BACKLIGHT_PERCENT);
}

esp_err_t board_display_init(void)
{
    const spi_bus_config_t bus_cfg = {
        .mosi_io_num = LCD_CTRL_MOSI_GPIO,
        .miso_io_num = -1,
        .sclk_io_num = LCD_CTRL_CLK_GPIO,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 64,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &bus_cfg, SPI_DMA_CH_AUTO));

    const spi_device_interface_config_t dev_cfg = {
        .command_bits = 1,
        .address_bits = 8,
        .mode = 0,
        .clock_speed_hz = 40000000,
        .spics_io_num = -1,
        .queue_size = 1,
    };
    ESP_ERROR_CHECK(spi_bus_add_device(SPI2_HOST, &dev_cfg, &s_spi));

    st7701_reset();
    st7701_select(true);
    st7701_init_sequence();
    st7701_select(false);

    const esp_lcd_rgb_panel_config_t panel_cfg = {
        .clk_src = LCD_CLK_SRC_DEFAULT,
        .timings = {
            .pclk_hz = LCD_PIXEL_CLOCK_HZ,
            .h_res = LCD_H_RES,
            .v_res = LCD_V_RES,
            .hsync_pulse_width = 8,
            .hsync_back_porch = 10,
            .hsync_front_porch = 50,
            .vsync_pulse_width = 3,
            .vsync_back_porch = 8,
            .vsync_front_porch = 8,
            .flags.pclk_active_neg = false,
        },
        .data_width = 16,
        .psram_trans_align = 64,
        .num_fbs = 1,
        .bounce_buffer_size_px = 10 * LCD_H_RES,
        .disp_gpio_num = -1,
        .pclk_gpio_num = LCD_RGB_PCLK_GPIO,
        .vsync_gpio_num = LCD_RGB_VSYNC_GPIO,
        .hsync_gpio_num = LCD_RGB_HSYNC_GPIO,
        .de_gpio_num = LCD_RGB_DE_GPIO,
        .data_gpio_nums = {
            LCD_RGB_DATA0_GPIO, LCD_RGB_DATA1_GPIO, LCD_RGB_DATA2_GPIO, LCD_RGB_DATA3_GPIO,
            LCD_RGB_DATA4_GPIO, LCD_RGB_DATA5_GPIO, LCD_RGB_DATA6_GPIO, LCD_RGB_DATA7_GPIO,
            LCD_RGB_DATA8_GPIO, LCD_RGB_DATA9_GPIO, LCD_RGB_DATA10_GPIO, LCD_RGB_DATA11_GPIO,
            LCD_RGB_DATA12_GPIO, LCD_RGB_DATA13_GPIO, LCD_RGB_DATA14_GPIO, LCD_RGB_DATA15_GPIO,
        },
        .flags = {
            .fb_in_psram = 1,
        },
    };

    ESP_ERROR_CHECK(esp_lcd_new_rgb_panel(&panel_cfg, &s_panel));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(s_panel));
    ESP_ERROR_CHECK(esp_lcd_panel_init(s_panel));
    backlight_init();
    ESP_LOGI(TAG, "Display initialized (%dx%d)", LCD_H_RES, LCD_V_RES);
    return ESP_OK;
}

esp_err_t board_lvgl_init(void)
{
    lv_init();

    const size_t buf_pixels = LCD_H_RES * LVGL_BUFFER_LINES;
    s_draw_buf1 = heap_caps_malloc(buf_pixels * sizeof(lv_color_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    s_draw_buf2 = heap_caps_malloc(buf_pixels * sizeof(lv_color_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (s_draw_buf1 == NULL || s_draw_buf2 == NULL) {
        ESP_LOGE(TAG, "LVGL draw buffers allocation failed");
        return ESP_ERR_NO_MEM;
    }

    lv_disp_draw_buf_init(&s_lvgl_draw_buf, s_draw_buf1, s_draw_buf2, buf_pixels);

    lv_disp_drv_init(&s_lvgl_disp_drv);
    s_lvgl_disp_drv.hor_res = LCD_H_RES;
    s_lvgl_disp_drv.ver_res = LCD_V_RES;
    s_lvgl_disp_drv.flush_cb = lvgl_flush_cb;
    s_lvgl_disp_drv.draw_buf = &s_lvgl_draw_buf;
    s_lvgl_disp_drv.user_data = s_panel;
    lv_disp_t *disp = lv_disp_drv_register(&s_lvgl_disp_drv);

    lv_indev_drv_init(&s_lvgl_indev_drv);
    s_lvgl_indev_drv.type = LV_INDEV_TYPE_POINTER;
    s_lvgl_indev_drv.disp = disp;
    s_lvgl_indev_drv.read_cb = board_touch_read;
    lv_indev_drv_register(&s_lvgl_indev_drv);

    const esp_timer_create_args_t tick_args = {
        .callback = lvgl_tick_cb,
        .name = "lvgl_tick",
    };
    ESP_ERROR_CHECK(esp_timer_create(&tick_args, &s_lvgl_tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(s_lvgl_tick_timer, LVGL_TICK_MS * 1000U));

    ESP_LOGI(TAG, "LVGL initialized");
    return ESP_OK;
}
