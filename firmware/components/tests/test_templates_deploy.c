// Unity smoke test for templates_deploy()
#include "unity.h"
#include "template_manager.h"

// This smoke test verifies that the call path is wired.
// In a clean test environment without provisioned LittleFS templates,
// we expect ESP_ERR_NOT_FOUND (idempotent, non-crashing behavior).

TEST_CASE("templates_deploy returns NOT_FOUND when template missing", "[templates][deploy]")
{
    esp_err_t ret = templates_deploy("flow-horizon");
    TEST_ASSERT_EQUAL(ESP_ERR_NOT_FOUND, ret);
}

