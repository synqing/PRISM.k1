/**
 * @file template_manager.c
 * @brief Template manager implementation stub
 */

#include "template_manager.h"
#include "template_patterns.h"
#include "esp_log.h"
#include "esp_console.h"
#include "pattern_storage.h"
#include "pattern_cache.h"
#include "led_playback.h"
#include <string.h>
#include <stdlib.h>
#include <assert.h>

static const char *TAG = "templates";

// Deprecated stub list; CLI now reflects real catalog
static const char* s_templates[15] = {
    "flow-horizon",
    "flow-lattice",
    "flow-orbit",
    "flow-trace",
    "flow-fall",
    "sine-backbeat",
    "sine-marquee",
    "sine-ripple",
    "sine-glacier",
    "sine-midnight",
    "noise-storm",
    "noise-meadow",
    "noise-cascade",
    "noise-rain",
    "noise-holo",
};

static int cmd_prism_templates(int argc, char **argv)
{
    (void)argc; (void)argv;
    size_t total = sizeof(s_templates)/sizeof(s_templates[0]);
    printf("templates (%zu):\n", total);
    for (size_t i = 0; i < total; ++i) {
        printf("  - %s\n", s_templates[i]);
    }
    return 0;
}

static void register_templates_cli(void)
{
    const esp_console_cmd_t cmd = {
        .command = "prism_templates",
        .help = "List built-in template presets",
        .hint = NULL,
        .func = &cmd_prism_templates,
        .argtable = NULL,
    };
    (void)esp_console_cmd_register(&cmd);

    static struct {
        struct {
            struct arg_str* id;
            void* end;
        } args;
    } s_cli;

    // Simple command without argtable (positional arg)
    extern int cmd_prism_template_deploy(int argc, char** argv);
    const esp_console_cmd_t deploy_cmd = {
        .command = "prism_template_deploy",
        .help = "Deploy a built-in template: prism_template_deploy <id>",
        .hint = NULL,
        .func = &cmd_prism_template_deploy,
        .argtable = NULL,
    };
    (void)esp_console_cmd_register(&deploy_cmd);

    // prism_templates_list [category]
    extern int cmd_prism_templates_list(int argc, char** argv);
    const esp_console_cmd_t list_cmd = {
        .command = "prism_templates_list",
        .help = "List templates; optional category filter: ambient|energy|special",
        .hint = NULL,
        .func = &cmd_prism_templates_list,
        .argtable = NULL,
    };
    (void)esp_console_cmd_register(&list_cmd);

    // prism_template_cache_stats
    extern int cmd_prism_template_cache_stats(int argc, char** argv);
    const esp_console_cmd_t cache_cmd = {
        .command = "prism_template_cache_stats",
        .help = "Show template RAM cache stats",
        .hint = NULL,
        .func = &cmd_prism_template_cache_stats,
        .argtable = NULL,
    };
    (void)esp_console_cmd_register(&cache_cmd);
}

int cmd_prism_template_deploy(int argc, char **argv)
{
    if (argc < 2) {
        printf("Usage: prism_template_deploy <id>\n");
        return 1;
    }
    const char* id = argv[1];
    esp_err_t ret = templates_deploy(id);
    if (ret == ESP_OK) {
        printf("OK: deployed '%s'\n", id);
        return 0;
    }
    printf("ERR: %s\n", esp_err_to_name(ret));
    return (ret == ESP_ERR_NOT_FOUND) ? 2 : 3;
}

int cmd_prism_templates_list(int argc, char **argv)
{
    const char* category = NULL;
    if (argc >= 2) category = argv[1];
    char** ids = NULL; size_t count = 0;
    esp_err_t ret = templates_list(category, &ids, &count);
    if (ret != ESP_OK) {
        printf("ERR: %s\n", esp_err_to_name(ret));
        return 1;
    }
    printf("templates (%zu)%s%s:\n", count,
           category?" category=":"",
           category?category:"");
    for (size_t i = 0; i < count; ++i) {
        printf("  - %s\n", ids[i]);
        free(ids[i]);
    }
    free(ids);
    return 0;
}

int cmd_prism_template_cache_stats(int argc, char **argv)
{
    (void)argc; (void)argv;
    uint32_t hits=0, misses=0; size_t used=0, entries=0;
    pattern_cache_stats(&hits, &misses, &used, &entries);
    printf("cache: entries=%zu used_bytes=%zu hits=%lu misses=%lu\n",
           entries, used, (unsigned long)hits, (unsigned long)misses);
    return 0;
}

esp_err_t templates_init(void) {
    ESP_LOGI(TAG, "Initializing template subsystem...");
    register_templates_cli();

    uint32_t start_ms = (xTaskGetTickCount() * portTICK_PERIOD_MS);

    size_t catalog_count = 0;
    const template_desc_t* catalog = template_catalog_get(&catalog_count);

    // Log total embedded size and validate constraints (<1.5MB)
    size_t total_size = 0;
    for (size_t i = 0; i < catalog_count; ++i) {
        total_size += catalog[i].size;
    }
    ESP_LOGI(TAG, "Embedded templates: %zu items, total %zu bytes (%.2f KB)",
             catalog_count, total_size, (float)total_size / 1024.0f);
    // Enforce 1.5MB storage budget for embedded catalog
    assert(total_size < (1536u * 1024u));

    // Provision to LittleFS (idempotent)
    size_t provisioned = 0;
    for (size_t i = 0; i < catalog_count; ++i) {
        const char* id = catalog[i].id;
        const uint8_t* data = catalog[i].data;
        size_t size = catalog[i].size;

        // Existence check: try read small buffer; NOT_FOUND => missing, otherwise present
        uint8_t probe[1]; size_t out_sz = 0;
        esp_err_t exists = template_storage_read(id, probe, sizeof(probe), &out_sz);
        if (exists == ESP_OK || exists == ESP_ERR_INVALID_SIZE) {
            ESP_LOGD(TAG, "Template '%s' already present, skipping", id);
            continue;
        }

        esp_err_t err = template_storage_write(id, data, size);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Provision failed: '%s' (%s)", id, esp_err_to_name(err));
            continue;
        }
        provisioned++;
        ESP_LOGI(TAG, "Provisioned template '%s' (%zu bytes)", id, size);
    }

    // Preload cache with all templates (best-effort)
    size_t cached = 0;
    for (size_t i = 0; i < catalog_count; ++i) {
        const char* id = catalog[i].id;
        const uint8_t* cptr = NULL; size_t csz = 0;
        if (pattern_cache_try_get(id, &cptr, &csz)) {
            continue; // already cached
        }
        // Read from storage and cache copy
        uint8_t* buf = (uint8_t*)malloc(catalog[i].size);
        if (!buf) {
            ESP_LOGW(TAG, "Cache skip (OOM): %s", id);
            continue;
        }
        size_t read_sz = 0;
        esp_err_t r = template_storage_read(id, buf, catalog[i].size, &read_sz);
        if (r == ESP_OK) {
            (void)pattern_cache_put_copy(id, buf, read_sz);
            cached++;
        }
        free(buf);
    }

    // Cache stats after preload
    uint32_t hits=0, misses=0; size_t used_bytes=0, entry_count=0;
    pattern_cache_stats(&hits, &misses, &used_bytes, &entry_count);

    uint32_t elapsed_ms = (xTaskGetTickCount() * portTICK_PERIOD_MS) - start_ms;
    ESP_LOGI(TAG, "Template provisioning complete: %zu/%zu new, cache entries: %zu, used=%zu bytes, in %lu ms",
             provisioned, catalog_count, entry_count, used_bytes, (unsigned long)elapsed_ms);
    return ESP_OK;
}

void templates_task(void *pvParameters) {
    ESP_LOGI(TAG, "Templates task started on core %d", xPortGetCoreID());

    while (1) {
        // TODO: Template pattern generation (task 10)
        // TODO: Metadata extraction (task 10)
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    ESP_LOGW(TAG, "Templates task exiting (unexpected)");
    vTaskDelete(NULL);
}

esp_err_t templates_deinit(void) {
    ESP_LOGI(TAG, "Deinitializing template subsystem...");
    // TODO: Cleanup implementation
    return ESP_OK;
}

static bool category_matches(const char* want, const char* have)
{
    if (!want || !want[0]) return true; // no filter
    if (!have) return false;
    return strcasecmp(want, have) == 0;
}

esp_err_t templates_list(const char* category, char*** out_ids, size_t* out_count)
{
    if (!out_ids || !out_count) return ESP_ERR_INVALID_ARG;
    *out_ids = NULL; *out_count = 0;

    size_t n = 0, cap = 0;
    char** list = NULL;

    size_t catalog_count = 0;
    const template_desc_t* catalog = template_catalog_get(&catalog_count);

    for (size_t i = 0; i < catalog_count; ++i) {
        if (!category_matches(category, catalog[i].category)) continue;
        if (n == cap) {
            size_t new_cap = cap ? cap * 2 : 8;
            char** tmp = (char**)realloc(list, new_cap * sizeof(char*));
            if (!tmp) { // cleanup allocations so far
                for (size_t k = 0; k < n; ++k) free(list[k]);
                free(list);
                return ESP_ERR_NO_MEM;
            }
            list = tmp; cap = new_cap;
        }
        size_t len = strnlen(catalog[i].id, 63);
        char* s = (char*)malloc(len + 1);
        if (!s) {
            for (size_t k = 0; k < n; ++k) free(list[k]);
            free(list);
            return ESP_ERR_NO_MEM;
        }
        memcpy(s, catalog[i].id, len); s[len] = '\0';
        list[n++] = s;
    }

    *out_ids = list; *out_count = n;
    return ESP_OK;
}

esp_err_t templates_deploy(const char* template_id)
{
    if (!template_id || !template_id[0]) return ESP_ERR_INVALID_ARG;

    uint32_t start_ms = (xTaskGetTickCount() * portTICK_PERIOD_MS);

    const uint8_t* cptr = NULL; size_t csz = 0;
    if (pattern_cache_try_get(template_id, &cptr, &csz)) {
        esp_err_t ret = playback_play_prism_blob(template_id, cptr, csz);
        uint32_t dt = (xTaskGetTickCount() * portTICK_PERIOD_MS) - start_ms;
        ESP_LOGI(TAG, "Deploy(template:%s) cache-hit size=%zu in %lu ms -> %s",
                 template_id, csz, (unsigned long)dt, esp_err_to_name(ret));
        return ret;
    }

    // Read from storage then warm cache and play
    // First, determine file size by attempting read into a generous buffer (size bound by embedded)
    size_t catalog_count = 0; const template_desc_t* catalog = template_catalog_get(&catalog_count);
    size_t max_sz = 0;
    for (size_t i = 0; i < catalog_count; ++i) if (strcmp(catalog[i].id, template_id) == 0) { max_sz = catalog[i].size; break; }
    if (max_sz == 0) {
        // Unknown template id
        return ESP_ERR_NOT_FOUND;
    }
    uint8_t* buf = (uint8_t*)malloc(max_sz);
    if (!buf) return ESP_ERR_NO_MEM;
    size_t read_sz = 0;
    esp_err_t r = template_storage_read(template_id, buf, max_sz, &read_sz);
    if (r != ESP_OK) {
        free(buf);
        return r;
    }
    (void)pattern_cache_put_copy(template_id, buf, read_sz);
    esp_err_t ret = playback_play_prism_blob(template_id, buf, read_sz);
    free(buf);
    uint32_t dt = (xTaskGetTickCount() * portTICK_PERIOD_MS) - start_ms;
    ESP_LOGI(TAG, "Deploy(template:%s) cache-miss size=%zu in %lu ms -> %s",
             template_id, read_sz, (unsigned long)dt, esp_err_to_name(ret));
    return ret;
}
