idf_component_register(
    SRCS "pattern_storage.c"
    INCLUDE_DIRS "include"
    REQUIRES freertos
    # TODO: Add esp_littlefs dependency in Task 5 (requires idf.py add-dependency esp_littlefs)
)
