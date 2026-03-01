set(UI_ROOT ${CMAKE_CURRENT_LIST_DIR}/..)
include(${UI_ROOT}/file_list_gen.cmake)

idf_component_register(
    SRCS
        "main.c"
        ${PROJECT_SOURCES}
    INCLUDE_DIRS
        "."
        ".."
        "../screens"
    REQUIRES
        lvgl
)
