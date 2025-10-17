/**
 * @file pattern_cache.c
 * @brief RAM hot cache for pattern binaries with LRU eviction
 */

#include "pattern_cache.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <string.h>
#include <stdlib.h>

typedef struct cache_entry {
    char id[PATTERN_CACHE_ID_MAX];
    uint8_t* data;
    size_t size;
    struct cache_entry* prev;
    struct cache_entry* next;
} cache_entry_t;

static const char* TAG = "pattern_cache";

static SemaphoreHandle_t s_mutex = NULL;
static bool s_inited = false;
static size_t s_capacity = PATTERN_CACHE_DEFAULT_CAPACITY;
static size_t s_used = 0;
static cache_entry_t* s_head = NULL; // MRU
static cache_entry_t* s_tail = NULL; // LRU
static uint32_t s_hits = 0;
static uint32_t s_misses = 0;
static size_t s_count = 0;

static void lock(void) { if (s_mutex) xSemaphoreTake(s_mutex, portMAX_DELAY); }
static void unlock(void) { if (s_mutex) xSemaphoreGive(s_mutex); }

static void list_remove(cache_entry_t* e) {
    if (!e) return;
    if (e->prev) e->prev->next = e->next; else s_head = e->next;
    if (e->next) e->next->prev = e->prev; else s_tail = e->prev;
    e->prev = e->next = NULL;
}

static void list_push_front(cache_entry_t* e) {
    e->prev = NULL;
    e->next = s_head;
    if (s_head) s_head->prev = e;
    s_head = e;
    if (!s_tail) s_tail = e;
}

static cache_entry_t* find_entry(const char* id) {
    for (cache_entry_t* it = s_head; it; it = it->next) {
        if (strncmp(it->id, id, PATTERN_CACHE_ID_MAX) == 0) {
            return it;
        }
    }
    return NULL;
}

static void evict_until_free(size_t needed) {
    while (s_tail && (s_capacity - s_used) < needed) {
        cache_entry_t* victim = s_tail;
        list_remove(victim);
        if (victim->data) free(victim->data);
        s_used -= victim->size;
        s_count--;
        free(victim);
    }
}

esp_err_t pattern_cache_init(size_t capacity_bytes) {
    if (s_inited) {
        ESP_LOGW(TAG, "already initialized");
        return ESP_OK;
    }
    s_mutex = xSemaphoreCreateMutex();
    if (!s_mutex) return ESP_ERR_NO_MEM;
    s_capacity = capacity_bytes > 0 ? capacity_bytes : PATTERN_CACHE_DEFAULT_CAPACITY;
    s_used = 0; s_head = s_tail = NULL; s_hits = s_misses = 0; s_count = 0;
    s_inited = true;
    ESP_LOGI(TAG, "initialized (capacity=%u KB)", (unsigned)(s_capacity/1024));
    return ESP_OK;
}

void pattern_cache_deinit(void) {
    if (!s_inited) return;
    lock();
    while (s_head) {
        cache_entry_t* next = s_head->next;
        if (s_head->data) free(s_head->data);
        free(s_head);
        s_head = next;
    }
    s_tail = NULL;
    s_used = 0;
    s_count = 0;
    unlock();
    vSemaphoreDelete(s_mutex);
    s_mutex = NULL;
    s_inited = false;
}

void pattern_cache_clear(void) {
    if (!s_inited) return;
    lock();
    while (s_head) {
        cache_entry_t* next = s_head->next;
        if (s_head->data) free(s_head->data);
        free(s_head);
        s_head = next;
    }
    s_tail = NULL;
    s_used = 0;
    s_count = 0;
    unlock();
}

void pattern_cache_invalidate(const char* pattern_id) {
    if (!s_inited || !pattern_id) return;
    lock();
    cache_entry_t* e = find_entry(pattern_id);
    if (e) {
        list_remove(e);
        if (e->data) free(e->data);
        s_used -= e->size;
        s_count--;
        free(e);
    }
    unlock();
}

bool pattern_cache_try_get(const char* pattern_id, const uint8_t** out_ptr, size_t* out_size) {
    if (!s_inited || !pattern_id) return false;
    bool found = false;
    lock();
    cache_entry_t* e = find_entry(pattern_id);
    if (e) {
        // Move to MRU
        list_remove(e);
        list_push_front(e);
        if (out_ptr) *out_ptr = e->data;
        if (out_size) *out_size = e->size;
        s_hits++;
        found = true;
    } else {
        s_misses++;
    }
    unlock();
    return found;
}

esp_err_t pattern_cache_put_copy(const char* pattern_id, const uint8_t* data, size_t size) {
    if (!s_inited || !pattern_id || !data || size == 0) return ESP_ERR_INVALID_ARG;
    if (size > s_capacity) {
        // Too large to cache; treat as no-op
        ESP_LOGD(TAG, "skip caching '%s' (%zu > capacity %u)", pattern_id, size, (unsigned)s_capacity);
        return ESP_OK;
    }

    lock();

    // Replace if exists
    cache_entry_t* existing = find_entry(pattern_id);
    if (existing) {
        list_remove(existing);
        if (existing->data) free(existing->data);
        s_used -= existing->size;
        s_count--;
        free(existing);
    }

    evict_until_free(size);

    cache_entry_t* e = (cache_entry_t*)calloc(1, sizeof(cache_entry_t));
    if (!e) { unlock(); return ESP_ERR_NO_MEM; }
    strlcpy(e->id, pattern_id, sizeof(e->id));
    e->data = (uint8_t*)malloc(size);
    if (!e->data) { free(e); unlock(); return ESP_ERR_NO_MEM; }
    memcpy(e->data, data, size);
    e->size = size;
    list_push_front(e);
    s_used += size;
    s_count++;
    unlock();
    return ESP_OK;
}

void pattern_cache_stats(uint32_t* out_hits, uint32_t* out_misses, size_t* out_used_bytes, size_t* out_entry_count) {
    if (!s_inited) {
        if (out_hits) *out_hits = 0;
        if (out_misses) *out_misses = 0;
        if (out_used_bytes) *out_used_bytes = 0;
        if (out_entry_count) *out_entry_count = 0;
        return;
    }
    lock();
    if (out_hits) *out_hits = s_hits;
    if (out_misses) *out_misses = s_misses;
    if (out_used_bytes) *out_used_bytes = s_used;
    if (out_entry_count) *out_entry_count = s_count;
    unlock();
}

