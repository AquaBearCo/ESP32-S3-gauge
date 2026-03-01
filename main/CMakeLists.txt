set(SRCS
    "main.c"
    "board/board_i2c.c"
    "board/board_exio.c"
    "board/board_touch.c"
    "board/board_display.c"
    "sensors/qmi8658.c"
    "sensors/lps22hb.c"
    "app/sensor_service.c"
    "ui/dashboard_ui.c"
    "ui/editor/editor_ui_stub.c"
)

if(CONFIG_GAUGE_USE_EDITOR_UI AND EXISTS "${CMAKE_CURRENT_LIST_DIR}/ui/editor/generated/ui.h")
    file(GLOB_RECURSE EDITOR_UI_SRCS "${CMAKE_CURRENT_LIST_DIR}/ui/editor/generated/*.c")
    if(EDITOR_UI_SRCS)
        list(REMOVE_ITEM SRCS "ui/editor/editor_ui_stub.c")
        list(APPEND SRCS "ui/editor/editor_ui_generated.c")
        list(APPEND SRCS ${EDITOR_UI_SRCS})
        message(STATUS "LVGL editor generated UI detected")
    endif()
endif()

idf_component_register(
    SRCS ${SRCS}
    INCLUDE_DIRS
        "."
        "board"
        "sensors"
        "app"
        "ui"
        "ui/editor"
        "ui/editor/generated"
    REQUIRES lvgl
             driver
             esp_lcd
)

# LVGL editor-generated headers fall back to <lvgl/lvgl.h> unless this is set.
target_compile_definitions(${COMPONENT_LIB} PUBLIC LV_LVGL_H_INCLUDE_SIMPLE=1)
