/**
 * @file pattern_cache.h
 * @brief RAM hot cache for pattern binaries with LRU eviction
 *
 * Provides a 256KB in-RAM cache to accelerate pattern loads and enable
 * <100ms pattern switching. Evicts least-recently-used entries when full.
 */

#ifndef PRISM_PATTERN_CACHE_H
#define PRISM_PATTERN_CACHE_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PATTERN_CACHE_DEFAULT_CAPACITY (256 * 1024) /* 256KB */
#define PATTERN_CACHE_ID_MAX           64

/** Initialize cache with the given capacity (bytes). */
esp_err_t pattern_cache_init(size_t capacity_bytes);

/** Deinitialize cache and free all entries. */
void pattern_cache_deinit(void);

/** Clear all entries but keep cache initialized. */
void pattern_cache_clear(void);

/** Remove a single entry by ID. */
void pattern_cache_invalidate(const char* pattern_id);

/**
 * Try to get cached data pointer by ID.
 * On success, returns true and sets out_ptr/out_size.
 * The returned pointer remains valid until the entry is evicted.
 */
bool pattern_cache_try_get(const char* pattern_id, const uint8_t** out_ptr, size_t* out_size);

/**
 * Insert or replace an entry by copying data into cache memory.
 * If necessary, evicts LRU entries to make room. If item is larger than
 * capacity, the call is a no-op and returns ESP_OK (uncached).
 */
esp_err_t pattern_cache_put_copy(const char* pattern_id, const uint8_t* data, size_t size);

/** Retrieve basic statistics. */
void pattern_cache_stats(uint32_t* out_hits, uint32_t* out_misses, size_t* out_used_bytes, size_t* out_entry_count);

#ifdef __cplusplus
}
#endif

#endif /* PRISM_PATTERN_CACHE_H */

