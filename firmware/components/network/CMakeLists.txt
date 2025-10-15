# Network component CMakeLists.txt

idf_component_register(
    SRCS
        "websocket_handler.c"
        "protocol_parser.c"
        "network_manager.c"
    INCLUDE_DIRS
        "include"
    REQUIRES
        esp_wifi
        esp_http_server
        mdns
        json
    PRIV_REQUIRES
        core
)