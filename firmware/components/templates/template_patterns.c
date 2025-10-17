/**
 * @file template_patterns.c
 * @brief Built-in template pattern catalog (15 presets)
 */

#include "template_patterns.h"
#include <stdint.h>

// Embedded binary arrays (generated via xxd -i). Declared here as extern;
// definitions live in data/*.c and are linked into this component.

// Ambient (flow-*)
extern const unsigned char flow_horizon_data[];  extern const unsigned int flow_horizon_data_len;
extern const unsigned char flow_lattice_data[];  extern const unsigned int flow_lattice_data_len;
extern const unsigned char flow_orbit_data[];    extern const unsigned int flow_orbit_data_len;
extern const unsigned char flow_trace_data[];    extern const unsigned int flow_trace_data_len;
extern const unsigned char flow_fall_data[];     extern const unsigned int flow_fall_data_len;

// Energy (sine-*)
extern const unsigned char sine_backbeat_data[]; extern const unsigned int sine_backbeat_data_len;
extern const unsigned char sine_marquee_data[];  extern const unsigned int sine_marquee_data_len;
extern const unsigned char sine_ripple_data[];   extern const unsigned int sine_ripple_data_len;
extern const unsigned char sine_glacier_data[];  extern const unsigned int sine_glacier_data_len;
extern const unsigned char sine_midnight_data[]; extern const unsigned int sine_midnight_data_len;

// Special (noise-*)
extern const unsigned char noise_storm_data[];   extern const unsigned int noise_storm_data_len;
extern const unsigned char noise_meadow_data[];  extern const unsigned int noise_meadow_data_len;
extern const unsigned char noise_cascade_data[]; extern const unsigned int noise_cascade_data_len;
extern const unsigned char noise_rain_data[];    extern const unsigned int noise_rain_data_len;
extern const unsigned char noise_holo_data[];    extern const unsigned int noise_holo_data_len;

static template_desc_t s_builtin_templates[15];
static bool s_catalog_built = false;

const template_desc_t* template_catalog_get(size_t* out_count)
{
    if (!s_catalog_built) {
        // Ambient (5)
        s_builtin_templates[0] = (template_desc_t){ "flow-horizon",  "ambient", (size_t)flow_horizon_data_len,  (const uint8_t*)flow_horizon_data };
        s_builtin_templates[1] = (template_desc_t){ "flow-lattice",  "ambient", (size_t)flow_lattice_data_len,  (const uint8_t*)flow_lattice_data };
        s_builtin_templates[2] = (template_desc_t){ "flow-orbit",    "ambient", (size_t)flow_orbit_data_len,    (const uint8_t*)flow_orbit_data };
        s_builtin_templates[3] = (template_desc_t){ "flow-trace",    "ambient", (size_t)flow_trace_data_len,    (const uint8_t*)flow_trace_data };
        s_builtin_templates[4] = (template_desc_t){ "flow-fall",     "ambient", (size_t)flow_fall_data_len,     (const uint8_t*)flow_fall_data };

        // Energy (5)
        s_builtin_templates[5] = (template_desc_t){ "sine-backbeat", "energy",  (size_t)sine_backbeat_data_len, (const uint8_t*)sine_backbeat_data };
        s_builtin_templates[6] = (template_desc_t){ "sine-marquee",  "energy",  (size_t)sine_marquee_data_len,  (const uint8_t*)sine_marquee_data };
        s_builtin_templates[7] = (template_desc_t){ "sine-ripple",   "energy",  (size_t)sine_ripple_data_len,   (const uint8_t*)sine_ripple_data };
        s_builtin_templates[8] = (template_desc_t){ "sine-glacier",  "energy",  (size_t)sine_glacier_data_len,  (const uint8_t*)sine_glacier_data };
        s_builtin_templates[9] = (template_desc_t){ "sine-midnight", "energy",  (size_t)sine_midnight_data_len, (const uint8_t*)sine_midnight_data };

        // Special (5)
        s_builtin_templates[10] = (template_desc_t){ "noise-storm",   "special", (size_t)noise_storm_data_len,   (const uint8_t*)noise_storm_data };
        s_builtin_templates[11] = (template_desc_t){ "noise-meadow",  "special", (size_t)noise_meadow_data_len,  (const uint8_t*)noise_meadow_data };
        s_builtin_templates[12] = (template_desc_t){ "noise-cascade", "special", (size_t)noise_cascade_data_len, (const uint8_t*)noise_cascade_data };
        s_builtin_templates[13] = (template_desc_t){ "noise-rain",    "special", (size_t)noise_rain_data_len,    (const uint8_t*)noise_rain_data };
        s_builtin_templates[14] = (template_desc_t){ "noise-holo",    "special", (size_t)noise_holo_data_len,    (const uint8_t*)noise_holo_data };

        s_catalog_built = true;
    }

    if (out_count) *out_count = 15;
    return (const template_desc_t*)s_builtin_templates;
}
