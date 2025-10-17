idf_component_register(
    SRCS "led_playback.c" "led_driver.c" "prism_wave_tables.c" "prism_temporal.c" "prism_temporal_runtime.c" "effect_engine.c"
    INCLUDE_DIRS "include"
    REQUIRES driver freertos esp_timer core perfmon console
    PRIV_REQUIRES storage
)

target_compile_definitions(${COMPONENT_LIB} PRIVATE PRISM_PERF_INSTRUMENTATION=1)
