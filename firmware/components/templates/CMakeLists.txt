# Templates component CMakeLists.txt

idf_component_register(
    SRCS
        "template_loader.c"
        "template_patterns.c"
    INCLUDE_DIRS
        "include"
    REQUIRES
        esp_system
    PRIV_REQUIRES
        core
        playback
)