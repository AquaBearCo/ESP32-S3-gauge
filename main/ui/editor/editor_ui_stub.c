#include "editor_ui.h"

#include "lvgl.h"

#include "ui/dashboard_ui.h"

void editor_ui_init(void)
{
    dashboard_ui_init();
    lv_obj_t *warn = lv_label_create(lv_scr_act());
    lv_obj_align(warn, LV_ALIGN_TOP_RIGHT, -10, 8);
    lv_obj_set_style_text_color(warn, lv_color_hex(0xFFAA66), 0);
    lv_label_set_text(warn, "Editor UI not imported");
}
