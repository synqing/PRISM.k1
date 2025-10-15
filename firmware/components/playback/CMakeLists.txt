# Playback component CMakeLists.txt

idf_component_register(
    SRCS
        "led_driver.c"
        "effect_engine.c"
        "animation_timer.c"
    INCLUDE_DIRS
        "include"
    REQUIRES
        driver
        esp_timer
    PRIV_REQUIRES
        core
        storage
)