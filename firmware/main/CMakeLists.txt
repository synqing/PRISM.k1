# Main component CMakeLists.txt

idf_component_register(
    SRCS
        "main.c"
        "app_init.c"
    INCLUDE_DIRS
        "."
        "include"
    REQUIRES
        nvs_flash
        esp_wifi
        esp_http_server
        mdns
        esp_timer
        driver
    PRIV_REQUIRES
        core
        network
        storage
        playback
        templates
)