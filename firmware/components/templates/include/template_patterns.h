/**
 * @file template_patterns.h
 * @brief Built-in template pattern catalog (15 presets)
 */

#ifndef PRISM_TEMPLATE_PATTERNS_H
#define PRISM_TEMPLATE_PATTERNS_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    const char* id;         // e.g. "flow-horizon"
    const char* category;   // "ambient", "energy", "special"
    size_t size;            // bytes
    const uint8_t* data;    // pointer to embedded .prism binary
} template_desc_t;

/**
 * @brief Get pointer to built-in template catalog (15 items)
 * @param out_count Optional; set to number of entries
 * @return Pointer to internal static array valid for program lifetime
 */
const template_desc_t* template_catalog_get(size_t* out_count);

#ifdef __cplusplus
}
#endif

#endif // PRISM_TEMPLATE_PATTERNS_H

