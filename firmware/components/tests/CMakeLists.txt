idf_component_register(
    SRCS "test_component_init.c" "test_wave_tables.c"
    INCLUDE_DIRS "."
    REQUIRES unity network playback templates  # storage temporarily disabled
)
