idf_component_register(
    SRCS 
        "test_main.c"
        "test_suite_temporal.c"
    INCLUDE_DIRS "."
    REQUIRES unity playback core storage esp_timer
)

