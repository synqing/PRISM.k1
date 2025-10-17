file(GLOB TEMPLATE_DATA_SRCS "${CMAKE_CURRENT_LIST_DIR}/data/*.c")

idf_component_register(
    SRCS
        "template_manager.c"
        "template_patterns.c"
        ${TEMPLATE_DATA_SRCS}
    INCLUDE_DIRS "include"
    REQUIRES freertos console storage
)
