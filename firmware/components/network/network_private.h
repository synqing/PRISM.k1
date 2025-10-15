/**
 * @file network_private.h
 * @brief Private internal state and definitions for network_manager
 */

#ifndef NETWORK_PRIVATE_H
#define NETWORK_PRIVATE_H

#include <stdbool.h>
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_http_server.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "prism_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/* WiFi Configuration */
#define WIFI_AP_SSID            "PRISM-SETUP"
#define WIFI_AP_PASS            ""              // Open AP
#define WIFI_AP_CHANNEL         1
#define WIFI_AP_MAX_CONN        4
#define WIFI_RETRY_MAX          5
#define WIFI_RETRY_BASE_MS      1000            // 1s base delay
#define WIFI_RETRY_MAX_MS       30000           // 30s max delay

/* NVS Keys for credential storage */
#define NVS_NAMESPACE           "prism_wifi"
#define NVS_KEY_SSID            "ssid"
#define NVS_KEY_PASSWORD        "password"
#define NVS_KEY_CONFIGURED      "configured"

/* Captive Portal Configuration */
#define CAPTIVE_PORTAL_URI      "/"
#define CAPTIVE_PORTAL_PORT     80

/* WebSocket Configuration */
#define WS_URI                  "/ws"

/**
 * WiFi operating mode
 */
typedef enum {
    WIFI_MODE_AP_PORTAL,        // AP mode with captive portal active
    WIFI_MODE_STA_CONNECTING,   // Transitioning to STA, connecting
    WIFI_MODE_STA_CONNECTED,    // STA mode, connected to AP
    WIFI_MODE_STA_DISCONNECTED  // STA mode, disconnected (retrying)
} wifi_op_mode_t;

/**
 * @brief WebSocket client session state
 *
 * Tracks individual WebSocket client connections with 4KB receive buffers
 * allocated from prism_pool_alloc(). Maximum WS_MAX_CLIENTS (2) concurrent.
 */
typedef struct {
    bool active;                    ///< Slot occupied?
    int socket_fd;                  ///< Socket file descriptor for sending
    uint32_t last_activity_ms;      ///< Last received data timestamp (for timeout)
    uint8_t* rx_buffer;             ///< 4KB receive buffer (prism_pool_alloc)
    size_t rx_buffer_size;          ///< Always WS_BUFFER_SIZE (4096)
} ws_client_session_t;

/**
 * Network manager internal state
 */
typedef struct {
    // WiFi state
    wifi_op_mode_t current_mode;
    esp_netif_t *ap_netif;
    esp_netif_t *sta_netif;
    bool wifi_initialized;

    // Reconnection state
    uint32_t retry_count;
    uint32_t retry_delay_ms;

    // Captive portal / WebSocket (shared HTTP server)
    httpd_handle_t http_server;
    bool portal_active;

    // Credential storage
    bool credentials_available;
    char sta_ssid[33];          // Max SSID length + null
    char sta_password[64];      // Max password length + null

    // mDNS state
    bool mdns_initialized;

    // WebSocket state
    bool ws_handler_registered;                     ///< WebSocket endpoint registered?
    ws_client_session_t ws_clients[WS_MAX_CLIENTS]; ///< Client session slots (2 max)
    SemaphoreHandle_t ws_mutex;                     ///< Mutex for ws_clients[] access

} network_state_t;

/* Global network state (internal to component) */
extern network_state_t g_net_state;

/* Event handlers (forward declarations) */
void wifi_event_handler(void* arg, esp_event_base_t event_base,
                       int32_t event_id, void* event_data);
void ip_event_handler(void* arg, esp_event_base_t event_base,
                     int32_t event_id, void* event_data);

/* Captive portal functions */
esp_err_t start_captive_portal(void);
esp_err_t stop_captive_portal(void);

/* WiFi mode transitions */
esp_err_t transition_to_sta_mode(void);
esp_err_t start_sta_connection(void);

/* Credential management */
esp_err_t load_credentials_from_nvs(void);
esp_err_t save_credentials_to_nvs(const char* ssid, const char* password);

/* mDNS management */
esp_err_t start_mdns_service(void);
esp_err_t stop_mdns_service(void);

/* WebSocket lifecycle */
esp_err_t init_websocket_handler(void);
esp_err_t deinit_websocket_handler(void);

/* WebSocket request handler (registered with httpd) */
esp_err_t ws_handler(httpd_req_t *req);

/* WebSocket session management */
int find_free_ws_slot(void);
int find_ws_client_by_fd(int fd);
bool is_ws_client_timeout(int client_idx);
void cleanup_ws_client(int client_idx);

/* WebSocket frame handling */
esp_err_t handle_ws_frame(httpd_req_t *req, int client_idx);
esp_err_t send_ws_error(httpd_req_t *req, uint8_t error_code);
esp_err_t send_ws_status(httpd_req_t *req, uint8_t status_code, const char* message);

#ifdef __cplusplus
}
#endif

#endif // NETWORK_PRIVATE_H
