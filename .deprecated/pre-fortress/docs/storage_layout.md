# Storage Layout Specification

**Version:** 1.0
**Purpose:** Define LittleFS partition structure and .prism format for PRISM K1
**Platform:** ESP32-S3 with 8MB Flash

## Partition Table

```
# ESP32 Partition Table
# Name,     Type, SubType, Offset,  Size,    Flags
nvs,        data, nvs,     0x9000,  0x6000,
phy_init,   data, phy,     0xf000,  0x1000,
factory,    app,  factory, 0x10000, 0x200000,  # 2MB firmware
storage,    data, spiffs,  0x210000,0x180000,  # 1.5MB LittleFS
coredump,   data, coredump,0x390000,0x10000,
```

## LittleFS Structure

### Directory Layout
```
/littlefs/
├── patterns/           # User-created patterns
│   ├── pulse_001.prism
│   ├── wave_002.prism
│   └── custom_003.prism
├── templates/          # Factory templates
│   ├── ambient_01.prism
│   ├── energy_01.prism
│   └── special_01.prism
├── palettes/           # Shared color palettes
│   ├── default.pal
│   ├── warm.pal
│   └── cool.pal
└── config/            # System configuration
    ├── network.cfg
    └── system.cfg
```

### Storage Allocation
- **Total Space:** 1,572,864 bytes (1.5MB)
- **Filesystem Overhead:** ~100KB
- **Usable Space:** ~1.47MB
- **Pattern Capacity:** 25-35 patterns (average 40-60KB each)
- **Template Space:** 300KB reserved (20KB × 15 templates)
- **Palette Space:** 50KB reserved

## .prism Binary Format

### File Structure Overview
```
┌─────────────────────────┐
│   File Header (64B)     │
├─────────────────────────┤
│   Metadata (256B)       │
├─────────────────────────┤
│   Palette Data (VAR)    │
├─────────────────────────┤
│   Timeline Data (VAR)   │
├─────────────────────────┤
│   Parameter Data (VAR)  │
├─────────────────────────┤
│   Preview Data (OPT)    │
└─────────────────────────┘
```

### File Header (64 bytes)
```c
typedef struct {
    // Magic & Version (8 bytes)
    uint32_t magic;           // 0x5052534D ('PRSM')
    uint8_t version_major;    // 1
    uint8_t version_minor;    // 0
    uint8_t compression;      // 0=none, 1=delta, 2=RLE
    uint8_t reserved1;

    // Section Offsets (24 bytes)
    uint32_t metadata_offset;    // Always 64
    uint32_t palette_offset;     // Typically 320
    uint32_t timeline_offset;    // Variable
    uint32_t params_offset;      // Variable
    uint32_t preview_offset;     // 0 if no preview
    uint32_t file_size;         // Total file size

    // Checksums (16 bytes)
    uint32_t header_crc;        // CRC of this header
    uint32_t metadata_crc;      // CRC of metadata section
    uint32_t data_crc;          // CRC of all data sections
    uint32_t reserved2;

    // Timestamps (16 bytes)
    uint32_t created_time;      // Unix timestamp
    uint32_t modified_time;     // Unix timestamp
    uint32_t access_count;      // Usage counter
    uint32_t reserved3;
} prism_header_t;
```

### Metadata Section (256 bytes)
```c
typedef struct {
    // Pattern Info (96 bytes)
    char name[32];              // User-friendly name
    char author[32];            // Creator name
    char description[32];       // Brief description

    // Pattern Properties (32 bytes)
    uint32_t duration_ms;       // Total duration
    uint16_t led_count;         // Number of LEDs
    uint8_t channels;           // 1=mono, 2=stereo
    uint8_t category;           // 0=ambient, 1=energy, 2=special
    float bpm;                  // Tempo if applicable
    float brightness;           // Default brightness (0-1)
    uint16_t palette_id;        // Reference to palette
    uint8_t effect_count;       // Number of effects used
    uint8_t reserved[13];

    // Parameter Defaults (48 bytes)
    struct {
        float value;            // Default value (0-1)
        char name[12];          // Parameter name
    } params[6];

    // Usage Stats (32 bytes)
    uint32_t play_count;        // Times played
    uint32_t total_time_ms;     // Total play time
    uint32_t last_played;       // Unix timestamp
    uint8_t rating;             // User rating (0-5)
    uint8_t flags;              // Bit flags
    uint8_t reserved2[18];

    // Reserved (48 bytes)
    uint8_t reserved3[48];
} prism_metadata_t;
```

### Palette Section (Variable)
```c
typedef struct {
    uint16_t palette_id;        // Unique ID
    uint8_t color_count;        // Number of colors (max 16)
    uint8_t interpolation;      // 0=discrete, 1=linear

    struct {
        uint8_t r, g, b, w;     // RGBW values
    } colors[16];
} prism_palette_t;
```

### Timeline Section (Variable)
```c
typedef struct {
    uint32_t section_size;      // Total timeline size
    uint16_t keyframe_count;    // Number of keyframes
    uint8_t encoding;           // 0=raw, 1=delta, 2=RLE
    uint8_t reserved;

    // Keyframes (variable count)
    struct {
        uint32_t time_ms;       // Timestamp
        uint8_t effect_id;      // Effect type
        uint8_t transition;     // Transition type
        uint16_t duration_ms;   // Effect duration
        float params[6];        // Effect parameters
    } keyframes[];
} prism_timeline_t;
```

### Parameter Curves (Variable)
```c
typedef struct {
    uint32_t section_size;      // Total params size
    uint8_t curve_count;        // Number of curves (max 6)
    uint8_t encoding;           // 0=raw, 1=delta
    uint16_t reserved;

    // Per parameter
    struct {
        uint8_t param_index;    // Which parameter (0-5)
        uint8_t interpolation;  // 0=step, 1=linear, 2=smooth
        uint16_t point_count;   // Number of points

        struct {
            uint32_t time_ms;   // Timestamp
            float value;        // Parameter value (0-1)
        } points[];
    } curves[];
} prism_params_t;
```

## Structural Efficiency Techniques

### 1. Shared Palettes
Instead of storing colors per pattern:
```c
// Before: 64 bytes per pattern
uint32_t colors[16];

// After: 2 bytes reference
uint16_t palette_id;

// Savings: 62 bytes per pattern
```

### 2. Delta Encoding
For slowly changing parameters:
```c
// Before: Raw values
float values[] = {0.50, 0.52, 0.54, 0.56, 0.58};  // 20 bytes

// After: Base + deltas
float base = 0.50;
int8_t deltas[] = {0, 2, 2, 2, 2};  // 9 bytes

// Savings: 55%
```

### 3. Run-Length Encoding
For static sections:
```c
// Before: 1000 identical frames
frame_t frames[1000];  // 16KB

// After: RLE
struct {
    frame_t frame;
    uint16_t count;
} rle = {frame, 1000};  // 18 bytes

// Savings: 99.9%
```

### 4. Hot Cache Management
```c
typedef struct {
    prism_header_t* headers[5];     // Quick access headers
    prism_metadata_t* metadata[5];  // Cached metadata
    uint8_t* data_cache[5];         // Decompressed data
    uint32_t cache_size[5];         // Size of cached data
    uint8_t lru_order[5];           // LRU tracking
    uint8_t cache_count;            // Active entries
} pattern_cache_t;

// Cache hit = <1ms switch
// Cache miss = <100ms switch (load + decompress)
```

## File Operations

### Write Pattern
```c
esp_err_t write_pattern(const char* filename,
                        const prism_header_t* header,
                        const void* data,
                        size_t size) {
    char path[64];
    snprintf(path, sizeof(path), "/littlefs/patterns/%s", filename);

    FILE* f = fopen(path, "wb");
    if (!f) return ESP_ERR_NO_MEM;

    // Write header
    fwrite(header, sizeof(prism_header_t), 1, f);

    // Write data sections
    fwrite(data, size, 1, f);

    // Verify CRC
    fseek(f, 0, SEEK_SET);
    uint32_t crc = calculate_crc(f);

    fclose(f);
    return (crc == header->data_crc) ? ESP_OK : ESP_ERR_INVALID_CRC;
}
```

### Read Pattern
```c
esp_err_t read_pattern_header(const char* filename,
                              prism_header_t* header) {
    char path[64];
    snprintf(path, sizeof(path), "/littlefs/patterns/%s", filename);

    FILE* f = fopen(path, "rb");
    if (!f) return ESP_ERR_NOT_FOUND;

    size_t read = fread(header, sizeof(prism_header_t), 1, f);
    fclose(f);

    if (read != 1) return ESP_ERR_INVALID_SIZE;
    if (header->magic != 0x5052534D) return ESP_ERR_INVALID_VERSION;

    return ESP_OK;
}
```

### List Patterns
```c
int list_patterns(char patterns[][32], int max_patterns) {
    DIR* dir = opendir("/littlefs/patterns");
    if (!dir) return 0;

    struct dirent* entry;
    int count = 0;

    while ((entry = readdir(dir)) && count < max_patterns) {
        if (strstr(entry->d_name, ".prism")) {
            strncpy(patterns[count], entry->d_name, 32);
            count++;
        }
    }

    closedir(dir);
    return count;
}
```

## Space Optimization Results

### Typical Pattern Sizes

| Pattern Type | Raw Size | Optimized | Savings |
|-------------|----------|-----------|---------|
| Simple Pulse | 120KB | 35KB | 71% |
| Complex Wave | 200KB | 85KB | 58% |
| Static Color | 80KB | 12KB | 85% |
| Multi-Effect | 250KB | 110KB | 56% |
| **Average** | **162KB** | **60KB** | **63%** |

### Storage Capacity

| Optimization | Patterns | Improvement |
|--------------|----------|-------------|
| No optimization | 9 patterns | Baseline |
| Shared palettes | 15 patterns | +67% |
| Delta encoding | 20 patterns | +122% |
| RLE for static | 25 patterns | +178% |
| **All combined** | **30-35 patterns** | **+289%** |

## Performance Metrics

### Read Performance
- Header only: <5ms
- Full pattern (<100KB): <50ms
- From cache: <1ms

### Write Performance
- Small pattern (<50KB): <100ms
- Large pattern (<200KB): <400ms
- With verification: +20ms

### Cache Performance
- Hit rate target: >80%
- Switch time (cached): <10ms
- Switch time (uncached): <100ms

## Error Handling

### File Corruption Detection
1. Magic number validation
2. Version compatibility check
3. CRC verification for each section
4. Size sanity checks

### Recovery Procedures
```c
esp_err_t recover_storage() {
    // 1. Scan all files
    // 2. Validate headers
    // 3. Mark corrupted files
    // 4. Rebuild palette index
    // 5. Clear cache
    // 6. Report status
}
```

## Future Expansions

### Version 1.1
- Compression: Add Heatshrink for parameter curves
- Streaming: Direct playback without full load
- Thumbnails: 32×32 preview images

### Version 2.0
- Encryption: AES-128 for commercial patterns
- Signatures: Artist verification
- Metadata: Extended tags and searching