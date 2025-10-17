idf_component_register(
    SRCS "network_manager.c" "protocol_parser.c" "uart_test.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_wifi esp_netif esp_event nvs_flash freertos esp_http_server esp_http_client mdns core esp_app_format
    PRIV_REQUIRES lwip storage playback templates
)
