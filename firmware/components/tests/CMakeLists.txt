idf_component_register(
    SRCS
        "test_component_init.c"
        "test_wave_tables.c"
        "test_prism_parser.c"
        "test_temporal_shapes.c"
        "test_temporal_shapes_snapshots.c"
        "test_decode_microbench.c"
        "test_pattern_cache.c"
        "test_effect_engine.c"
        "test_templates_list.c"
        "test_templates_deploy.c"
    INCLUDE_DIRS "."
    REQUIRES unity network playback templates storage heap esp_timer esp_system bench
)
