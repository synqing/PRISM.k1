idf_component_register(
    SRCS "network_manager.c" "protocol_parser.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_wifi esp_netif esp_event nvs_flash freertos esp_http_server mdns core
    PRIV_REQUIRES lwip storage
)
