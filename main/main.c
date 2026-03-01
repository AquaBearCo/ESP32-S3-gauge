#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl.h"
#include "sdkconfig.h"

#include "app/sensor_service.h"
#include "board/board_display.h"
#include "board/board_exio.h"
#include "board/board_i2c.h"
#include "board/board_touch.h"
#include "ui/dashboard_ui.h"
#include "ui/editor/editor_ui.h"

static const char *TAG = "main";

void app_main(void)
{
    ESP_LOGI(TAG, "Booting esp32-s3-gauge");

    ESP_ERROR_CHECK(board_i2c_init());
    ESP_ERROR_CHECK(board_exio_init());
    ESP_ERROR_CHECK(board_display_init());

    esp_err_t touch_err = board_touch_init();
    if (touch_err != ESP_OK) {
        ESP_LOGW(TAG, "Touch unavailable: %s", esp_err_to_name(touch_err));
    }

    ESP_ERROR_CHECK(board_lvgl_init());
    ESP_ERROR_CHECK(sensor_service_init());

#if CONFIG_GAUGE_USE_EDITOR_UI
    editor_ui_init();
    ESP_LOGI(TAG, "Running editor UI");
#else
    dashboard_ui_init();
    ESP_LOGI(TAG, "Running built-in dashboard UI");
#endif

    while (true) {
        lv_timer_handler();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
