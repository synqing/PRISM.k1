/**
 * @file led_playback.c
 * @brief LED playback engine (built-in effects, 120 FPS)
 */

#include "led_playback.h"
#include "led_driver.h"
#include "esp_log.h"
#include "prism_wave_tables.h"
#include "esp_attr.h"
#include "esp_cpu.h"
#include <string.h>

// Built-in effect IDs (initial set)
#define EFFECT_WAVE_SINGLE      0x0001
#define EFFECT_PALETTE_CYCLE    0x0040

static const char *TAG = "playback";

typedef struct {
    bool     running;
    uint16_t effect_id;
    uint8_t  params[8];
    uint8_t  param_count;
    uint32_t frame_counter;
} playback_state_t;

static playback_state_t s_pb = {0};
// Precomputed spatial phase per LED (0-255 over LED_COUNT_PER_CH)
static uint8_t s_phase_per_led[LED_COUNT_PER_CH];

#ifdef CONFIG_PRISM_PROFILE_TEMPORAL
typedef struct {
    uint32_t total_cycles;
    uint32_t min_cycles;
    uint32_t max_cycles;
    uint32_t samples;
} wave_prof_accum_t;

static wave_prof_accum_t s_prof_wave = {0};

static inline uint32_t wave_prof_begin(void) {
    return esp_cpu_get_cycle_count();
}

static inline void wave_prof_end(uint32_t start)
{
    uint32_t end = esp_cpu_get_cycle_count();
    uint32_t cycles = end - start;
    if (s_prof_wave.samples == 0 || cycles < s_prof_wave.min_cycles) {
        s_prof_wave.min_cycles = cycles;
    }
    if (cycles > s_prof_wave.max_cycles) {
        s_prof_wave.max_cycles = cycles;
    }
    s_prof_wave.total_cycles += cycles;
    s_prof_wave.samples++;
}
#else
#define wave_prof_begin() (0)
#define wave_prof_end(start) do { (void)(start); } while(0)
#endif

esp_err_t playback_init(void) {
    ESP_LOGI(TAG, "Initializing playback subsystem (120 FPS target)...");
    // Defer LED driver init until first play request
    // Precompute spatial phase offsets for WAVE effects
    for (int i = 0; i < LED_COUNT_PER_CH; i++) {
        s_phase_per_led[i] = (uint8_t)((i * 256) / LED_COUNT_PER_CH);
    }
    ESP_LOGI(TAG, "Playback subsystem ready");
    return ESP_OK;
}

void playback_task(void *pvParameters) {
    ESP_LOGI(TAG, "Playback task started on core %d (HIGHEST priority)", xPortGetCoreID());

    TickType_t last_wake = xTaskGetTickCount();

    // Simple frame buffers for both channels (GRB)
    static uint8_t frame_ch1[LED_FRAME_SIZE_CH];
    static uint8_t frame_ch2[LED_FRAME_SIZE_CH];

    // Main render loop at LED_FPS_TARGET
    while (1) {
        if (s_pb.running) {
            // Minimal built-in effects (fast, integer math)
            switch (s_pb.effect_id) {
                case EFFECT_WAVE_SINGLE: {
                    uint32_t t0 = wave_prof_begin();
                    // LUT-based sinusoidal green wave across strip using sin8_table
                    // Params: [0]=amplitude (0-255), [1]=speed (phase inc per frame)
                    uint8_t amp = (s_pb.param_count >= 1) ? s_pb.params[0] : 255;
                    uint8_t spd = (s_pb.param_count >= 2) ? s_pb.params[1] : 2;
                    uint8_t tphase = (uint8_t)(s_pb.frame_counter * spd);
                    for (int i = 0; i < LED_COUNT_PER_CH; i++) {
                        uint8_t phase = (uint8_t)(s_phase_per_led[i] + tphase);
                        uint8_t s = sin8_table[phase];
                        // Scale sine by amplitude
                        uint8_t val = (uint8_t)((uint16_t)s * amp / 255);
                        // GRB: set G=val, R=0, B=0
                        frame_ch1[i * 3 + 0] = val; // G
                        frame_ch1[i * 3 + 1] = 0;   // R
                        frame_ch1[i * 3 + 2] = 0;   // B
                        frame_ch2[i * 3 + 0] = val; // G
                        frame_ch2[i * 3 + 1] = 0;   // R
                        frame_ch2[i * 3 + 2] = 0;   // B
                    }
                    wave_prof_end(t0);
                    break;
                }
                case EFFECT_PALETTE_CYCLE:
                default: {
                    // Cycle through simple GRB palette
                    uint8_t t = (uint8_t)(s_pb.frame_counter & 0xFF);
                    uint8_t r = t;
                    uint8_t g = 255 - t;
                    uint8_t b = (t >> 1) ^ 0x7F;
                    for (int i = 0; i < LED_COUNT_PER_CH; i++) {
                        // Slight spatial offset per LED to create gradient
                        uint8_t o = (uint8_t)((i * 2 + t) & 0xFF);
                        frame_ch1[i * 3 + 0] = g ^ o;  // G
                        frame_ch1[i * 3 + 1] = r ^ (o >> 1);  // R
                        frame_ch1[i * 3 + 2] = b ^ (o << 1);  // B
                        frame_ch2[i * 3 + 0] = frame_ch1[i * 3 + 0];
                        frame_ch2[i * 3 + 1] = frame_ch1[i * 3 + 1];
                        frame_ch2[i * 3 + 2] = frame_ch1[i * 3 + 2];
                    }
                    break;
                }
            }
            // Submit to LED driver
            (void)led_driver_submit_frames(frame_ch1, frame_ch2);
            s_pb.frame_counter++;
        }

        // 120 FPS frame time
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(LED_FRAME_TIME_MS));
    }

    ESP_LOGW(TAG, "Playback task exiting (unexpected)");
    vTaskDelete(NULL);
}

esp_err_t playback_deinit(void) {
    ESP_LOGI(TAG, "Deinitializing playback subsystem...");
    // No explicit resources; LED driver lifecycle is separate
    return ESP_OK;
}

esp_err_t playback_play_builtin(uint16_t effect_id, const uint8_t* params, uint8_t param_count)
{
    // Ensure LED driver is initialized and running
    esp_err_t ret = led_driver_init();
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "LED driver init failed: %s", esp_err_to_name(ret));
        return ret;
    }
    ret = led_driver_start();
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "LED driver start failed: %s", esp_err_to_name(ret));
        return ret;
    }

    // Set playback state
    s_pb.effect_id = effect_id;
    s_pb.param_count = (param_count > 8) ? 8 : param_count;
    if (params && s_pb.param_count) {
        memcpy(s_pb.params, params, s_pb.param_count);
    }
    s_pb.frame_counter = 0;
    s_pb.running = true;
    ESP_LOGI(TAG, "Playback started: effect=0x%04X params=%u fps=%d", effect_id, s_pb.param_count, LED_FPS_TARGET);
    return ESP_OK;
}

esp_err_t playback_stop(void)
{
    if (!s_pb.running) {
        return ESP_OK;
    }
    s_pb.running = false;
    // Clear LEDs once
    static uint8_t black[LED_FRAME_SIZE_CH] = {0};
    (void)led_driver_submit_frames(black, black);
    ESP_LOGI(TAG, "Playback stopped (driver remains running)");
    return ESP_OK;
}

bool playback_is_running(void)
{
    return s_pb.running;
}
