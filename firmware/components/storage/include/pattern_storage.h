/**
 * @file pattern_storage.h
 * @brief Pattern storage manager for PRISM K1
 *
 * Manages LittleFS filesystem for pattern persistence.
 * Implements ADR-005 (/littlefs mount path), ADR-006 (15-25 patterns),
 * and ADR-007 (partition at 0x320000, 1.5MB).
 */

#ifndef PRISM_PATTERN_STORAGE_H
#define PRISM_PATTERN_STORAGE_H

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "prism_config.h"
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// ADR-005: Storage mount path
#define STORAGE_MOUNT_PATH  "/littlefs"
#define STORAGE_PARTITION   "littlefs"

// ADR-004/ADR-006: Pattern storage bounds
// Use canonical maximum size from CANON (256KB), minimum count from ADR-006
#define PATTERN_SIZE_MAX    PATTERN_MAX_SIZE   // 256KB per pattern (SoT)
#define PATTERN_MIN_COUNT   15                 // Must fit 15 patterns minimum
#define PATTERN_IDEAL_COUNT 25                 // Target capacity

/**
 * @brief Initialize storage subsystem
 *
 * Mounts LittleFS at /littlefs (ADR-005) and validates partition (ADR-007).
 * Auto-formats filesystem on first boot.
 *
 * @return ESP_OK on success
 * @return ESP_ERR_NOT_FOUND if partition not found
 * @return ESP_ERR_INVALID_SIZE if partition size mismatch
 */
esp_err_t storage_init(void);

/**
 * @brief Deinitialize storage subsystem
 *
 * Unmounts filesystem and cleanup.
 *
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t storage_deinit(void);

/**
 * @brief Storage task entry point
 *
 * FreeRTOS task for pattern file operations and cache management.
 *
 * Stack: 6KB
 * Priority: 4 (medium-low)
 * Core: 0
 *
 * @param pvParameters Task parameters (unused)
 */
void storage_task(void *pvParameters);

/**
 * @brief Create a new pattern in storage
 *
 * Writes pattern binary data to filesystem. Enforces ADR-006 bounds:
 * - Pattern size: max 100KB
 * - Pattern count: max 25 patterns
 *
 * @param pattern_id Unique pattern identifier (no extension)
 * @param data Pattern binary data
 * @param len Data length in bytes (max PATTERN_SIZE_MAX)
 * @return ESP_OK on success
 * @return ESP_ERR_INVALID_ARG if parameters are invalid
 * @return ESP_ERR_INVALID_SIZE if pattern exceeds size limit
 * @return ESP_ERR_NO_MEM if storage is full (25 patterns)
 */
esp_err_t storage_pattern_create(const char *pattern_id, const uint8_t *data, size_t len);

/**
 * @brief Read a pattern from storage
 *
 * Retrieves pattern binary data from filesystem.
 *
 * @param pattern_id Pattern identifier
 * @param buffer Output buffer for pattern data
 * @param buffer_size Size of output buffer
 * @param out_size Actual bytes read (output parameter)
 * @return ESP_OK on success
 * @return ESP_ERR_INVALID_ARG if parameters are invalid
 * @return ESP_ERR_NOT_FOUND if pattern doesn't exist
 * @return ESP_ERR_INVALID_SIZE if buffer is too small
 */
esp_err_t storage_pattern_read(const char *pattern_id, uint8_t *buffer, size_t buffer_size, size_t *out_size);

/**
 * @brief Delete a pattern from storage
 *
 * Removes pattern file from filesystem.
 *
 * @param pattern_id Pattern identifier
 * @return ESP_OK on success
 * @return ESP_ERR_INVALID_ARG if pattern_id is NULL
 * @return ESP_ERR_NOT_FOUND if pattern doesn't exist
 */
esp_err_t storage_pattern_delete(const char *pattern_id);

/**
 * @brief List all stored patterns
 *
 * Enumerates pattern files in storage directory.
 *
 * @param pattern_list Array of pattern ID strings (caller must allocate)
 * @param max_count Maximum patterns to list
 * @param out_count Actual pattern count (output parameter)
 * @return ESP_OK on success
 * @return ESP_ERR_INVALID_ARG if parameters are invalid
 */
esp_err_t storage_pattern_list(char **pattern_list, size_t max_count, size_t *out_count);

/**
 * @brief Get total pattern count
 *
 * Returns number of patterns currently stored.
 *
 * @param out_count Pattern count (output parameter)
 * @return ESP_OK on success
 * @return ESP_ERR_INVALID_ARG if out_count is NULL
 */
esp_err_t storage_pattern_count(size_t *out_count);

// ============================================================================
// Template Storage API (Atomic Write Semantics)
// ============================================================================

/**
 * @brief Write template data with atomic semantics
 *
 * Writes template to temporary file, syncs to disk, then atomically renames
 * to final location. Guarantees no partial writes are visible.
 *
 * Flow: <name>.tmp → fflush → fsync → rename → <name>
 *
 * @param template_id Template identifier (e.g., "palette.dat", "template_001")
 * @param data Template binary data
 * @param len Data length in bytes
 * @return ESP_OK on success
 * @return ESP_ERR_INVALID_ARG if parameters are invalid
 * @return ESP_ERR_NO_MEM if filesystem error
 * @return ESP_FAIL if write/sync/rename fails (temp file cleaned up)
 */
esp_err_t template_storage_write(const char *template_id, const uint8_t *data, size_t len);

/**
 * @brief Read template data from storage
 *
 * Retrieves template binary data from /littlefs/templates/
 *
 * @param template_id Template identifier
 * @param buffer Output buffer for template data
 * @param buffer_size Size of output buffer
 * @param out_size Actual bytes read (output parameter)
 * @return ESP_OK on success
 * @return ESP_ERR_INVALID_ARG if parameters are invalid
 * @return ESP_ERR_NOT_FOUND if template doesn't exist
 * @return ESP_ERR_INVALID_SIZE if buffer is too small
 */
esp_err_t template_storage_read(const char *template_id, uint8_t *buffer, size_t buffer_size, size_t *out_size);

/**
 * @brief List all stored templates
 *
 * Enumerates template files in /littlefs/templates/, skipping temporary files.
 *
 * @param template_list Array of template ID strings (caller must allocate)
 * @param max_count Maximum templates to list
 * @param out_count Actual template count (output parameter)
 * @return ESP_OK on success
 * @return ESP_ERR_INVALID_ARG if parameters are invalid
 */
esp_err_t template_storage_list(char **template_list, size_t max_count, size_t *out_count);

/**
 * @brief Delete a template from storage
 *
 * Removes template file and any leftover temporary files.
 *
 * @param template_id Template identifier
 * @return ESP_OK on success
 * @return ESP_ERR_INVALID_ARG if template_id is NULL
 * @return ESP_ERR_NOT_FOUND if template doesn't exist
 */
esp_err_t template_storage_delete(const char *template_id);

#ifdef __cplusplus
}
#endif

#endif // PRISM_PATTERN_STORAGE_H
