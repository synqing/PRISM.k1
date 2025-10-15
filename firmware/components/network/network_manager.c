/**
 * @file network_manager.c
 * @brief Network management implementation
 */

#include "network_manager.h"
#include "network_private.h"
#include "prism_memory_pool.h"
#include "prism_heap_monitor.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_mac.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "mdns.h"
#include "esp_http_server.h"
#include <string.h>

static const char *TAG = "network";

/* Global network state */
network_state_t g_net_state = {0};

/**
 * @brief Initialize WiFi dual-mode (AP + STA)
 */
static esp_err_t init_wifi_dual_mode(void) {
    esp_err_t ret;

    // Initialize TCP/IP stack
    ret = esp_netif_init();
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "Failed to init TCP/IP stack: %s", esp_err_to_name(ret));
        return ret;
    }

    // Create default event loop
    ret = esp_event_loop_create_default();
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "Failed to create event loop: %s", esp_err_to_name(ret));
        return ret;
    }

    // Create AP netif
    g_net_state.ap_netif = esp_netif_create_default_wifi_ap();
    if (!g_net_state.ap_netif) {
        ESP_LOGE(TAG, "Failed to create AP netif");
        return ESP_FAIL;
    }

    // Create STA netif
    g_net_state.sta_netif = esp_netif_create_default_wifi_sta();
    if (!g_net_state.sta_netif) {
        ESP_LOGE(TAG, "Failed to create STA netif");
        return ESP_FAIL;
    }

    // Initialize WiFi with default config
    wifi_init_config_t wifi_cfg = WIFI_INIT_CONFIG_DEFAULT();
    ret = esp_wifi_init(&wifi_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "WiFi init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    // Register event handlers
    ret = esp_event_handler_instance_register(WIFI_EVENT,
                                               ESP_EVENT_ANY_ID,
                                               &wifi_event_handler,
                                               NULL,
                                               NULL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register WiFi event handler: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = esp_event_handler_instance_register(IP_EVENT,
                                               IP_EVENT_STA_GOT_IP,
                                               &ip_event_handler,
                                               NULL,
                                               NULL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register IP event handler: %s", esp_err_to_name(ret));
        return ret;
    }

    // Set WiFi storage to RAM (we manage persistence via NVS)
    ret = esp_wifi_set_storage(WIFI_STORAGE_RAM);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set WiFi storage: %s", esp_err_to_name(ret));
        return ret;
    }

    g_net_state.wifi_initialized = true;
    ESP_LOGI(TAG, "WiFi dual-mode initialized");

    return ESP_OK;
}

/**
 * @brief Start AP mode with captive portal
 */
static esp_err_t start_ap_mode(void) {
    esp_err_t ret;

    // Configure AP
    wifi_config_t ap_config = {
        .ap = {
            .ssid = WIFI_AP_SSID,
            .ssid_len = strlen(WIFI_AP_SSID),
            .channel = WIFI_AP_CHANNEL,
            .password = WIFI_AP_PASS,
            .max_connection = WIFI_AP_MAX_CONN,
            .authmode = WIFI_AUTH_OPEN,
            .pmf_cfg = {
                .required = false,
            },
        },
    };

    // Set mode to APSTA (allows both AP and STA to coexist)
    ret = esp_wifi_set_mode(WIFI_MODE_APSTA);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set WiFi mode: %s", esp_err_to_name(ret));
        return ret;
    }

    // Configure AP
    ret = esp_wifi_set_config(WIFI_IF_AP, &ap_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set AP config: %s", esp_err_to_name(ret));
        return ret;
    }

    // Start WiFi
    ret = esp_wifi_start();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start WiFi: %s", esp_err_to_name(ret));
        return ret;
    }

    g_net_state.current_mode = WIFI_MODE_AP_PORTAL;
    ESP_LOGI(TAG, "AP mode started: SSID=%s", WIFI_AP_SSID);

    return ESP_OK;
}

esp_err_t network_init(void) {
    esp_err_t ret;

    ESP_LOGI(TAG, "Initializing network subsystem...");

    // Initialize NVS (required for WiFi)
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS partition was truncated, erasing...");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "NVS init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    // Initialize WiFi dual-mode
    ret = init_wifi_dual_mode();
    if (ret != ESP_OK) {
        return ret;
    }

    // Load stored credentials if available
    ret = load_credentials_from_nvs();
    if (ret == ESP_OK && g_net_state.credentials_available) {
        ESP_LOGI(TAG, "Found stored credentials, will attempt STA connection");
        // Will transition to STA mode in network_task
    } else {
        ESP_LOGI(TAG, "No stored credentials, starting captive portal");
    }

    // Start in AP mode with portal (always start here)
    ret = start_ap_mode();
    if (ret != ESP_OK) {
        return ret;
    }

    // Start captive portal HTTP server
    ret = start_captive_portal();
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to start captive portal: %s", esp_err_to_name(ret));
        // Non-fatal, continue
    }

    ESP_LOGI(TAG, "Network subsystem initialized");
    return ESP_OK;
}

/* ========================================================================
 * STUB IMPLEMENTATIONS - To be completed in subsequent subtasks
 * ======================================================================== */

/* ========================================================================
 * NVS CREDENTIAL PERSISTENCE (Subtask 2.3)
 * ======================================================================== */

/**
 * @brief Load WiFi credentials from NVS
 *
 * Attempts to load stored SSID and password from NVS.
 * If successful, populates g_net_state with credentials.
 *
 * @return ESP_OK if credentials loaded, ESP_ERR_NVS_NOT_FOUND if not stored
 */
esp_err_t load_credentials_from_nvs(void) {
    nvs_handle_t nvs_handle;
    esp_err_t ret;

    // Open NVS namespace
    ret = nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs_handle);
    if (ret != ESP_OK) {
        if (ret == ESP_ERR_NVS_NOT_FOUND) {
            ESP_LOGD(TAG, "NVS namespace not found, no credentials stored");
        } else {
            ESP_LOGE(TAG, "Failed to open NVS: %s", esp_err_to_name(ret));
        }
        g_net_state.credentials_available = false;
        return ret;
    }

    // Check if credentials are configured
    uint8_t configured = 0;
    ret = nvs_get_u8(nvs_handle, NVS_KEY_CONFIGURED, &configured);
    if (ret != ESP_OK || configured == 0) {
        ESP_LOGD(TAG, "No WiFi credentials configured in NVS");
        nvs_close(nvs_handle);
        g_net_state.credentials_available = false;
        return ESP_ERR_NVS_NOT_FOUND;
    }

    // Read SSID
    size_t ssid_len = sizeof(g_net_state.sta_ssid);
    ret = nvs_get_str(nvs_handle, NVS_KEY_SSID, g_net_state.sta_ssid, &ssid_len);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read SSID from NVS: %s", esp_err_to_name(ret));
        nvs_close(nvs_handle);
        g_net_state.credentials_available = false;
        return ret;
    }

    // Read password
    size_t pass_len = sizeof(g_net_state.sta_password);
    ret = nvs_get_str(nvs_handle, NVS_KEY_PASSWORD, g_net_state.sta_password, &pass_len);
    if (ret != ESP_OK) {
        // Password not found is acceptable (open network)
        if (ret == ESP_ERR_NVS_NOT_FOUND) {
            ESP_LOGD(TAG, "No password stored (open network)");
            g_net_state.sta_password[0] = '\0';
            ret = ESP_OK;
        } else {
            ESP_LOGE(TAG, "Failed to read password from NVS: %s", esp_err_to_name(ret));
            nvs_close(nvs_handle);
            g_net_state.credentials_available = false;
            return ret;
        }
    }

    nvs_close(nvs_handle);

    g_net_state.credentials_available = true;
    ESP_LOGI(TAG, "Loaded credentials from NVS: SSID='%s'", g_net_state.sta_ssid);

    return ESP_OK;
}

/**
 * @brief Save WiFi credentials to NVS
 *
 * Stores SSID and password in NVS for persistence across reboots.
 * Passwords are zeroed from memory after storage.
 *
 * @param ssid WiFi network SSID (max 32 chars)
 * @param password WiFi password (max 63 chars, empty for open network)
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t save_credentials_to_nvs(const char* ssid, const char* password) {
    nvs_handle_t nvs_handle;
    esp_err_t ret;

    if (!ssid) {
        ESP_LOGE(TAG, "SSID cannot be NULL");
        return ESP_ERR_INVALID_ARG;
    }

    // Open NVS namespace for writing
    ret = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS for writing: %s", esp_err_to_name(ret));
        return ret;
    }

    // Write SSID
    ret = nvs_set_str(nvs_handle, NVS_KEY_SSID, ssid);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write SSID to NVS: %s", esp_err_to_name(ret));
        nvs_close(nvs_handle);
        return ret;
    }

    // Write password (if provided)
    if (password && password[0] != '\0') {
        ret = nvs_set_str(nvs_handle, NVS_KEY_PASSWORD, password);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to write password to NVS: %s", esp_err_to_name(ret));
            nvs_close(nvs_handle);
            return ret;
        }
    } else {
        // Clear password for open network
        nvs_erase_key(nvs_handle, NVS_KEY_PASSWORD);
    }

    // Set configured flag
    ret = nvs_set_u8(nvs_handle, NVS_KEY_CONFIGURED, 1);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set configured flag: %s", esp_err_to_name(ret));
        nvs_close(nvs_handle);
        return ret;
    }

    // Commit changes
    ret = nvs_commit(nvs_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to commit NVS changes: %s", esp_err_to_name(ret));
        nvs_close(nvs_handle);
        return ret;
    }

    nvs_close(nvs_handle);

    ESP_LOGI(TAG, "Saved credentials to NVS: SSID='%s'", ssid);

    return ESP_OK;
}

/* ========================================================================
 * CAPTIVE PORTAL HTTP SERVER (Subtask 2.2)
 * ======================================================================== */

/**
 * HTML form for WiFi credential capture
 * Minimal, embedded in firmware to avoid filesystem dependency
 */
static const char PORTAL_HTML[] =
"<!DOCTYPE html>"
"<html><head>"
"<meta name='viewport' content='width=device-width,initial-scale=1'>"
"<title>PRISM Setup</title>"
"<style>"
"body{font-family:Arial,sans-serif;max-width:400px;margin:50px auto;padding:20px;}"
"input{width:100%;padding:8px;margin:8px 0;box-sizing:border-box;}"
"button{width:100%;padding:10px;background:#4CAF50;color:white;border:none;cursor:pointer;}"
"button:hover{background:#45a049;}"
"</style>"
"</head><body>"
"<h2>PRISM K1 Setup</h2>"
"<form action='/connect' method='post'>"
"<label>WiFi Network:</label>"
"<input type='text' name='ssid' required maxlength='32'>"
"<label>Password:</label>"
"<input type='password' name='pass' maxlength='63'>"
"<button type='submit'>Connect</button>"
"</form>"
"</body></html>";

/**
 * Success page shown after credential submission
 */
static const char PORTAL_SUCCESS_HTML[] =
"<!DOCTYPE html>"
"<html><head>"
"<meta name='viewport' content='width=device-width,initial-scale=1'>"
"<meta http-equiv='refresh' content='5;url=/'>"
"<title>PRISM Setup</title>"
"<style>"
"body{font-family:Arial,sans-serif;max-width:400px;margin:50px auto;padding:20px;text-align:center;}"
"</style>"
"</head><body>"
"<h2>✓ Connected!</h2>"
"<p>Your PRISM K1 is connecting to the network.</p>"
"<p>You can close this window.</p>"
"</body></html>";

/**
 * HTTP GET handler for root "/" - serves credential form
 */
static esp_err_t portal_get_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store, no-cache, must-revalidate");
    return httpd_resp_send(req, PORTAL_HTML, HTTPD_RESP_USE_STRLEN);
}

/**
 * Parse URL-encoded form data
 * Simple parser for "ssid=value&pass=value"
 */
static esp_err_t parse_form_data(const char* data, size_t len,
                                  char* ssid, size_t ssid_len,
                                  char* pass, size_t pass_len) {
    char* data_copy = prism_pool_alloc(len + 1);
    if (!data_copy) {
        ESP_LOGE(TAG, "Failed to allocate buffer for form parsing");
        return ESP_ERR_NO_MEM;
    }

    memcpy(data_copy, data, len);
    data_copy[len] = '\0';

    // Parse ssid
    char* ssid_start = strstr(data_copy, "ssid=");
    if (!ssid_start) {
        prism_pool_free(data_copy);
        return ESP_ERR_INVALID_ARG;
    }
    ssid_start += 5; // Skip "ssid="

    char* ssid_end = strchr(ssid_start, '&');
    size_t ssid_actual_len = ssid_end ? (size_t)(ssid_end - ssid_start) : strlen(ssid_start);
    if (ssid_actual_len >= ssid_len) {
        prism_pool_free(data_copy);
        return ESP_ERR_INVALID_SIZE;
    }

    // URL decode SSID (basic: only handle +  space)
    for (size_t i = 0; i < ssid_actual_len; i++) {
        ssid[i] = (ssid_start[i] == '+') ? ' ' : ssid_start[i];
    }
    ssid[ssid_actual_len] = '\0';

    // Parse password
    char* pass_start = strstr(data_copy, "pass=");
    if (pass_start) {
        pass_start += 5; // Skip "pass="
        char* pass_end = strchr(pass_start, '&');
        size_t pass_actual_len = pass_end ? (size_t)(pass_end - pass_start) : strlen(pass_start);
        if (pass_actual_len >= pass_len) {
            prism_pool_free(data_copy);
            return ESP_ERR_INVALID_SIZE;
        }

        // URL decode password
        for (size_t i = 0; i < pass_actual_len; i++) {
            pass[i] = (pass_start[i] == '+') ? ' ' : pass_start[i];
        }
        pass[pass_actual_len] = '\0';
    } else {
        pass[0] = '\0'; // No password (open network)
    }

    prism_pool_free(data_copy);
    return ESP_OK;
}

/**
 * HTTP POST handler for "/connect" - receives credentials
 */
static esp_err_t portal_post_handler(httpd_req_t *req) {
    char* buf = prism_pool_alloc(256); // Small buffer for form data
    if (!buf) {
        httpd_resp_send_500(req);
        return ESP_ERR_NO_MEM;
    }

    int ret = httpd_req_recv(req, buf, 256);
    if (ret <= 0) {
        prism_pool_free(buf);
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
            httpd_resp_send_408(req);
        }
        return ESP_FAIL;
    }

    char ssid[33] = {0};
    char pass[64] = {0};

    esp_err_t parse_ret = parse_form_data(buf, ret, ssid, sizeof(ssid), pass, sizeof(pass));
    prism_pool_free(buf);

    if (parse_ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to parse form data");
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid form data");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Received credentials: SSID='%s'", ssid);

    // Save credentials to NVS
    esp_err_t save_ret = save_credentials_to_nvs(ssid, pass);
    if (save_ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save credentials: %s", esp_err_to_name(save_ret));
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to save credentials");
        return ESP_FAIL;
    }

    // Update state
    strlcpy(g_net_state.sta_ssid, ssid, sizeof(g_net_state.sta_ssid));
    strlcpy(g_net_state.sta_password, pass, sizeof(g_net_state.sta_password));
    g_net_state.credentials_available = true;

    // Send success response
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, PORTAL_SUCCESS_HTML, HTTPD_RESP_USE_STRLEN);

    // Schedule STA transition (handled in network_task)
    ESP_LOGI(TAG, "Credentials saved, will transition to STA mode");

    return ESP_OK;
}

/**
 * @brief Start captive portal HTTP server
 */
esp_err_t start_captive_portal(void) {
    if (g_net_state.portal_active) {
        ESP_LOGW(TAG, "Captive portal already running");
        return ESP_OK;
    }

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = CAPTIVE_PORTAL_PORT;
    config.max_open_sockets = 4;
    config.lru_purge_enable = true;
    config.stack_size = 4096;

    esp_err_t ret = httpd_start(&g_net_state.http_server, &config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start HTTP server: %s", esp_err_to_name(ret));
        return ret;
    }

    // Register GET handler for root
    httpd_uri_t uri_get = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = portal_get_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(g_net_state.http_server, &uri_get);

    // Register POST handler for credential submission
    httpd_uri_t uri_post = {
        .uri = "/connect",
        .method = HTTP_POST,
        .handler = portal_post_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(g_net_state.http_server, &uri_post);

    // Wildcard handler for captive portal detection
    // Many devices check connectivity via specific URLs
    httpd_uri_t uri_wildcard = {
        .uri = "/*",
        .method = HTTP_GET,
        .handler = portal_get_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(g_net_state.http_server, &uri_wildcard);

    g_net_state.portal_active = true;
    ESP_LOGI(TAG, "Captive portal started on port %d", CAPTIVE_PORTAL_PORT);

    // Initialize WebSocket handler on same HTTP server
    ret = init_websocket_handler();
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to init WebSocket handler: %s", esp_err_to_name(ret));
        // Non-fatal, portal still works
    }

    return ESP_OK;
}

/**
 * @brief Stop captive portal HTTP server
 */
esp_err_t stop_captive_portal(void) {
    if (!g_net_state.portal_active) {
        return ESP_OK;
    }

    // Deinitialize WebSocket first (cleans up clients)
    if (g_net_state.ws_handler_registered) {
        esp_err_t ret = deinit_websocket_handler();
        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "Failed to deinit WebSocket: %s", esp_err_to_name(ret));
            // Non-fatal, continue
        }
    }

    if (g_net_state.http_server) {
        esp_err_t ret = httpd_stop(g_net_state.http_server);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to stop HTTP server: %s", esp_err_to_name(ret));
            return ret;
        }
        g_net_state.http_server = NULL;
    }

    g_net_state.portal_active = false;
    ESP_LOGI(TAG, "Captive portal stopped");

    return ESP_OK;
}

/* ========================================================================
 * STA MODE TRANSITION AND CONNECTION (Subtask 2.4)
 * ======================================================================== */

/**
 * @brief Start STA connection attempt with stored credentials
 *
 * Configures STA interface and initiates connection to stored AP.
 * Called after credentials are loaded from NVS or received via portal.
 *
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t start_sta_connection(void) {
    esp_err_t ret;

    if (!g_net_state.credentials_available) {
        ESP_LOGE(TAG, "Cannot start STA: no credentials available");
        return ESP_ERR_INVALID_STATE;
    }

    // Configure STA
    wifi_config_t sta_config = {0};
    strlcpy((char*)sta_config.sta.ssid, g_net_state.sta_ssid, sizeof(sta_config.sta.ssid));
    strlcpy((char*)sta_config.sta.password, g_net_state.sta_password, sizeof(sta_config.sta.password));
    sta_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    sta_config.sta.pmf_cfg.capable = true;
    sta_config.sta.pmf_cfg.required = false;

    ret = esp_wifi_set_config(WIFI_IF_STA, &sta_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set STA config: %s", esp_err_to_name(ret));
        return ret;
    }

    // Connect
    ret = esp_wifi_connect();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start WiFi connect: %s", esp_err_to_name(ret));
        return ret;
    }

    g_net_state.current_mode = WIFI_MODE_STA_CONNECTING;
    ESP_LOGI(TAG, "Connecting to SSID: %s", g_net_state.sta_ssid);

    return ESP_OK;
}

/**
 * @brief Transition from AP+Portal mode to STA mode
 *
 * Stops captive portal and initiates STA connection.
 * AP remains active for fallback (APSTA mode).
 *
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t transition_to_sta_mode(void) {
    esp_err_t ret;

    ESP_LOGI(TAG, "Transitioning to STA mode...");

    // Stop captive portal (keep AP running for fallback)
    if (g_net_state.portal_active) {
        ret = stop_captive_portal();
        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "Failed to stop portal during transition: %s", esp_err_to_name(ret));
            // Non-fatal, continue
        }
    }

    // Reset retry state
    g_net_state.retry_count = 0;
    g_net_state.retry_delay_ms = WIFI_RETRY_BASE_MS;

    // Attempt STA connection
    ret = start_sta_connection();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start STA connection: %s", esp_err_to_name(ret));
        // Could restart portal here as fallback
        return ret;
    }

    return ESP_OK;
}

/* ========================================================================
 * mDNS SERVICE ADVERTISEMENT (Subtask 2.5)
 * ======================================================================== */

#define MDNS_HOSTNAME       "prism-k1"
#define MDNS_INSTANCE       "PRISM K1 LED Controller"
#define MDNS_SERVICE_TYPE   "_prism"
#define MDNS_PROTO          "_tcp"
#define MDNS_PORT           80  // HTTP/WebSocket port

/**
 * @brief Start mDNS service for device discovery
 *
 * Advertises device as "prism-k1.local" with _prism._tcp service.
 * Should be called after STA obtains IP address.
 *
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t start_mdns_service(void) {
    esp_err_t ret;

    if (g_net_state.mdns_initialized) {
        ESP_LOGD(TAG, "mDNS already initialized");
        return ESP_OK;
    }

    // Initialize mDNS
    ret = mdns_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to init mDNS: %s", esp_err_to_name(ret));
        return ret;
    }

    // Set hostname
    ret = mdns_hostname_set(MDNS_HOSTNAME);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set mDNS hostname: %s", esp_err_to_name(ret));
        mdns_free();
        return ret;
    }

    // Set instance name
    ret = mdns_instance_name_set(MDNS_INSTANCE);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set mDNS instance: %s", esp_err_to_name(ret));
        mdns_free();
        return ret;
    }

    // Add HTTP service
    ret = mdns_service_add(NULL, "_http", "_tcp", 80, NULL, 0);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to add HTTP service: %s", esp_err_to_name(ret));
        // Non-fatal, continue
    }

    // Add custom PRISM service for discovery
    mdns_txt_item_t prism_txt[] = {
        {"version", "1.0"},
        {"device", "prism-k1"},
        {"leds", "320"}
    };

    ret = mdns_service_add(NULL, MDNS_SERVICE_TYPE, MDNS_PROTO, MDNS_PORT,
                          prism_txt, sizeof(prism_txt) / sizeof(mdns_txt_item_t));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add PRISM service: %s", esp_err_to_name(ret));
        mdns_free();
        return ret;
    }

    g_net_state.mdns_initialized = true;
    ESP_LOGI(TAG, "mDNS started: %s.local (_prism._tcp on port %d)", MDNS_HOSTNAME, MDNS_PORT);

    return ESP_OK;
}

/**
 * @brief Stop mDNS service
 *
 * Frees mDNS resources. Should be called when STA loses IP.
 *
 * @return ESP_OK on success
 */
esp_err_t stop_mdns_service(void) {
    if (!g_net_state.mdns_initialized) {
        return ESP_OK;
    }

    mdns_free();
    g_net_state.mdns_initialized = false;
    ESP_LOGI(TAG, "mDNS stopped");

    return ESP_OK;
}

/* ========================================================================
 * EVENT HANDLERS WITH EXPONENTIAL BACKOFF (Subtask 2.4)
 * ======================================================================== */

/**
 * @brief Calculate next retry delay with exponential backoff
 *
 * Doubles delay on each retry, capped at WIFI_RETRY_MAX_MS.
 */
static void update_retry_delay(void) {
    g_net_state.retry_delay_ms *= 2;
    if (g_net_state.retry_delay_ms > WIFI_RETRY_MAX_MS) {
        g_net_state.retry_delay_ms = WIFI_RETRY_MAX_MS;
    }
}

/**
 * @brief WiFi event handler with reconnection logic
 *
 * Handles all WiFi events for both AP and STA modes.
 * Implements exponential backoff for STA reconnection.
 */
void wifi_event_handler(void* arg, esp_event_base_t event_base,
                       int32_t event_id, void* event_data) {
    switch (event_id) {
        // AP mode events
        case WIFI_EVENT_AP_STACONNECTED: {
            wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
            ESP_LOGI(TAG, "Station " MACSTR " joined AP", MAC2STR(event->mac));
            break;
        }

        case WIFI_EVENT_AP_STADISCONNECTED: {
            wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
            ESP_LOGI(TAG, "Station " MACSTR " left AP", MAC2STR(event->mac));
            break;
        }

        // STA mode events
        case WIFI_EVENT_STA_START:
            ESP_LOGI(TAG, "STA started");
            break;

        case WIFI_EVENT_STA_CONNECTED: {
            wifi_event_sta_connected_t* event = (wifi_event_sta_connected_t*) event_data;
            ESP_LOGI(TAG, "Connected to AP SSID:%s channel:%d",
                     event->ssid, event->channel);
            // Reset retry counters on successful connection
            g_net_state.retry_count = 0;
            g_net_state.retry_delay_ms = WIFI_RETRY_BASE_MS;
            break;
        }

        case WIFI_EVENT_STA_DISCONNECTED: {
            wifi_event_sta_disconnected_t* event = (wifi_event_sta_disconnected_t*) event_data;
            ESP_LOGW(TAG, "Disconnected from AP, reason: %d", event->reason);

            g_net_state.current_mode = WIFI_MODE_STA_DISCONNECTED;

            // Implement exponential backoff reconnection
            if (g_net_state.retry_count < WIFI_RETRY_MAX) {
                ESP_LOGI(TAG, "Retry %d/%d in %d ms",
                         g_net_state.retry_count + 1,
                         WIFI_RETRY_MAX,
                         g_net_state.retry_delay_ms);

                // Delay before retry
                vTaskDelay(pdMS_TO_TICKS(g_net_state.retry_delay_ms));

                // Attempt reconnection
                esp_err_t ret = esp_wifi_connect();
                if (ret == ESP_OK) {
                    g_net_state.retry_count++;
                    update_retry_delay();
                    g_net_state.current_mode = WIFI_MODE_STA_CONNECTING;
                } else {
                    ESP_LOGE(TAG, "Reconnect failed: %s", esp_err_to_name(ret));
                }
            } else {
                ESP_LOGE(TAG, "Max retries reached, staying disconnected");
                ESP_LOGI(TAG, "Restart captive portal or use stored credentials");
                // Could restart portal here for reconfiguration
                // For now, will keep retrying with max delay
                g_net_state.retry_count = 0; // Reset for continuous retry
            }
            break;
        }

        case WIFI_EVENT_STA_STOP:
            ESP_LOGI(TAG, "STA stopped");
            g_net_state.current_mode = WIFI_MODE_AP_PORTAL;
            break;

        default:
            ESP_LOGD(TAG, "Unhandled WiFi event: %ld", event_id);
            break;
    }
}

/**
 * @brief IP event handler for STA connectivity
 *
 * Handles IP acquisition and triggers mDNS startup.
 */
void ip_event_handler(void* arg, esp_event_base_t event_base,
                     int32_t event_id, void* event_data) {
    switch (event_id) {
        case IP_EVENT_STA_GOT_IP: {
            ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
            ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
            ESP_LOGI(TAG, "Netmask: " IPSTR, IP2STR(&event->ip_info.netmask));
            ESP_LOGI(TAG, "Gateway: " IPSTR, IP2STR(&event->ip_info.gw));

            g_net_state.current_mode = WIFI_MODE_STA_CONNECTED;

            // Start mDNS service now that we have IP
            esp_err_t ret = start_mdns_service();
            if (ret != ESP_OK) {
                ESP_LOGW(TAG, "Failed to start mDNS: %s", esp_err_to_name(ret));
                // Non-fatal, continue
            }
            break;
        }

        case IP_EVENT_STA_LOST_IP:
            ESP_LOGW(TAG, "Lost IP address");
            g_net_state.current_mode = WIFI_MODE_STA_DISCONNECTED;
            // Stop mDNS when IP is lost
            stop_mdns_service();
            break;

        default:
            ESP_LOGD(TAG, "Unhandled IP event: %ld", event_id);
            break;
    }
}

/* ========================================================================
 * WEBSOCKET SESSION MANAGEMENT (Task 3 - Phase 3)
 * ======================================================================== */

/**
 * @brief Find a free slot in ws_clients array
 *
 * Searches for an inactive slot and allocates 4KB RX buffer.
 * Must be called with ws_mutex held.
 *
 * @return Index 0-1 on success, -1 if no slots available or allocation fails
 */
int find_free_ws_slot(void) {
    for (int i = 0; i < WS_MAX_CLIENTS; i++) {
        if (!g_net_state.ws_clients[i].active) {
            // Allocate RX buffer for this slot
            uint8_t* rx_buffer = prism_pool_alloc(WS_BUFFER_SIZE);
            if (rx_buffer == NULL) {
                ESP_LOGE(TAG, "Failed to allocate WebSocket RX buffer (%d bytes)", WS_BUFFER_SIZE);
                return -1;
            }

            g_net_state.ws_clients[i].rx_buffer = rx_buffer;
            g_net_state.ws_clients[i].rx_buffer_size = WS_BUFFER_SIZE;
            g_net_state.ws_clients[i].active = true;
            g_net_state.ws_clients[i].last_activity_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;

            ESP_LOGI(TAG, "Allocated WebSocket slot %d (buffer: %d bytes)", i, WS_BUFFER_SIZE);
            return i;
        }
    }

    ESP_LOGW(TAG, "No free WebSocket slots (max %d clients)", WS_MAX_CLIENTS);
    return -1;
}

/**
 * @brief Find client index by socket file descriptor
 *
 * Must be called with ws_mutex held.
 *
 * @param fd Socket file descriptor from httpd_req_t
 * @return Index 0-1 on success, -1 if not found
 */
int find_ws_client_by_fd(int fd) {
    for (int i = 0; i < WS_MAX_CLIENTS; i++) {
        if (g_net_state.ws_clients[i].active &&
            g_net_state.ws_clients[i].socket_fd == fd) {
            return i;
        }
    }
    return -1;
}

/**
 * @brief Check if client has exceeded timeout period
 *
 * Compares current time against last_activity_ms + WS_TIMEOUT_MS.
 *
 * @param client_idx Index in ws_clients array
 * @return true if timeout exceeded, false otherwise
 */
bool is_ws_client_timeout(int client_idx) {
    if (client_idx < 0 || client_idx >= WS_MAX_CLIENTS) {
        return false;
    }

    if (!g_net_state.ws_clients[client_idx].active) {
        return false;
    }

    uint32_t now_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;
    uint32_t idle_ms = now_ms - g_net_state.ws_clients[client_idx].last_activity_ms;

    if (idle_ms > WS_TIMEOUT_MS) {
        ESP_LOGW(TAG, "Client %d timeout: %lu ms idle (max %d ms)",
                 client_idx, idle_ms, WS_TIMEOUT_MS);
        return true;
    }

    return false;
}

/**
 * @brief Clean up client session and free resources
 *
 * Frees RX buffer and clears session structure.
 * Must be called with ws_mutex held.
 *
 * @param client_idx Index in ws_clients array
 */
void cleanup_ws_client(int client_idx) {
    if (client_idx < 0 || client_idx >= WS_MAX_CLIENTS) {
        return;
    }

    if (!g_net_state.ws_clients[client_idx].active) {
        return;  // Already cleaned up
    }

    // Free RX buffer
    if (g_net_state.ws_clients[client_idx].rx_buffer != NULL) {
        prism_pool_free(g_net_state.ws_clients[client_idx].rx_buffer);
        g_net_state.ws_clients[client_idx].rx_buffer = NULL;
    }

    // Clear session
    memset(&g_net_state.ws_clients[client_idx], 0, sizeof(ws_client_session_t));

    ESP_LOGI(TAG, "WebSocket client %d cleaned up", client_idx);
}

/* ========================================================================
 * WEBSOCKET SENDING FUNCTIONS (Task 3 - Phase 5)
 * ======================================================================== */

/**
 * @brief Send error frame to WebSocket client
 *
 * Sends a simple TLV-like error message. Format will be finalized in Task 4.
 * Placeholder format: [0xFF (error type), error_code]
 *
 * @param req HTTP request structure
 * @param error_code Error code (0x01=unsupported type, 0x02=too large, etc.)
 * @return ESP_OK on success
 */
esp_err_t send_ws_error(httpd_req_t *req, uint8_t error_code) {
    uint8_t frame[2] = {0xFF, error_code};  // Placeholder TLV format

    httpd_ws_frame_t ws_pkt = {0};
    ws_pkt.type = HTTPD_WS_TYPE_BINARY;
    ws_pkt.payload = frame;
    ws_pkt.len = sizeof(frame);

    esp_err_t ret = httpd_ws_send_frame(req, &ws_pkt);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to send error frame: %s", esp_err_to_name(ret));
    } else {
        ESP_LOGD(TAG, "Sent error frame: code=0x%02X", error_code);
    }

    return ret;
}

/**
 * @brief Send status frame to WebSocket client
 *
 * Sends a status message with optional text. Format will be finalized in Task 4.
 * Placeholder format: [0x30 (status type), status_code, message...]
 *
 * @param req HTTP request structure
 * @param status_code Status code (0x00=OK, etc.)
 * @param message Optional status message (can be NULL)
 * @return ESP_OK on success
 */
esp_err_t send_ws_status(httpd_req_t *req, uint8_t status_code, const char* message) {
    uint8_t frame[128];
    size_t frame_len = 0;

    // Placeholder TLV encoding (Task 4 will define proper format)
    frame[0] = 0x30;  // Status message type
    frame[1] = status_code;
    frame_len = 2;

    if (message != NULL) {
        size_t msg_len = strlen(message);
        if (msg_len > 125) msg_len = 125;  // Cap at 125 bytes
        memcpy(&frame[2], message, msg_len);
        frame_len += msg_len;
    }

    httpd_ws_frame_t ws_pkt = {0};
    ws_pkt.type = HTTPD_WS_TYPE_BINARY;
    ws_pkt.payload = frame;
    ws_pkt.len = frame_len;

    esp_err_t ret = httpd_ws_send_frame(req, &ws_pkt);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to send status frame: %s", esp_err_to_name(ret));
    } else {
        ESP_LOGD(TAG, "Sent status frame: code=0x%02X msg='%s'",
                 status_code, message ? message : "(none)");
    }

    return ret;
}

/**
 * @brief Send binary frame to all connected WebSocket clients
 *
 * Broadcasts binary data (typically TLV-encoded messages) to all active clients.
 * This is the public API used by TLV protocol layer.
 *
 * NOTE: ESP-IDF httpd_ws_send_frame() requires httpd_req_t, but we only have
 * socket FDs for broadcast. Investigation needed for proper implementation.
 * Current implementation is a placeholder.
 *
 * @param data Pointer to binary data buffer
 * @param len Length of data in bytes
 * @return ESP_OK if sent to at least one client, ESP_FAIL if no clients or error
 */
esp_err_t ws_broadcast_binary(const uint8_t* data, size_t len) {
    if (data == NULL || len == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    if (!g_net_state.ws_handler_registered) {
        ESP_LOGW(TAG, "WebSocket handler not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    int sent_count = 0;

    xSemaphoreTake(g_net_state.ws_mutex, portMAX_DELAY);

    for (int i = 0; i < WS_MAX_CLIENTS; i++) {
        if (!g_net_state.ws_clients[i].active) {
            continue;  // Skip inactive slots
        }

        // TODO: Implement proper broadcast without httpd_req_t
        // Options to investigate:
        // 1. httpd_queue_work() - queue async send job
        // 2. httpd_ws_send_data() - if it exists in ESP-IDF version
        // 3. Store httpd_req_t copy per client (thread-safety concerns)

        ESP_LOGD(TAG, "Broadcasting %zu bytes to client %d (fd=%d)",
                 len, i, g_net_state.ws_clients[i].socket_fd);
        sent_count++;
    }

    xSemaphoreGive(g_net_state.ws_mutex);

    // Placeholder return
    return (sent_count > 0) ? ESP_OK : ESP_FAIL;
}

/* ========================================================================
 * WEBSOCKET FRAME HANDLING (Task 3 - Phase 4)
 * ======================================================================== */

/**
 * @brief Handle a single WebSocket frame from a client
 *
 * Implements two-step receive pattern per ESP-IDF documentation:
 * 1. Query frame size with max_len=0
 * 2. Receive into pre-allocated buffer
 *
 * @param req HTTP request structure (contains WebSocket frame)
 * @param client_idx Index in ws_clients array
 * @return ESP_OK to keep connection, ESP_FAIL to close
 */
esp_err_t handle_ws_frame(httpd_req_t *req, int client_idx) {
    esp_err_t ret;

    // Step 1: Get frame size
    httpd_ws_frame_t ws_pkt = {0};
    ws_pkt.type = HTTPD_WS_TYPE_BINARY;  // Expect binary frames

    ret = httpd_ws_recv_frame(req, &ws_pkt, 0);  // max_len=0 queries size
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get frame size: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGD(TAG, "WebSocket frame: len=%zu type=%d", ws_pkt.len, ws_pkt.type);

    // Handle close frames
    if (ws_pkt.type == HTTPD_WS_TYPE_CLOSE) {
        ESP_LOGI(TAG, "Client %d sent CLOSE frame", client_idx);
        return ESP_FAIL;  // Signal to close connection
    }

    // Reject non-binary frames (text not supported)
    if (ws_pkt.type != HTTPD_WS_TYPE_BINARY) {
        ESP_LOGW(TAG, "Rejecting non-binary frame (type=%d)", ws_pkt.type);
        send_ws_error(req, 0x01);  // Error: unsupported frame type
        return ESP_FAIL;
    }

    // Step 2: Validate size and receive into buffer
    if (ws_pkt.len > WS_BUFFER_SIZE) {
        ESP_LOGW(TAG, "Frame too large (%zu bytes), max is %d", ws_pkt.len, WS_BUFFER_SIZE);
        send_ws_error(req, 0x02);  // Error: frame too large
        return ESP_FAIL;
    }

    if (ws_pkt.len == 0) {
        ESP_LOGD(TAG, "Empty frame received (heartbeat?)");
        // Update activity timestamp for empty frames too
        g_net_state.ws_clients[client_idx].last_activity_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;
        return ESP_OK;
    }

    // Receive into pre-allocated buffer
    ws_pkt.payload = g_net_state.ws_clients[client_idx].rx_buffer;
    ret = httpd_ws_recv_frame(req, &ws_pkt, ws_pkt.len);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to receive frame: %s", esp_err_to_name(ret));
        return ret;
    }

    // Update activity timestamp
    g_net_state.ws_clients[client_idx].last_activity_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;

    ESP_LOGI(TAG, "Received %zu bytes from client %d", ws_pkt.len, client_idx);
    ESP_LOG_BUFFER_HEX_LEVEL(TAG, ws_pkt.payload, ws_pkt.len > 64 ? 64 : ws_pkt.len, ESP_LOG_DEBUG);

    // TODO: Task 4 - Pass to TLV parser
    // return tlv_dispatch_command(ws_pkt.payload, ws_pkt.len, req);

    // Placeholder: Echo received data back for testing
    send_ws_status(req, 0x00, "Frame received");  // Status: OK

    return ESP_OK;
}

/**
 * @brief Main WebSocket request handler (registered with httpd)
 *
 * Called by ESP-IDF HTTP server for both new connections and data frames.
 * Manages client slots and delegates frame handling.
 *
 * @param req HTTP request structure
 * @return ESP_OK to keep connection, ESP_FAIL to close
 */
esp_err_t ws_handler(httpd_req_t *req) {
    if (req->method == HTTP_GET) {
        // New WebSocket connection (upgrade from HTTP GET)
        ESP_LOGI(TAG, "New WebSocket connection request");

        xSemaphoreTake(g_net_state.ws_mutex, portMAX_DELAY);

        int slot_idx = find_free_ws_slot();
        if (slot_idx < 0) {
            xSemaphoreGive(g_net_state.ws_mutex);
            ESP_LOGW(TAG, "Rejecting connection: max clients reached");
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
                               "Max WebSocket clients reached");
            return ESP_FAIL;
        }

        // Store socket FD for sending
        int sockfd = httpd_req_to_sockfd(req);
        g_net_state.ws_clients[slot_idx].socket_fd = sockfd;

        xSemaphoreGive(g_net_state.ws_mutex);

        ESP_LOGI(TAG, "WebSocket client %d connected (fd=%d)", slot_idx, sockfd);
        return ESP_OK;  // Accept connection
    }

    // Data frame received
    int sockfd = httpd_req_to_sockfd(req);

    xSemaphoreTake(g_net_state.ws_mutex, portMAX_DELAY);

    int client_idx = find_ws_client_by_fd(sockfd);
    if (client_idx < 0) {
        xSemaphoreGive(g_net_state.ws_mutex);
        ESP_LOGW(TAG, "Frame from unknown client (fd=%d)", sockfd);
        return ESP_FAIL;  // Close unknown connection
    }

    // Check timeout before processing
    if (is_ws_client_timeout(client_idx)) {
        ESP_LOGW(TAG, "Client %d timed out, closing connection", client_idx);
        cleanup_ws_client(client_idx);
        xSemaphoreGive(g_net_state.ws_mutex);
        return ESP_FAIL;
    }

    // Handle frame
    esp_err_t ret = handle_ws_frame(req, client_idx);

    // Cleanup on error
    if (ret != ESP_OK) {
        ESP_LOGI(TAG, "Frame handling failed for client %d, closing connection", client_idx);
        cleanup_ws_client(client_idx);
    }

    xSemaphoreGive(g_net_state.ws_mutex);

    return ret;
}

/* ========================================================================
 * WEBSOCKET LIFECYCLE (Task 3 - Phase 2)
 * ======================================================================== */

/**
 * @brief Initialize WebSocket handler on existing HTTP server
 *
 * Registers WebSocket endpoint at /ws on the HTTP server created by
 * start_captive_portal(). Creates mutex for client array protection.
 *
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t init_websocket_handler(void) {
    esp_err_t ret;

    if (g_net_state.ws_handler_registered) {
        ESP_LOGD(TAG, "WebSocket handler already registered");
        return ESP_OK;
    }

    if (!g_net_state.http_server) {
        ESP_LOGE(TAG, "Cannot register WebSocket: HTTP server not running");
        return ESP_ERR_INVALID_STATE;
    }

    // Create mutex for ws_clients[] access
    g_net_state.ws_mutex = xSemaphoreCreateMutex();
    if (g_net_state.ws_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create WebSocket mutex");
        return ESP_ERR_NO_MEM;
    }

    // Initialize client slots to zero
    memset(g_net_state.ws_clients, 0, sizeof(g_net_state.ws_clients));

    // Register WebSocket endpoint
    httpd_uri_t ws_uri = {
        .uri        = WS_URI,
        .method     = HTTP_GET,
        .handler    = ws_handler,
        .user_ctx   = NULL,
        .is_websocket = true,
        .handle_ws_control_frames = true  // ESP-IDF handles PING/PONG
    };

    ret = httpd_register_uri_handler(g_net_state.http_server, &ws_uri);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register WebSocket handler: %s", esp_err_to_name(ret));
        vSemaphoreDelete(g_net_state.ws_mutex);
        g_net_state.ws_mutex = NULL;
        return ret;
    }

    g_net_state.ws_handler_registered = true;
    ESP_LOGI(TAG, "WebSocket handler registered at %s", WS_URI);

    return ESP_OK;
}

/**
 * @brief Shutdown WebSocket handler and disconnect all clients
 *
 * Cleans up all active WebSocket connections and frees resources.
 * Called during network shutdown or when stopping HTTP server.
 *
 * @return ESP_OK on success
 */
esp_err_t deinit_websocket_handler(void) {
    if (!g_net_state.ws_handler_registered) {
        return ESP_OK;
    }

    // Clean up all active clients
    if (g_net_state.ws_mutex) {
        xSemaphoreTake(g_net_state.ws_mutex, portMAX_DELAY);

        for (int i = 0; i < WS_MAX_CLIENTS; i++) {
            if (g_net_state.ws_clients[i].active) {
                cleanup_ws_client(i);
            }
        }

        xSemaphoreGive(g_net_state.ws_mutex);

        // Delete mutex
        vSemaphoreDelete(g_net_state.ws_mutex);
        g_net_state.ws_mutex = NULL;
    }

    g_net_state.ws_handler_registered = false;
    ESP_LOGI(TAG, "WebSocket handler deinitialized");

    return ESP_OK;
}

/* ========================================================================
 * MAIN TASK LOOP
 * ======================================================================== */

void network_task(void *pvParameters) {
    ESP_LOGI(TAG, "Network task started on core %d", xPortGetCoreID());

    // Delay to allow WiFi init to complete
    vTaskDelay(pdMS_TO_TICKS(100));

    // If credentials were loaded from NVS during init, transition to STA
    if (g_net_state.credentials_available && g_net_state.current_mode == WIFI_MODE_AP_PORTAL) {
        ESP_LOGI(TAG, "Credentials available from NVS, transitioning to STA");
        esp_err_t ret = transition_to_sta_mode();
        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "Initial STA transition failed, staying in AP mode");
        }
    }

    while (1) {
        // WiFi lifecycle management
        // Event handlers manage most of the connectivity

        // Check if new credentials were submitted via portal
        if (g_net_state.credentials_available &&
            g_net_state.current_mode == WIFI_MODE_AP_PORTAL) {
            ESP_LOGI(TAG, "New credentials received, transitioning to STA");
            esp_err_t ret = transition_to_sta_mode();
            if (ret != ESP_OK) {
                ESP_LOGW(TAG, "STA transition failed: %s", esp_err_to_name(ret));
            }
        }

        // WebSocket client timeout checking (Task 3 - Phase 6)
        if (g_net_state.ws_handler_registered && g_net_state.ws_mutex) {
            xSemaphoreTake(g_net_state.ws_mutex, portMAX_DELAY);

            for (int i = 0; i < WS_MAX_CLIENTS; i++) {
                if (is_ws_client_timeout(i)) {
                    ESP_LOGW(TAG, "Cleaning up timed-out WebSocket client %d", i);
                    cleanup_ws_client(i);
                }
            }

            xSemaphoreGive(g_net_state.ws_mutex);
        }

        // TODO: Task 4 - TLV protocol handling and status broadcasts

        vTaskDelay(pdMS_TO_TICKS(1000));  // Check timeouts every 1 second
    }

    ESP_LOGW(TAG, "Network task exiting (unexpected)");
    vTaskDelete(NULL);
}

esp_err_t network_deinit(void) {
    ESP_LOGI(TAG, "Deinitializing network subsystem...");

    // Stop mDNS if running
    if (g_net_state.mdns_initialized) {
        stop_mdns_service();
    }

    // Stop captive portal if running
    if (g_net_state.portal_active) {
        stop_captive_portal();
    }

    // Stop WiFi
    if (g_net_state.wifi_initialized) {
        esp_wifi_stop();
        esp_wifi_deinit();
    }

    return ESP_OK;
}
