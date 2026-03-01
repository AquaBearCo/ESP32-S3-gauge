/**
 * @file c_dev_esp32_s3_gauge_lvgl_ui_gen.c
 */

/*********************
 *      INCLUDES
 *********************/

#include "c_dev_esp32_s3_gauge_lvgl_ui_gen.h"

#if LV_USE_XML
#endif /* LV_USE_XML */

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/

/*----------------
 * Translations
 *----------------*/

/**********************
 *  GLOBAL VARIABLES
 **********************/

/*--------------------
 *  Permanent screens
 *-------------------*/

lv_obj_t * dashboard = NULL;

/*----------------
 * Global styles
 *----------------*/

lv_style_t bg_root;
lv_style_t bg_panel;
lv_style_t txt_primary;
lv_style_t txt_muted;

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
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void c_dev_esp32_s3_gauge_lvgl_ui_init_gen(const char * asset_path)
{
    (void)asset_path;

    /*----------------
     * Global styles
     *----------------*/

    static bool style_inited = false;

    if (!style_inited) {
        lv_style_init(&bg_root);
        lv_style_set_bg_color(&bg_root, lv_color_hex(0x06090F));

        lv_style_init(&bg_panel);
        lv_style_set_bg_color(&bg_panel, lv_color_hex(0x101722));
        lv_style_set_radius(&bg_panel, PANEL_RADIUS);

        lv_style_init(&txt_primary);
        lv_style_set_text_color(&txt_primary, lv_color_hex(0xE8EEF5));

        lv_style_init(&txt_muted);
        lv_style_set_text_color(&txt_muted, lv_color_hex(0x89A0B8));

        style_inited = true;
    }

    /*----------------
     * Fonts
     *----------------*/


    /*----------------
     * Images
     *----------------*/
    /*----------------
     * Subjects
     *----------------*/
    /*----------------
     * Translations
     *----------------*/

#if LV_USE_XML
    /* Register widgets */

    /* Register fonts */

    /* Register subjects */

    /* Register callbacks */
#endif

    /* Register all the global assets so that they won't be created again when globals.xml is parsed.
     * While running in the editor skip this step to update the preview when the XML changes */
#if LV_USE_XML && !defined(LV_EDITOR_PREVIEW)
    /* Register images */
#endif

#if LV_USE_XML == 0
    /*--------------------
     *  Permanent screens
     *-------------------*/
    /* If XML is enabled it's assumed that the permanent screens are created
     * manaully from XML using lv_xml_create() */
    /* To allow screens to reference each other, create them all before calling the sceen create functions */
    dashboard = lv_obj_create(NULL);

    dashboard_create();
#endif
}

/* Callbacks */

/**********************
 *   STATIC FUNCTIONS
 **********************/
