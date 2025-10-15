# Network Manager Test Suite

## Overview

This directory contains Unity-based unit and integration tests for the network_manager component.

## Test Categories

### 1. Network Initialization Tests
- `test_network_init_success` - Verifies complete initialization sequence
- `test_network_init_nvs_failure` - Tests NVS failure handling

### 2. NVS Credential Persistence Tests
- `test_save_credentials_success` - Verifies credential storage
- `test_load_credentials_success` - Verifies credential retrieval
- `test_load_credentials_not_found` - Tests missing credential handling

### 3. Captive Portal Tests
- `test_start_captive_portal_success` - Verifies HTTP server startup
- `test_portal_form_parsing` - Tests URL-encoded form parsing
- `test_portal_form_parsing_spaces` - Tests space handling in form data

### 4. Reconnection Logic Tests
- `test_exponential_backoff_timing` - Verifies backoff schedule (1s → 2s → 4s → 8s → 16s → 30s)
- `test_wifi_disconnect_reconnect` - Tests disconnect event handling
- `test_connection_success_resets_counters` - Verifies retry counter reset

### 5. mDNS Tests
- `test_mdns_service_start` - Verifies mDNS registration
- `test_mdns_txt_records` - Validates TXT record content
- `test_mdns_stop_idempotent` - Tests safe repeated stops

### 6. Integration Tests
- `test_integration_portal_to_sta` - Full portal-to-connection flow
- `test_integration_boot_with_credentials` - Auto-connect on boot
- `test_integration_network_dropout` - Network recovery testing

### 7. Memory Management Tests
- `test_memory_pool_usage_portal` - Verifies prism_pool_alloc usage
- `test_no_memory_leaks` - Long-running leak detection

## Running Tests

### Unit Tests (Host-based with Mocks)
```bash
cd firmware
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

### Hardware Integration Tests
```bash
# Flash test firmware to ESP32-S3
cd firmware/components/network/test
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor

# Observe test results in serial output
```

### Manual Test Procedures

#### Captive Portal Flow
1. Power on device (no stored credentials)
2. Connect to "PRISM-SETUP" AP from phone/laptop
3. Captive portal should auto-open
4. Submit WiFi credentials via form
5. Verify device connects to specified network
6. Verify `prism-k1.local` resolves via mDNS

#### Reconnection Testing
1. Connect device to WiFi network
2. Power cycle router or move device out of range
3. Monitor serial logs for reconnection attempts
4. Verify exponential backoff delays: 1s, 2s, 4s, 8s, 16s, 30s
5. Verify continuous retry after 5 attempts
6. Restore network connectivity
7. Verify successful reconnection and counter reset

#### Persistence Testing
1. Submit credentials via portal
2. Verify connection success
3. Power cycle device
4. Verify device auto-connects on boot without portal
5. Verify mDNS advertises `prism-k1.local`

#### mDNS Discovery
```bash
# Linux
avahi-browse -r _prism._tcp

# macOS
dns-sd -B _prism._tcp

# Expected output:
# prism-k1._prism._tcp.local
#   version=1.0
#   device=prism-k1
#   leds=320
```

## Mock Requirements

Tests marked with `TEST_PASS_MESSAGE("Mock-based test...")` require:
- ESP-IDF Unity test framework
- Mock implementations of:
  - `esp_wifi` API
  - `esp_netif` API
  - `nvs` API
  - `mdns` API
  - `esp_http_server` API
  - `esp_event` loop

## Test Coverage Goals

- **Unit Test Coverage:** >80% of network_manager.c functions
- **Integration Test Coverage:** All critical user flows
- **Hardware Validation:** Portal, reconnection, mDNS on actual ESP32-S3

## Known Limitations

- Mock framework not yet implemented (tests are documented placeholders)
- Hardware-in-the-loop tests require manual execution
- Long-running reliability tests (48+ hours) are manual procedures

## Future Enhancements

1. Implement complete mock framework
2. Add automated hardware-in-the-loop testing
3. Add fuzzing for form parsing
4. Add stress testing (rapid connect/disconnect cycles)
5. Add concurrent client testing (multiple devices connecting to AP)
