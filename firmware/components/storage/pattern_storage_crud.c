/**
 * @file pattern_storage_crud.c
 * @brief Pattern CRUD operations for PRISM K1
 *
 * Implements create, read, update, delete, and list operations for pattern files.
 * Enforces ADR-006 bounds (15-25 patterns, 100KB max size).
 */

#include "pattern_storage.h"
#include "esp_log.h"
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <stdio.h>

static const char *TAG = "storage_crud";

// Pattern storage directory
#define PATTERN_DIR     "/littlefs/patterns"
#define MAX_FILENAME    64

/**
 * @brief Helper: Build pattern file path
 * @param pattern_id Pattern identifier
 * @param path Output path buffer
 * @param len Buffer length
 */
static void build_pattern_path(const char *pattern_id, char *path, size_t len) {
    snprintf(path, len, "%s/%s.bin", PATTERN_DIR, pattern_id);
}

esp_err_t storage_pattern_create(const char *pattern_id, const uint8_t *data, size_t len) {
    if (!pattern_id || !data || len == 0) {
        ESP_LOGE(TAG, "Invalid arguments: pattern_id=%p, data=%p, len=%zu",
                 (void*)pattern_id, (void*)data, len);
        return ESP_ERR_INVALID_ARG;
    }

    // Enforce pattern size limit (ADR-006: 100KB)
    if (len > PATTERN_SIZE_MAX) {
        ESP_LOGE(TAG, "Pattern too large: %zu bytes (max %d)", len, PATTERN_SIZE_MAX);
        return ESP_ERR_INVALID_SIZE;
    }

    // Ensure patterns directory exists
    struct stat st;
    if (stat(PATTERN_DIR, &st) != 0) {
        ESP_LOGI(TAG, "Creating patterns directory: %s", PATTERN_DIR);
        if (mkdir(PATTERN_DIR, 0755) != 0) {
            ESP_LOGE(TAG, "Failed to create patterns directory");
            return ESP_ERR_NO_MEM;
        }
    }

    // Check storage bounds (ADR-006: 15-25 patterns)
    size_t count = 0;
    storage_pattern_count(&count);
    if (count >= PATTERN_IDEAL_COUNT) {
        ESP_LOGW(TAG, "Pattern storage full (%zu/%d patterns)", count, PATTERN_IDEAL_COUNT);
        return ESP_ERR_NO_MEM;
    }

    // Build file path
    char path[MAX_FILENAME];
    build_pattern_path(pattern_id, path, sizeof(path));

    // Write pattern to file
    FILE *f = fopen(path, "wb");
    if (!f) {
        ESP_LOGE(TAG, "Failed to create pattern file: %s", path);
        return ESP_ERR_NO_MEM;
    }

    size_t written = fwrite(data, 1, len, f);
    fclose(f);

    if (written != len) {
        ESP_LOGE(TAG, "Failed to write pattern data: wrote %zu/%zu bytes", written, len);
        remove(path);  // Clean up partial write
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Pattern created: %s (%zu bytes)", pattern_id, len);
    return ESP_OK;
}

esp_err_t storage_pattern_read(const char *pattern_id, uint8_t *buffer, size_t buffer_size, size_t *out_size) {
    if (!pattern_id || !buffer || !out_size) {
        ESP_LOGE(TAG, "Invalid arguments: pattern_id=%p, buffer=%p, out_size=%p",
                 (void*)pattern_id, (void*)buffer, (void*)out_size);
        return ESP_ERR_INVALID_ARG;
    }

    char path[MAX_FILENAME];
    build_pattern_path(pattern_id, path, sizeof(path));

    // Check if file exists and get size
    struct stat st;
    if (stat(path, &st) != 0) {
        ESP_LOGW(TAG, "Pattern not found: %s", pattern_id);
        return ESP_ERR_NOT_FOUND;
    }

    // Validate buffer size
    if ((size_t)st.st_size > buffer_size) {
        ESP_LOGE(TAG, "Buffer too small: need %ld bytes, have %zu", st.st_size, buffer_size);
        return ESP_ERR_INVALID_SIZE;
    }

    // Read pattern data
    FILE *f = fopen(path, "rb");
    if (!f) {
        ESP_LOGE(TAG, "Failed to open pattern: %s", path);
        return ESP_ERR_NOT_FOUND;
    }

    size_t bytes_read = fread(buffer, 1, buffer_size, f);
    fclose(f);

    if (bytes_read != (size_t)st.st_size) {
        ESP_LOGE(TAG, "Failed to read complete pattern: read %zu/%ld bytes",
                 bytes_read, st.st_size);
        return ESP_FAIL;
    }

    *out_size = bytes_read;
    ESP_LOGI(TAG, "Pattern read: %s (%zu bytes)", pattern_id, bytes_read);
    return ESP_OK;
}

esp_err_t storage_pattern_delete(const char *pattern_id) {
    if (!pattern_id) {
        ESP_LOGE(TAG, "Invalid argument: pattern_id is NULL");
        return ESP_ERR_INVALID_ARG;
    }

    char path[MAX_FILENAME];
    build_pattern_path(pattern_id, path, sizeof(path));

    // Check if file exists before attempting delete
    struct stat st;
    if (stat(path, &st) != 0) {
        ESP_LOGW(TAG, "Pattern not found for delete: %s", pattern_id);
        return ESP_ERR_NOT_FOUND;
    }

    // Delete the file
    if (remove(path) != 0) {
        ESP_LOGE(TAG, "Failed to delete pattern: %s", pattern_id);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Pattern deleted: %s", pattern_id);
    return ESP_OK;
}

esp_err_t storage_pattern_list(char **pattern_list, size_t max_count, size_t *out_count) {
    if (!pattern_list || !out_count) {
        ESP_LOGE(TAG, "Invalid arguments: pattern_list=%p, out_count=%p",
                 (void*)pattern_list, (void*)out_count);
        return ESP_ERR_INVALID_ARG;
    }

    *out_count = 0;  // Initialize output

    DIR *dir = opendir(PATTERN_DIR);
    if (!dir) {
        ESP_LOGW(TAG, "Patterns directory not found: %s", PATTERN_DIR);
        return ESP_OK;  // Empty list is valid
    }

    size_t count = 0;
    struct dirent *entry;

    while ((entry = readdir(dir)) != NULL && count < max_count) {
        // Skip . and ..
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        // Extract pattern ID (remove .bin extension)
        char *dot = strrchr(entry->d_name, '.');
        if (dot && strcmp(dot, ".bin") == 0) {
            size_t id_len = dot - entry->d_name;
            if (id_len > 0 && id_len < MAX_FILENAME) {
                strncpy(pattern_list[count], entry->d_name, id_len);
                pattern_list[count][id_len] = '\0';
                count++;
            }
        }
    }

    closedir(dir);
    *out_count = count;

    ESP_LOGI(TAG, "Pattern list: %zu patterns found", count);
    return ESP_OK;
}

esp_err_t storage_pattern_count(size_t *out_count) {
    if (!out_count) {
        ESP_LOGE(TAG, "Invalid argument: out_count is NULL");
        return ESP_ERR_INVALID_ARG;
    }

    *out_count = 0;  // Initialize output

    DIR *dir = opendir(PATTERN_DIR);
    if (!dir) {
        // Directory doesn't exist yet - return 0 count
        return ESP_OK;
    }

    size_t count = 0;
    struct dirent *entry;

    while ((entry = readdir(dir)) != NULL) {
        // Skip . and ..
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        // Count all files (assume they're all patterns)
        count++;
    }

    closedir(dir);
    *out_count = count;

    ESP_LOGD(TAG, "Pattern count: %zu", count);
    return ESP_OK;
}
