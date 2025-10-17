/**
 * @file test_pattern_cache.c
 * @brief Unity tests for RAM hot cache (Task 7)
 */

#include "unity.h"
#include "pattern_cache.h"
#include <string.h>

TEST_CASE("pattern cache basic put/get and eviction", "[cache][storage]") {
    // Use small capacity to force eviction logic
    TEST_ASSERT_EQUAL(ESP_OK, pattern_cache_init(1024));

    uint8_t a[400]; memset(a, 0xAA, sizeof(a));
    uint8_t b[400]; memset(b, 0xBB, sizeof(b));
    uint8_t c[400]; memset(c, 0xCC, sizeof(c));

    TEST_ASSERT_EQUAL(ESP_OK, pattern_cache_put_copy("a", a, sizeof(a)));
    TEST_ASSERT_EQUAL(ESP_OK, pattern_cache_put_copy("b", b, sizeof(b)));

    // Both a and b should be present (used=800)
    const uint8_t* ptr = NULL; size_t sz = 0;
    TEST_ASSERT_TRUE(pattern_cache_try_get("a", &ptr, &sz));
    TEST_ASSERT_EQUAL_UINT32(sizeof(a), sz);
    TEST_ASSERT_EQUAL_HEX8(0xAA, ptr[0]);

    TEST_ASSERT_TRUE(pattern_cache_try_get("b", &ptr, &sz));
    TEST_ASSERT_EQUAL_UINT32(sizeof(b), sz);
    TEST_ASSERT_EQUAL_HEX8(0xBB, ptr[0]);

    // Insert c (400). Capacity=1024. Need 400, free=224 -> evict LRU until enough room.
    TEST_ASSERT_EQUAL(ESP_OK, pattern_cache_put_copy("c", c, sizeof(c)));

    // After eviction, only most recent entries should remain (depending on LRU order)
    // Access pattern b before inserting c moved b to MRU, so a should be evicted first.
    TEST_ASSERT_FALSE(pattern_cache_try_get("a", &ptr, &sz));
    TEST_ASSERT_TRUE(pattern_cache_try_get("b", &ptr, &sz));
    TEST_ASSERT_TRUE(pattern_cache_try_get("c", &ptr, &sz));

    pattern_cache_deinit();
}

TEST_CASE("pattern cache stats track hits/misses", "[cache][storage]") {
    TEST_ASSERT_EQUAL(ESP_OK, pattern_cache_init(512));
    uint8_t d[200]; memset(d, 0xDD, sizeof(d));
    TEST_ASSERT_EQUAL(ESP_OK, pattern_cache_put_copy("d", d, sizeof(d)));

    const uint8_t* ptr = NULL; size_t sz = 0;
    (void)pattern_cache_try_get("x", &ptr, &sz); // miss
    (void)pattern_cache_try_get("d", &ptr, &sz); // hit

    uint32_t h=0,m=0; size_t used=0, cnt=0;
    pattern_cache_stats(&h, &m, &used, &cnt);
    TEST_ASSERT_TRUE(h >= 1);
    TEST_ASSERT_TRUE(m >= 1);
    TEST_ASSERT_TRUE(used > 0);
    TEST_ASSERT_TRUE(cnt >= 1);

    pattern_cache_deinit();
}

