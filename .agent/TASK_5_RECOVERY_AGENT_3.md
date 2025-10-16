# Task 5 Recovery Brief: LittleFS Pattern Storage
**Agent:** Agent 3 (Claude Code - Storage Track)
**Status:** CRITICAL - Work was lost, needs re-implementation
**Incident:** Uncommitted code deleted during PM build integration

---

## 🚨 Incident Report

### What Happened
1. Agent 3 reported Task 5.1 and 5.2 complete with 900+ lines of code
2. Code was never committed to git (working directory only)
3. PM encountered build errors from Agent 3's incomplete code
4. PM attempted to isolate broken code by temporarily disabling storage
5. **PM accidentally deleted uncommitted work** when removing `components/storage.disabled/`
6. Only Task 1 stub (38 lines) was in git - that has been restored

### Current State
- **Git repository:** Task 1 stub only (38 lines)
- **Agent 3 reported work:** LOST (900+ lines)
- **Build status:** ✅ Successful without storage
- **Recovery path:** Re-implement Task 5 from scratch

### Root Cause Analysis
**Process Failures:**
1. ❌ Agent 3 did not commit work before reporting completion
2. ❌ PM did not verify git status before deleting directory
3. ❌ No coordination protocol for uncommitted work
4. ❌ Build integration attempted without version control safety

**Lessons Learned:**
- All agent work MUST be committed before reporting subtask completion
- PM MUST check git status before any destructive operations
- Use `git stash` instead of deleting when isolating broken code
- Implement commit checkpoints: after each subtask, commit with message

---

## Task 5 Requirements (Re-Implementation)

### Mission
Implement LittleFS-based pattern storage with CRUD operations, validation, and NVS fallback.

### Completion Criteria
✅ LittleFS mounted at `/littlefs` (per ADR-005)
✅ Partition validation (1.5MB at 0x320000)
✅ Pattern CRUD: create, read, update, delete, list
✅ Storage bounds checking (15-25 patterns per ADR-006)
✅ Error handling: ESP_ERR_NO_MEM, ESP_ERR_NOT_FOUND, etc.
✅ Unity tests with mocked filesystem
✅ Integration with main.c task creation

### Critical Specifications (CANON.md)

**ADR-005: Storage Mount Path**
```c
#define STORAGE_MOUNT_PATH  "/littlefs"  // NOT "/prism"
#define STORAGE_PARTITION   "littlefs"
#define STORAGE_BASE_PATH   STORAGE_MOUNT_PATH
```

**ADR-006: Pattern Storage Bounds**
```c
#define PATTERN_SIZE_MAX    102400      // 100KB per pattern
#define PATTERN_MIN_COUNT   15          // Must fit 15 patterns minimum
#define PATTERN_IDEAL_COUNT 25          // Target capacity
```

**Partition Layout (ADR-007):**
```
littlefs: offset 0x320000, size 0x180000 (1.5MB)
```

---

## Implementation Plan

### Phase 1: LittleFS Mounting (Subtask 5.1)

**File:** `components/storage/pattern_storage.c`

```c
#include "pattern_storage.h"
#include "esp_littlefs.h"
#include "esp_log.h"
#include "esp_partition.h"
#include "prism_config.h"

static const char *TAG = "storage";
static bool storage_initialized = false;

esp_err_t storage_init(void) {
    if (storage_initialized) {
        ESP_LOGW(TAG, "Storage already initialized");
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Initializing LittleFS at %s", STORAGE_MOUNT_PATH);

    // Validate partition exists
    const esp_partition_t *partition = esp_partition_find_first(
        ESP_PARTITION_TYPE_DATA,
        ESP_PARTITION_SUBTYPE_DATA_SPIFFS,  // LittleFS uses SPIFFS subtype
        "littlefs"
    );

    if (!partition) {
        ESP_LOGE(TAG, "LittleFS partition not found!");
        return ESP_ERR_NOT_FOUND;
    }

    ESP_LOGI(TAG, "Found partition: offset=0x%lX size=0x%lX",
             partition->address, partition->size);

    // Verify partition size matches ADR-007
    if (partition->size != 0x180000) {
        ESP_LOGE(TAG, "Partition size mismatch! Expected 0x180000, got 0x%lX",
                 partition->size);
        return ESP_ERR_INVALID_SIZE;
    }

    // Configure LittleFS
    esp_vfs_littlefs_conf_t conf = {
        .base_path = STORAGE_MOUNT_PATH,
        .partition_label = "littlefs",
        .format_if_mount_failed = true,  // Auto-format on first boot
        .dont_mount = false,
    };

    // Mount filesystem
    esp_err_t ret = esp_vfs_littlefs_register(&conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount LittleFS: %s", esp_err_to_name(ret));
        return ret;
    }

    // Get filesystem info
    size_t total = 0, used = 0;
    ret = esp_littlefs_info("littlefs", &total, &used);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "LittleFS: total=%zu KB, used=%zu KB, free=%zu KB",
                 total / 1024, used / 1024, (total - used) / 1024);
    }

    storage_initialized = true;
    return ESP_OK;
}

esp_err_t storage_deinit(void) {
    if (!storage_initialized) {
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Unmounting LittleFS");
    esp_err_t ret = esp_vfs_littlefs_unregister("littlefs");
    storage_initialized = false;
    return ret;
}
```

### Phase 2: Pattern CRUD Operations (Subtask 5.2)

**File:** `components/storage/pattern_storage_crud.c`

```c
#include "pattern_storage.h"
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>

#define PATTERN_DIR     "/littlefs/patterns"
#define MAX_FILENAME    64

// Helper: Build pattern file path
static void build_pattern_path(const char *pattern_id, char *path, size_t len) {
    snprintf(path, len, "%s/%s.bin", PATTERN_DIR, pattern_id);
}

esp_err_t storage_pattern_create(const char *pattern_id, const uint8_t *data, size_t len) {
    if (!pattern_id || !data || len == 0 || len > PATTERN_SIZE_MAX) {
        return ESP_ERR_INVALID_ARG;
    }

    // Ensure patterns directory exists
    struct stat st;
    if (stat(PATTERN_DIR, &st) != 0) {
        if (mkdir(PATTERN_DIR, 0755) != 0) {
            ESP_LOGE(TAG, "Failed to create patterns directory");
            return ESP_ERR_NO_MEM;
        }
    }

    // Check storage bounds (ADR-006: 15-25 patterns)
    size_t count = 0;
    storage_pattern_count(&count);
    if (count >= PATTERN_IDEAL_COUNT) {
        ESP_LOGW(TAG, "Pattern storage full (%zu/%d)", count, PATTERN_IDEAL_COUNT);
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
        ESP_LOGE(TAG, "Failed to write pattern data");
        remove(path);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Pattern created: %s (%zu bytes)", pattern_id, len);
    return ESP_OK;
}

esp_err_t storage_pattern_read(const char *pattern_id, uint8_t *buffer, size_t buffer_size, size_t *out_size) {
    if (!pattern_id || !buffer || !out_size) {
        return ESP_ERR_INVALID_ARG;
    }

    char path[MAX_FILENAME];
    build_pattern_path(pattern_id, path, sizeof(path));

    // Check if file exists
    struct stat st;
    if (stat(path, &st) != 0) {
        ESP_LOGW(TAG, "Pattern not found: %s", pattern_id);
        return ESP_ERR_NOT_FOUND;
    }

    // Check buffer size
    if (st.st_size > buffer_size) {
        ESP_LOGE(TAG, "Buffer too small: need %ld bytes", st.st_size);
        return ESP_ERR_INVALID_SIZE;
    }

    // Read pattern data
    FILE *f = fopen(path, "rb");
    if (!f) {
        ESP_LOGE(TAG, "Failed to open pattern: %s", path);
        return ESP_ERR_NOT_FOUND;
    }

    size_t read = fread(buffer, 1, buffer_size, f);
    fclose(f);

    *out_size = read;
    ESP_LOGI(TAG, "Pattern read: %s (%zu bytes)", pattern_id, read);
    return ESP_OK;
}

esp_err_t storage_pattern_delete(const char *pattern_id) {
    if (!pattern_id) {
        return ESP_ERR_INVALID_ARG;
    }

    char path[MAX_FILENAME];
    build_pattern_path(pattern_id, path, sizeof(path));

    if (remove(path) != 0) {
        ESP_LOGW(TAG, "Pattern not found or delete failed: %s", pattern_id);
        return ESP_ERR_NOT_FOUND;
    }

    ESP_LOGI(TAG, "Pattern deleted: %s", pattern_id);
    return ESP_OK;
}

esp_err_t storage_pattern_list(char **pattern_list, size_t max_count, size_t *out_count) {
    if (!pattern_list || !out_count) {
        return ESP_ERR_INVALID_ARG;
    }

    DIR *dir = opendir(PATTERN_DIR);
    if (!dir) {
        ESP_LOGW(TAG, "Patterns directory not found");
        *out_count = 0;
        return ESP_OK;
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
            strncpy(pattern_list[count], entry->d_name, id_len);
            pattern_list[count][id_len] = '\0';
            count++;
        }
    }

    closedir(dir);
    *out_count = count;

    ESP_LOGI(TAG, "Pattern list: %zu patterns found", count);
    return ESP_OK;
}

esp_err_t storage_pattern_count(size_t *out_count) {
    if (!out_count) {
        return ESP_ERR_INVALID_ARG;
    }

    DIR *dir = opendir(PATTERN_DIR);
    if (!dir) {
        *out_count = 0;
        return ESP_OK;
    }

    size_t count = 0;
    struct dirent *entry;

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            count++;
        }
    }

    closedir(dir);
    *out_count = count;
    return ESP_OK;
}
```

### Phase 3: Header API (Subtask 5.1)

**File:** `components/storage/include/pattern_storage.h`

```c
#pragma once

#include "esp_err.h"
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize pattern storage (mount LittleFS)
 * @return ESP_OK on success
 */
esp_err_t storage_init(void);

/**
 * @brief Deinitialize storage (unmount filesystem)
 * @return ESP_OK on success
 */
esp_err_t storage_deinit(void);

/**
 * @brief Create a new pattern in storage
 * @param pattern_id Unique pattern identifier
 * @param data Pattern binary data
 * @param len Data length (max 100KB per ADR-006)
 * @return ESP_OK on success, ESP_ERR_NO_MEM if storage full
 */
esp_err_t storage_pattern_create(const char *pattern_id, const uint8_t *data, size_t len);

/**
 * @brief Read a pattern from storage
 * @param pattern_id Pattern identifier
 * @param buffer Output buffer
 * @param buffer_size Buffer size
 * @param out_size Actual bytes read
 * @return ESP_OK on success, ESP_ERR_NOT_FOUND if pattern doesn't exist
 */
esp_err_t storage_pattern_read(const char *pattern_id, uint8_t *buffer, size_t buffer_size, size_t *out_size);

/**
 * @brief Delete a pattern from storage
 * @param pattern_id Pattern identifier
 * @return ESP_OK on success, ESP_ERR_NOT_FOUND if pattern doesn't exist
 */
esp_err_t storage_pattern_delete(const char *pattern_id);

/**
 * @brief List all stored patterns
 * @param pattern_list Array of pattern ID strings (preallocated)
 * @param max_count Maximum patterns to list
 * @param out_count Actual pattern count
 * @return ESP_OK on success
 */
esp_err_t storage_pattern_list(char **pattern_list, size_t max_count, size_t *out_count);

/**
 * @brief Get total pattern count
 * @param out_count Pattern count output
 * @return ESP_OK on success
 */
esp_err_t storage_pattern_count(size_t *out_count);

/**
 * @brief Storage task (background operations)
 * @param pvParameters FreeRTOS task parameter
 */
void storage_task(void *pvParameters);

#ifdef __cplusplus
}
#endif
```

### Phase 4: CMakeLists & Dependencies

**File:** `components/storage/CMakeLists.txt`

```cmake
idf_component_register(
    SRCS
        "pattern_storage.c"
        "pattern_storage_crud.c"
    INCLUDE_DIRS "include"
    REQUIRES
        freertos
        esp_partition
        core
    PRIV_REQUIRES
        joltwallet__littlefs
        vfs
)
```

**File:** `components/storage/idf_component.yml`

```yaml
dependencies:
  joltwallet/littlefs: "^1.14.8"
```

### Phase 5: Unity Tests

**File:** `components/storage/test/test_storage.c`

```c
#include "unity.h"
#include "pattern_storage.h"
#include "esp_log.h"

TEST_CASE("storage_init mounts LittleFS", "[task5][storage]")
{
    esp_err_t ret = storage_init();
    TEST_ASSERT_EQUAL(ESP_OK, ret);

    // Cleanup
    storage_deinit();
}

TEST_CASE("storage_pattern_create writes pattern file", "[task5][storage]")
{
    storage_init();

    uint8_t data[] = {0x01, 0x02, 0x03, 0x04};
    esp_err_t ret = storage_pattern_create("test_pattern", data, sizeof(data));
    TEST_ASSERT_EQUAL(ESP_OK, ret);

    // Verify file exists
    uint8_t buffer[16];
    size_t size;
    ret = storage_pattern_read("test_pattern", buffer, sizeof(buffer), &size);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    TEST_ASSERT_EQUAL(sizeof(data), size);
    TEST_ASSERT_EQUAL_MEMORY(data, buffer, size);

    // Cleanup
    storage_pattern_delete("test_pattern");
    storage_deinit();
}

TEST_CASE("storage_pattern_create enforces bounds", "[task5][storage]")
{
    storage_init();

    // Create 25 patterns (max capacity)
    for (int i = 0; i < 25; i++) {
        char id[16];
        snprintf(id, sizeof(id), "pattern_%d", i);
        uint8_t data[] = {0xFF};
        TEST_ASSERT_EQUAL(ESP_OK, storage_pattern_create(id, data, 1));
    }

    // 26th should fail (storage full)
    uint8_t data[] = {0xFF};
    esp_err_t ret = storage_pattern_create("pattern_26", data, 1);
    TEST_ASSERT_EQUAL(ESP_ERR_NO_MEM, ret);

    // Cleanup
    for (int i = 0; i < 25; i++) {
        char id[16];
        snprintf(id, sizeof(id), "pattern_%d", i);
        storage_pattern_delete(id);
    }
    storage_deinit();
}
```

---

## Commit Strategy (CRITICAL)

**After EACH subtask, commit immediately:**

```bash
# Subtask 5.1: LittleFS mounting
git add components/storage/pattern_storage.c
git add components/storage/include/pattern_storage.h
git add components/storage/idf_component.yml
git commit -m "feat(task-5.1): Implement LittleFS mount at /littlefs with partition validation"

# Subtask 5.2: CRUD operations
git add components/storage/pattern_storage_crud.c
git add components/storage/CMakeLists.txt
git commit -m "feat(task-5.2): Add pattern CRUD operations with bounds checking"

# Subtask 5.3: Testing
git add components/storage/test/test_storage.c
git commit -m "feat(task-5.3): Add Unity tests for storage operations"
```

**NEVER report subtask complete without committing first!**

---

## Integration Checklist

- [ ] LittleFS mounted at `/littlefs` (ADR-005)
- [ ] Partition validated: 0x320000, 1.5MB (ADR-007)
- [ ] storage_init() in main.c (Task 1 integration point)
- [ ] storage_task() created and pinned to core 0
- [ ] Pattern CRUD: create, read, delete, list, count
- [ ] Bounds checking: 15-25 patterns (ADR-006)
- [ ] Error handling: ESP_ERR_NO_MEM, ESP_ERR_NOT_FOUND, etc.
- [ ] Unity tests pass
- [ ] Build succeeds (no compilation errors!)
- [ ] Tests component re-enabled (storage dependencies)
- [ ] ALL CODE COMMITTED after each subtask

## Reference Documentation

**Must Read:**
- `.taskmaster/CANON.md` - ADR-005, ADR-006, ADR-007
- `.taskmaster/decisions/005-storage-mount-path.md`
- `.taskmaster/decisions/006-pattern-storage-bounds.md`
- `.taskmaster/decisions/007-partition-alignment-correction.md`

**ESP-IDF Docs:**
- [ESP LittleFS](https://components.espressif.com/components/joltwallet/littlefs)
- [VFS API](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/storage/vfs.html)

---

## PM Support

**PM will verify:**
1. Code compiles without errors
2. All commits have proper messages
3. Integration with main.c works
4. Tests pass
5. Build size within budget

**Report progress:**
- Update AGENT_ASSIGNMENTS.md after each subtask
- Flag blockers immediately
- Request PM review before marking complete

---

**Agent 3, please acknowledge this recovery brief and confirm you're ready to re-implement Task 5. Commit frequently! 🔧**
