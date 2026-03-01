/**
 * @file dashboard_gen.c
 * @brief Template source file for LVGL objects
 */

/*********************
 *      INCLUDES
 *********************/

#include "dashboard_gen.h"
#include "c_dev_esp32_s3_gauge_lvgl_ui.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/***********************
 *  STATIC VARIABLES
 **********************/

/***********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

lv_obj_t * dashboard_create(void)
{
    LV_TRACE_OBJ_CREATE("begin");


    static bool style_inited = false;

    if (!style_inited) {

        style_inited = true;
    }

    if (dashboard == NULL) dashboard = lv_obj_create(NULL);
    lv_obj_t * lv_obj_0 = dashboard;
    lv_obj_set_name_static(lv_obj_0, "dashboard_#");
    lv_obj_set_style_bg_color(lv_obj_0, lv_color_hex(0x06090F), 0);

    lv_obj_t * lv_label_0 = lv_label_create(lv_obj_0);
    lv_obj_set_x(lv_label_0, 12);
    lv_obj_set_y(lv_label_0, 10);
    lv_label_set_text(lv_label_0, "GMT800 Dash UI");
    lv_obj_set_style_text_color(lv_label_0, lv_color_hex(0xE8EEF5), 0);
    
    lv_obj_t * lv_label_1 = lv_label_create(lv_obj_0);
    lv_obj_set_x(lv_label_1, 12);
    lv_obj_set_y(lv_label_1, 42);
    lv_label_set_text(lv_label_1, "LVGL Editor workspace project is ready.");
    lv_obj_set_style_text_color(lv_label_1, lv_color_hex(0x89A0B8), 0);
    
    lv_obj_t * lv_label_2 = lv_label_create(lv_obj_0);
    lv_obj_set_x(lv_label_2, 12);
    lv_obj_set_y(lv_label_2, 62);
    lv_label_set_text(lv_label_2, "Use Generate code, then import into ESP-IDF.");
    lv_obj_set_style_text_color(lv_label_2, lv_color_hex(0x89A0B8), 0);
    
    lv_obj_t * lv_obj_1 = lv_obj_create(lv_obj_0);
    lv_obj_set_x(lv_obj_1, 10);
    lv_obj_set_y(lv_obj_1, 90);
    lv_obj_set_width(lv_obj_1, 460);
    lv_obj_set_height(lv_obj_1, 180);
    lv_obj_set_style_bg_color(lv_obj_1, lv_color_hex(0x101722), 0);
    lv_obj_set_style_radius(lv_obj_1, 14, 0);
    lv_obj_t * lv_label_3 = lv_label_create(lv_obj_1);
    lv_obj_set_x(lv_label_3, 12);
    lv_obj_set_y(lv_label_3, 10);
    lv_label_set_text(lv_label_3, "Speed");
    lv_obj_set_style_text_color(lv_label_3, lv_color_hex(0xE8EEF5), 0);
    
    lv_obj_t * lv_label_4 = lv_label_create(lv_obj_1);
    lv_obj_set_x(lv_label_4, 12);
    lv_obj_set_y(lv_label_4, 36);
    lv_label_set_text(lv_label_4, "Altitude");
    lv_obj_set_style_text_color(lv_label_4, lv_color_hex(0xE8EEF5), 0);
    
    lv_obj_t * lv_label_5 = lv_label_create(lv_obj_1);
    lv_obj_set_x(lv_label_5, 12);
    lv_obj_set_y(lv_label_5, 62);
    lv_label_set_text(lv_label_5, "Roll / Pitch / Yaw");
    lv_obj_set_style_text_color(lv_label_5, lv_color_hex(0xE8EEF5), 0);
    
    lv_obj_t * lv_label_6 = lv_label_create(lv_obj_1);
    lv_obj_set_x(lv_label_6, 12);
    lv_obj_set_y(lv_label_6, 88);
    lv_label_set_text(lv_label_6, "Min / Max Altitude");
    lv_obj_set_style_text_color(lv_label_6, lv_color_hex(0xE8EEF5), 0);
    
    lv_obj_t * lv_label_7 = lv_label_create(lv_obj_1);
    lv_obj_set_x(lv_label_7, 12);
    lv_obj_set_y(lv_label_7, 114);
    lv_label_set_text(lv_label_7, "Vertical Speed");
    lv_obj_set_style_text_color(lv_label_7, lv_color_hex(0xE8EEF5), 0);

    LV_TRACE_OBJ_CREATE("finished");

    return lv_obj_0;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

