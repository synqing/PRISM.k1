# Core component CMakeLists.txt

idf_component_register(
    SRCS
        "esp_system_init.c"
        "memory_manager.c"
        "error_handler.c"
    INCLUDE_DIRS
        "include"
    REQUIRES
        esp_system
        heap
        log
)