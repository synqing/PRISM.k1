# Core component CMakeLists.txt

idf_component_register(
    SRCS
        "prism_memory_pool.c"
        "prism_heap_monitor.c"
        "prism_secure.c"
    INCLUDE_DIRS
        "include"
    REQUIRES
        esp_system
        heap
        log
        esp_timer
        freertos
)