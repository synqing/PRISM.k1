idf_component_register(
    SRCS
        "pattern_storage.c"
    INCLUDE_DIRS "include"
    REQUIRES
        freertos
        esp_partition
        core
    PRIV_REQUIRES
        joltwallet__littlefs
        vfs
)
