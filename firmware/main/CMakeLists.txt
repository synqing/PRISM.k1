# Main component CMakeLists.txt

idf_component_register(
    SRCS
        "main.c"
    INCLUDE_DIRS
        "."
    REQUIRES
        nvs_flash
        esp_netif
        esp_event
        esp_timer
        heap
        spi_flash
    PRIV_REQUIRES
        core
        network
        storage
        playback
        templates
        bench
)
