// Unity tests for templates_list() filtering and counts
#include "unity.h"
#include "template_manager.h"
#include "template_patterns.h"
#include <stdlib.h>

static void free_string_list(char** list, size_t count)
{
    if (!list) return;
    for (size_t i = 0; i < count; ++i) free(list[i]);
    free(list);
}

TEST_CASE("templates_list returns all 15 when category=NULL", "[templates]")
{
    char** ids = NULL; size_t count = 0;
    TEST_ASSERT_EQUAL(ESP_OK, templates_list(NULL, &ids, &count));
    TEST_ASSERT_NOT_NULL(ids);
    TEST_ASSERT_EQUAL_UINT32(15, count);
    free_string_list(ids, count);
}

TEST_CASE("templates_list filters by category (5 each)", "[templates]")
{
    const char* cats[] = {"ambient", "energy", "special"};
    for (int i = 0; i < 3; ++i) {
        char** ids = NULL; size_t count = 0;
        TEST_ASSERT_EQUAL(ESP_OK, templates_list(cats[i], &ids, &count));
        TEST_ASSERT_NOT_NULL(ids);
        TEST_ASSERT_EQUAL_UINT32(5, count);
        free_string_list(ids, count);
    }
}

