# AGENT 1 - TASK 10 RESUMPTION BRIEF

**Agent ID:** AGENT-1-FIRMWARE (NEW SESSION)
**Task:** Task 10 - Template System with 15 Patterns
**Status:** IN PROGRESS (5 subtasks pending)
**Handoff Date:** 2025-10-17
**Previous Agent:** Ran out of context after basic scaffolding

---

## 🎯 MISSION OBJECTIVE

Complete Task 10: Deliver template system with 15 categorized patterns and deployment flow.

**Success Criteria:**
- 15 .prism templates embedded and provisioned on first boot
- <60s first-boot provisioning time
- Templates cached at boot via `pattern_cache_preload()`
- WebSocket CONTROL commands trigger deployment
- <2s deployment, <100ms playback switch
- Total templates fit in 1.5MB partition

---

## ✅ WHAT PREVIOUS AGENT COMPLETED

### Basic Infrastructure Only

**Files Created:**
- `firmware/components/templates/template_manager.h` - API stubs
- `firmware/components/templates/template_manager.c` - Shell implementation
- `firmware/components/templates/CMakeLists.txt` - Build config

**Current Implementation:**
```c
// template_manager.c (lines 14-30)
static const char* s_templates[15] = {
    "canvas_solid", "canvas_gradient", "canvas_wave_slow",
    "canvas_wave_fast", "canvas_twinkle", "canvas_sparkle",
    "canvas_breath", "canvas_rainbow", "canvas_wipe_left",
    "canvas_wipe_right", "canvas_progressive", "canvas_offset",
    "canvas_sync", "canvas_dualwave", "canvas_custom",
};

esp_err_t templates_init(void) {
    ESP_LOGI(TAG, "Initializing template subsystem...");
    register_templates_cli();
    // TODO: Line 58 - provision presets to /littlefs/templates
    ESP_LOGI(TAG, "Provisioned %zu templates (virtual catalog)", 15);
    return ESP_OK;
}
```

**What Works:**
- ✅ CLI command `prism_templates` lists template names
- ✅ Integrated into `main.c:172` (called at boot)
- ✅ Firmware builds successfully

**What DOESN'T Work:**
- ❌ No actual .prism files provisioned to LittleFS
- ❌ No template binary data embedded
- ❌ No cache preloading
- ❌ No WebSocket integration
- ❌ No performance validation

---

## 📋 REMAINING WORK - 5 SUBTASKS

### Subtask 10.1: Define Built-in Template Descriptors
**Status:** PENDING (START HERE)
**File:** `firmware/components/templates/template_patterns.c` (NEW FILE)

**Requirements:**
1. Create descriptor structure:
```c
typedef struct {
    const char* id;
    const char* category;  // "ambient", "energy", "special"
    size_t size;
    const uint8_t* data;   // Embedded .prism binary
} template_desc_t;
```

2. Embed 15 .prism files as binary data:
```c
// Use xxd -i or similar to convert .prism → C arrays
static const uint8_t flow_horizon_data[] = {
    0x50, 0x52, 0x49, 0x53, 0x4d, ...  // PRISM header + payload
};
```

3. Create descriptor table:
```c
static const template_desc_t builtin_templates[15] = {
    {"flow-horizon", "ambient", sizeof(flow_horizon_data), flow_horizon_data},
    // ... 14 more entries
};

const template_desc_t* template_catalog_get(size_t* out_count) {
    *out_count = 15;
    return builtin_templates;
}
```

4. Update `CMakeLists.txt` to include `template_patterns.c`

**Source Files Available:**
- 17 .prism files in `/Users/spectrasynq/Workspace_Management/Software/PRISM.k1/out/presets/`
- Use best 15 for embedded catalog

**Validation:**
- Total size <1.5MB (check with static assert)
- Each .prism has valid v1.1 header (led_count=160, frame_count≥1)

---

### Subtask 10.2: Implement First-Boot Template Provisioning
**Status:** PENDING
**File:** `firmware/components/templates/template_manager.c` (MODIFY)

**Requirements:**
1. Implement provisioning logic in `templates_init()`:
```c
esp_err_t templates_init(void) {
    ESP_LOGI(TAG, "Initializing template subsystem...");
    register_templates_cli();

    size_t catalog_count = 0;
    const template_desc_t* catalog = template_catalog_get(&catalog_count);

    for (size_t i = 0; i < catalog_count; i++) {
        char path[64];
        snprintf(path, sizeof(path), "/littlefs/templates/%s.prism", catalog[i].id);

        // Check if template already exists
        if (pattern_storage_exists(catalog[i].id)) {
            ESP_LOGD(TAG, "Template '%s' already exists, skipping", catalog[i].id);
            continue;
        }

        // Write template binary to LittleFS
        esp_err_t err = pattern_storage_write(catalog[i].id,
                                               catalog[i].data,
                                               catalog[i].size,
                                               true);  // atomic
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to provision '%s': %s",
                     catalog[i].id, esp_err_to_name(err));
            continue;
        }

        ESP_LOGI(TAG, "Provisioned template '%s' (%zu bytes)",
                 catalog[i].id, catalog[i].size);
    }

    ESP_LOGI(TAG, "Template provisioning complete (%zu templates)", catalog_count);
    return ESP_OK;
}
```

2. Use existing storage API:
   - `pattern_storage_exists()` - check if file present
   - `pattern_storage_write()` - write .prism binary
   - Both defined in `firmware/components/storage/include/pattern_storage.h`

**Validation:**
- First boot: All 15 templates written to `/littlefs/templates/`
- Second boot: No rewrites (idempotent check)
- <60s provisioning time

---

### Subtask 10.3: Register Metadata and Cache Preload
**Status:** PENDING
**File:** `firmware/components/templates/template_manager.c` (MODIFY)

**Requirements:**
1. After provisioning, preload all templates into cache:
```c
// Add after provisioning loop in templates_init()
for (size_t i = 0; i < catalog_count; i++) {
    const uint8_t* cached_data = NULL;
    size_t cached_size = 0;

    // Try to get from cache (will load from storage if not cached)
    if (pattern_cache_try_get(catalog[i].id, &cached_data, &cached_size)) {
        ESP_LOGD(TAG, "Template '%s' already cached", catalog[i].id);
    } else {
        // Load from storage and cache it
        uint8_t* data = NULL;
        size_t size = 0;
        esp_err_t err = pattern_storage_read(catalog[i].id, &data, &size);
        if (err == ESP_OK) {
            pattern_cache_put_copy(catalog[i].id, data, size);
            free(data);
            ESP_LOGI(TAG, "Cached template '%s'", catalog[i].id);
        }
    }
}
```

2. Create `templates_list()` API:
```c
esp_err_t templates_list(const char* category, char*** out_ids, size_t* out_count) {
    size_t catalog_count = 0;
    const template_desc_t* catalog = template_catalog_get(&catalog_count);

    // Filter by category if specified
    // Allocate array of string pointers
    // Return filtered list
}
```

3. Create `templates_deploy()` API:
```c
esp_err_t templates_deploy(const char* template_id) {
    // Load from cache (or storage if not cached)
    // Trigger playback switch
    // Return ESP_OK on success
}
```

**Validation:**
- Cache stats show 15 entries cached after boot
- `templates_list()` returns correct counts per category
- `templates_deploy()` switches patterns in <100ms

---

### Subtask 10.4: Wire Deployment Flow Through Protocol Dispatcher
**Status:** PENDING
**File:** `firmware/components/network/protocol_parser.c` (MODIFY)

**Requirements:**
1. Add CONTROL command handler for template deployment:
```c
// In handle_control_command() or similar
case CONTROL_DEPLOY_TEMPLATE: {
    char template_id[64];
    // Parse template_id from TLV payload

    esp_err_t err = templates_deploy(template_id);
    if (err == ESP_OK) {
        // Send success TLV response
        send_tlv_response(client, MSG_TYPE_SUCCESS, NULL, 0);
    } else {
        // Send error TLV response
        send_tlv_response(client, MSG_TYPE_ERROR, NULL, 0);
    }
    break;
}
```

2. Update STATUS command to include template metadata:
```c
// In handle_status_command()
typedef struct {
    uint8_t version[4];
    uint16_t led_count;
    uint32_t storage_available;
    uint16_t max_chunk_size;
    uint8_t template_count;  // NEW
    // ... rest of status fields
} __attribute__((packed)) status_payload_t;

status_payload_t status = {
    // ... existing fields
    .template_count = 15,  // From catalog
};
```

**Validation:**
- WebSocket CONTROL command triggers `templates_deploy()`
- STATUS response includes template count
- Deployment completes in <2s

---

### Subtask 10.5: Validate Storage Size and Deployment Performance
**Status:** PENDING

**Requirements:**
1. Add instrumentation in `templates_init()`:
```c
uint32_t start_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;
// ... provisioning logic ...
uint32_t elapsed_ms = (xTaskGetTickCount() * portTICK_PERIOD_MS) - start_ms;
ESP_LOGI(TAG, "Provisioning completed in %lu ms", elapsed_ms);
```

2. Add storage size check:
```c
size_t total_size = 0;
for (size_t i = 0; i < catalog_count; i++) {
    total_size += catalog[i].size;
}
ESP_LOGI(TAG, "Total template storage: %zu bytes (%.2f MB)",
         total_size, (float)total_size / (1024*1024));
assert(total_size < (1.5 * 1024 * 1024));  // <1.5MB
```

3. Add deployment timing in `templates_deploy()`:
```c
uint32_t start_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;
// ... deployment logic ...
uint32_t elapsed_ms = (xTaskGetTickCount() * portTICK_PERIOD_MS) - start_ms;
ESP_LOGI(TAG, "Deployment completed in %lu ms", elapsed_ms);
```

**Success Criteria:**
- First boot provisioning: <60s
- Template deployment: <2s
- Playback switch: <100ms
- Total storage: <1.5MB
- Cache hit rate: >80% after boot

---

## 🔧 CRITICAL REFERENCE DOCUMENTATION

### Existing APIs You Will Use

**Storage API (`firmware/components/storage/include/pattern_storage.h`):**
```c
esp_err_t pattern_storage_init(void);                      // Already called
esp_err_t pattern_storage_exists(const char* pattern_id);  // Check if exists
esp_err_t pattern_storage_write(const char* pattern_id, const uint8_t* data, size_t size, bool atomic);
esp_err_t pattern_storage_read(const char* pattern_id, uint8_t** out_data, size_t* out_size);
esp_err_t pattern_storage_list(char*** out_ids, size_t* out_count);
```

**Cache API (`firmware/components/storage/include/pattern_cache.h`):**
```c
esp_err_t pattern_cache_init(size_t capacity_bytes);       // Already called
bool pattern_cache_try_get(const char* pattern_id, const uint8_t** out_ptr, size_t* out_size);
esp_err_t pattern_cache_put_copy(const char* pattern_id, const uint8_t* data, size_t size);
void pattern_cache_stats(uint32_t* hits, uint32_t* misses, size_t* used_bytes, size_t* entry_count);
```

**Parser API (`firmware/components/storage/include/prism_parser.h`):**
```c
esp_err_t parse_prism_header(const uint8_t* data, size_t size, prism_header_v11_t* out_header);
```

### CANON Specifications

From `.taskmaster/CANON.md`:
- LED count: 320 LEDs (ADR-003)
- Pattern max size: 256KB (ADR-004)
- Storage partition: 1.5MB @ `/littlefs` (ADR-005)
- Pattern min count: 15 (ADR-006)
- WebSocket buffer: 4096 bytes (ADR-002)

### Available .prism Files

Located in `/Users/spectrasynq/Workspace_Management/Software/PRISM.k1/out/presets/`:
```
flow-horizon.prism    noise-storm.prism     sine-marquee.prism
flow-lattice.prism    noise-meadow.prism    sine-ripple.prism
flow-orbit.prism      noise-cascade.prism   sine-glacier.prism
flow-trace.prism      noise-rain.prism      sine-midnight.prism
flow-fall.prism       noise-holo.prism      sine-backbeat.prism
flow-lanterns.prism                         sine-mirror.prism
```

Pick best 15 for embedded catalog. Categories:
- **Ambient (5):** flow-* patterns (smooth, slow)
- **Energy (5):** sine-* patterns (rhythmic, dynamic)
- **Special (5):** noise-* patterns (textured, complex)

---

## 🎯 RECOMMENDED WORKFLOW

### Day 1: Subtask 10.1 (Template Descriptors)
```bash
# 1. Create template_patterns.c
cd /Users/spectrasynq/Workspace_Management/Software/PRISM.k1/firmware/components/templates

# 2. Convert 15 .prism files to C arrays
for f in /Users/spectrasynq/Workspace_Management/Software/PRISM.k1/out/presets/*.prism; do
    xxd -i "$f" > "$(basename "$f" .prism)_data.c"
done

# 3. Create template_patterns.c with descriptor table
# 4. Add to CMakeLists.txt
# 5. Build and verify
cd ../../
idf.py build
```

### Day 2: Subtask 10.2 (Provisioning)
```bash
# 1. Implement provisioning logic in templates_init()
# 2. Add storage API calls
# 3. Test idempotency
# 4. Measure timing
```

### Day 3: Subtask 10.3 (Cache Preload)
```bash
# 1. Add cache preload loop
# 2. Implement templates_list()
# 3. Implement templates_deploy()
# 4. Test cache hit rates
```

### Day 4: Subtask 10.4 (Protocol Integration)
```bash
# 1. Add CONTROL command handler
# 2. Update STATUS response
# 3. Test WebSocket deployment
```

### Day 5: Subtask 10.5 (Validation)
```bash
# 1. Add timing instrumentation
# 2. Run full boot test
# 3. Measure deployment latency
# 4. Generate report
```

---

## 🚨 CRITICAL WARNINGS

1. **Memory Budget:** 15 templates × ~100KB avg = 1.5MB. Verify total size <1.5MB.
2. **Cache Size:** 256KB cache can hold ~2-3 templates. LRU will evict as needed.
3. **Build Time:** Embedding 1.5MB of binary data increases build time. Use `-j8` for parallel builds.
4. **Atomic Writes:** Always use `atomic=true` for pattern_storage_write() to prevent corruption.
5. **Heap Usage:** Provisioning allocates ~256KB temp buffers. Monitor heap during first boot.

---

## 🎖️ SUCCESS METRICS

**Before Marking Task 10 Complete:**
- [ ] All 15 templates embedded in `template_patterns.c`
- [ ] First boot provisions templates in <60s
- [ ] Second boot skips provisioning (idempotent)
- [ ] All templates cached at boot (cache_stats shows 15 entries)
- [ ] WebSocket CONTROL triggers deployment
- [ ] Deployment completes in <2s
- [ ] Playback switch <100ms (from logs)
- [ ] Total template storage <1.5MB
- [ ] Firmware builds with zero errors
- [ ] Unit tests pass for templates_list/deploy

---

## 💬 QUESTIONS FOR CAPTAIN

If blocked, check:
1. **CANON.md first** - specifications for storage/cache/protocol
2. **Existing code** - storage/cache/parser APIs already implemented
3. **Out directory** - 17 .prism files ready for embedding

If still blocked after 30 min research:
- Document blocker clearly
- Tag @Captain in task update
- Continue with non-blocked subtasks

---

## 🎯 IMMEDIATE FIRST STEP

```bash
cd /Users/spectrasynq/Workspace_Management/Software/PRISM.k1/firmware/components/templates

# Create template_patterns.c
cat > template_patterns.c << 'EOF'
/**
 * @file template_patterns.c
 * @brief Built-in template pattern catalog (15 presets)
 */

#include "template_manager.h"
#include <stddef.h>

// Template binary data (generated via xxd -i)
// TODO: Embed .prism files as C arrays here

typedef struct {
    const char* id;
    const char* category;
    size_t size;
    const uint8_t* data;
} template_desc_t;

static const template_desc_t builtin_templates[15] = {
    // TODO: Add 15 template descriptors
};

const template_desc_t* template_catalog_get(size_t* out_count) {
    if (out_count) *out_count = 15;
    return builtin_templates;
}
EOF

# Start implementing!
```

---

**MISSION START:** Subtask 10.1 - Define template descriptors with embedded binaries.

**Good hunting, Agent 1!** 🫡

---

**PM:** Captain
**Created:** 2025-10-17
**Previous Agent Context:** Ran out after basic scaffolding
**Estimated Completion:** 5 days (1 day per subtask)
