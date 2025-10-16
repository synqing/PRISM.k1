idf_component_register(
    SRCS "test_component_init.c" "test_wave_tables.c" "test_prism_parser.c" "test_temporal_shapes.c" "test_temporal_shapes_snapshots.c"
    INCLUDE_DIRS "."
    REQUIRES unity network playback templates storage
)
