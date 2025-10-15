idf_component_register(
    SRCS "test_protocol_parser.c" "test_network_manager.c"
    INCLUDE_DIRS "."
    REQUIRES unity network
)
