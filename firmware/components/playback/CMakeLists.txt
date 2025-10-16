idf_component_register(
    SRCS "led_playback.c" "led_driver.c" "prism_wave_tables.c"
    INCLUDE_DIRS "include"
    REQUIRES driver freertos esp_timer core
)
