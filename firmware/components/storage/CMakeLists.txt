# Storage component CMakeLists.txt

idf_component_register(
    SRCS
        "pattern_storage.c"
        "pattern_format.c"
        "cache_manager.c"
    INCLUDE_DIRS
        "include"
    REQUIRES
        esp_littlefs
        fatfs
        spi_flash
    PRIV_REQUIRES
        core
)