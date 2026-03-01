/**
 * @file c_dev_esp32_s3_gauge_lvgl_ui_gen.h
 */

#ifndef C_DEV_ESP32_S3_GAUGE_LVGL_UI_GEN_H
#define C_DEV_ESP32_S3_GAUGE_LVGL_UI_GEN_H

#ifndef UI_SUBJECT_STRING_LENGTH
#define UI_SUBJECT_STRING_LENGTH 256
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
    #include "lvgl.h"
#else
    #include "lvgl/lvgl.h"
#endif

/*********************
 *      DEFINES
 *********************/

#define GAP_SM 8

#define GAP_MD 12

#define PANEL_RADIUS 14

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL VARIABLES
 **********************/

/*-------------------
 * Permanent screens
 *------------------*/

extern lv_obj_t * dashboard;

/*----------------
 * Global styles
 *----------------*/

extern lv_style_t bg_root;
extern lv_style_t bg_panel;
extern lv_style_t txt_primary;
extern lv_style_t txt_muted;

/*----------------
 * Fonts
 *----------------*/

/*----------------
 * Images
 *----------------*/

/*----------------
 * Subjects
 *----------------*/

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/*----------------
 * Event Callbacks
 *----------------*/

/**
 * Initialize the component library
 */

void c_dev_esp32_s3_gauge_lvgl_ui_init_gen(const char * asset_path);

/**********************
 *      MACROS
 **********************/

/**********************
 *   POST INCLUDES
 **********************/

/*Include all the widget and components of this library*/
#include "screens/dashboard_gen.h"

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*C_DEV_ESP32_S3_GAUGE_LVGL_UI_GEN_H*/