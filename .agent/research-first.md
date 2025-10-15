# PRISM.k1 Research-First Methodology

**Purpose:** Prevent costly architectural mistakes through disciplined upfront research.

---

## 🎯 Why Research-First?

### The K1.juce Lesson

**K1.juce (Audio Plugin):** 5-day research phase prevented significant architectural mistakes.

**For PRISM.k1 (Embedded Firmware), this is MORE critical because:**

| Mistake Type | K1.juce Impact | PRISM.k1 Impact |
|--------------|---------------|-----------------|
| **Wrong memory pattern** | Slow, patch later | **Device bricks in field** |
| **Wrong protocol choice** | Bad UX, update | **Upload failures, RMAs** |
| **Wrong compression** | File size issue | **Flash wear, early death** |
| **Wrong timing** | Audio glitches | **Flickering LEDs, unusable** |

**Bottom Line:** In embedded systems, mistakes are measured in customer returns, not GitHub issues.

---

## ⏱️ Research Time Investment

### Compressed Timeline (2-3 Days)

**NOT 5 days like K1.juce.** Firmware research is more focused:

**Day 1: Hardware Constraints & Memory**
- ESP32-S3 memory management patterns
- LittleFS performance characteristics
- RMT peripheral capabilities
- **Deliverable:** Hardware constraints document

**Day 2: Protocol & Communication**
- WebSocket binary protocol design
- Pattern file format optimization
- WiFi reliability patterns
- **Deliverable:** Protocol architecture document

**Day 3: Performance & Reliability**
- Real-time scheduling for 60 FPS
- Power management strategies
- Field reliability patterns
- **Deliverable:** Performance validation document

**Total:** 2-3 focused days → Saves 2-3 weeks of rework

---

## 🔬 Research Sprint Execution

### Day 1: Hardware Constraints & Memory Architecture

#### Research Query 1: ESP32-S3 Memory Management

```bash
task-master research \
  --query="ESP32-S3 memory fragmentation prevention strategies 2024-2025, heap allocation best practices, PSRAM usage patterns for real-time LED control" \
  --detail=high \
  --save-file \
  --save-to=hardware-research
```

**Key Questions to Answer:**
- Maximum safe heap fragmentation threshold?
- PSRAM vs internal RAM for frame buffers?
- Best allocation patterns to prevent fragmentation?
- When to use `heap_caps_malloc()` vs `malloc()`?
- How to monitor fragmentation in production?

**Expected Findings:**
- Use `heap_caps_malloc(size, MALLOC_CAP_DMA)` for LED buffers
- Keep internal RAM for time-critical code
- Warning threshold: <50KB free heap
- Critical threshold: <32KB largest free block
- Use memory pools for fixed-size allocations

#### Research Query 2: LittleFS on ESP32

```bash
task-master research \
  --query="LittleFS on ESP32-S3 performance characteristics, wear leveling, concurrent access patterns, recommended partition sizes, atomic operations" \
  --detail=high \
  --save-file \
  --save-to=hardware-research
```

**Key Questions:**
- Optimal block size for 1.47MB partition?
- How to implement atomic file operations?
- Concurrent read/write safety?
- Expected wear cycles before failure?
- Best practices for power-loss recovery?

**Expected Findings:**
- Block size: 4KB (ESP32 flash page size)
- Atomic pattern: write to .tmp, fsync, rename
- Wear leveling: built-in, ~100K cycles
- Concurrent: single writer + multiple readers safe
- Recovery: journal-based for critical ops

#### Research Query 3: RMT Peripheral for WS2812B

```bash
task-master research \
  --query="ESP32-S3 RMT peripheral WS2812B LED control at 60 FPS, DMA usage, timing accuracy, CPU overhead, double-buffering techniques" \
  --detail=high \
  --save-file \
  --save-to=hardware-research
```

**Key Questions:**
- Can RMT handle 320 LEDs at 60 FPS?
- DMA configuration for zero CPU overhead?
- Timing precision for WS2812B protocol?
- Double-buffering pattern?
- How to handle timing violations?

**Expected Findings:**
- RMT can drive 8 channels simultaneously
- Use RMT with DMA for CPU-free operation
- Timing: 0.4μs +/- 150ns for '0', 0.8μs +/- 150ns for '1'
- Double-buffer in RAM, DMA from PSRAM
- Hardware handles timing, no CPU jitter

#### Deliverable: Hardware Constraints Document

**Save to:** `.taskmaster/docs/research/hardware-constraints.md`

**Template:**
```markdown
# PRISM.k1 Hardware Constraints Research

## Memory Management
- **Heap Budget:** <150KB total usage
- **Fragmentation:** Max 10% acceptable
- **Allocation Pattern:** Use heap_caps_malloc(size, MALLOC_CAP_DMA)
- **Monitoring:** esp_get_free_heap_size() + heap_caps_get_largest_free_block()
- **Warning Threshold:** 50KB free
- **Critical Threshold:** 32KB largest block

## LittleFS Configuration
- **Partition Size:** 1.47MB usable
- **Block Size:** 4KB (ESP32 flash page)
- **Wear Leveling:** ~100K cycles
- **Atomic Operations:** .tmp + fsync + rename pattern
- **Concurrent Access:** Single writer + multiple readers

## RMT Peripheral
- **LED Count:** 320 (fits in single RMT channel)
- **Frame Rate:** 60 FPS achievable
- **DMA:** Enabled for zero CPU overhead
- **Buffer:** Double-buffer in RAM (1.92KB per buffer)
- **Timing:** Hardware-controlled, no CPU jitter

## Design Constraints
1. NO malloc/free in 60 FPS loop
2. NO blocking operations in playback task
3. ALL file writes must be atomic
4. ALL heap allocations must use heap_caps_malloc
5. Monitor fragmentation every 10 seconds
```

---

### Day 2: Protocol Architecture & WebSocket Design

#### Research Query 4: ESP-IDF WebSocket Implementation

```bash
task-master research \
  --query="ESP-IDF 5.x WebSocket server implementation best practices, binary frame handling, large file upload strategies, memory-efficient patterns 2024-2025" \
  --detail=high \
  --save-file \
  --save-to=protocol-research
```

**Key Questions:**
- Recommended receive buffer size?
- How to handle multi-MB uploads?
- Binary frame parsing patterns?
- Memory efficiency for large transfers?
- Error recovery strategies?

**Expected Findings:**
- Buffer size: 8KB (ESP-IDF recommended)
- Chunked upload: httpd_req_recv() in loop
- Stream directly to file, don't buffer in RAM
- Use Range headers for resume capability
- Validate CRC/SHA256 during upload

#### Research Query 5: Binary Protocol Design

```bash
task-master research \
  --query="Binary protocol design for embedded systems 2024, TLV encoding best practices, CRC vs checksum tradeoffs, error recovery patterns" \
  --detail=high \
  --save-file \
  --save-to=protocol-research
```

**Key Questions:**
- TLV vs fixed-size records?
- CRC32 sufficient or need stronger?
- How to version protocol for updates?
- Error detection vs correction?
- Endianness considerations?

**Expected Findings:**
- TLV flexible, fixed-size faster
- CRC32 sufficient for error detection
- Magic bytes + version field for forward compat
- Detection only (correction too expensive)
- Little-endian (ESP32 native)

#### Research Query 6: Pattern Format Optimization

```bash
task-master research \
  --query="LED pattern file format optimization techniques, structural efficiency vs compression, palette sharing, delta encoding for animations" \
  --detail=high \
  --save-file \
  --save-to=protocol-research
```

**Key Questions:**
- Compression vs structural optimization?
- How much can palette sharing save?
- Delta encoding for keyframes?
- Runtime decompression overhead?
- Trade-offs for 60 FPS playback?

**Expected Findings:**
- Structural efficiency FIRST (40-60% savings)
- Compression optional (extra 20-30%)
- Shared palettes: ~25% size reduction
- Delta keyframes: ~15% reduction
- Runtime: structural = fast, compression = slower

#### Deliverable: Protocol Architecture Document

**Save to:** `.taskmaster/docs/research/protocol-architecture.md`

**Template:**
```markdown
# PRISM.k1 Protocol Architecture Research

## WebSocket Configuration
- **Buffer Size:** 8KB receive buffer
- **Upload Strategy:** Chunked transfer via httpd_req_recv()
- **Memory Pattern:** Stream to file, no RAM buffering
- **Resume Support:** Range headers for interrupted uploads
- **Verification:** SHA256 during upload, validate on completion

## Binary Protocol Format
- **Encoding:** TLV for flexibility
- **Header:** Magic bytes 'PRISM\\x00\\x00\\x00' + version (1 byte)
- **Validation:** CRC32 for header + payload
- **Endianness:** Little-endian (ESP32 native)
- **Versioning:** Version field enables protocol evolution

## Pattern File Format (.prism)
### Header (32 bytes)
- Magic: 4 bytes ('PRSM')
- Version: 1 byte
- Duration: 4 bytes (milliseconds)
- LED count: 2 bytes
- Reserved: 5 bytes
- CRC32: 4 bytes (header)
- CRC32: 4 bytes (payload)
- Reserved: 8 bytes

### Body (TLV Records)
- Type 0x01: Palette (shared colors)
- Type 0x02: Timeline (keyframes with delta encoding)
- Type 0x03: Metadata (name, author, etc.)
- Type 0x04: Preview (thumbnail)
- Type 0x05: Custom data

### Optimization Strategy
1. **Structural Efficiency (40-60% savings):**
   - Shared palette across all keyframes
   - Delta encoding for timeline
   - Quantized timing values
   
2. **Optional Compression (20-30% additional):**
   - Heatshrink or gzip (only if >50KB)
   - Trade-off: Size vs playback latency
   - Decision: Skip compression initially

## Design Decisions
- Use TLV for extensibility
- Structural optimization only (no compression)
- 200KB max pattern size
- SHA256 verification for uploads
- Atomic file operations for reliability
```

---

### Day 3: Performance & Reliability Validation

#### Research Query 7: Real-Time Performance

```bash
task-master research \
  --query="ESP32-S3 FreeRTOS task scheduling for 60 FPS LED output, priority inversion prevention, watchdog timer best practices, deterministic timing" \
  --detail=high \
  --save-file \
  --save-to=performance-research
```

**Key Questions:**
- Task priorities for 60 FPS guarantee?
- How to prevent priority inversion?
- Watchdog configuration?
- ISR vs task trade-offs?
- How to measure jitter?

**Expected Findings:**
- Playback task: highest priority (tskIDLE_PRIORITY + 10)
- Network task: medium (+ 5)
- Use hardware timer for 60 FPS (not vTaskDelay)
- Disable watchdog on playback task
- Measure jitter with esp_timer_get_time()

#### Research Query 8: Power Management

```bash
task-master research \
  --query="ESP32-S3 WiFi power management, deep sleep for LED controller, brownout detection, USB power negotiation" \
  --detail=high \
  --save-file \
  --save-to=performance-research
```

**Key Questions:**
- WiFi power save modes?
- Can we use light sleep between frames?
- Brownout threshold settings?
- USB power limits (500mA vs 2A)?
- Recovery from power issues?

**Expected Findings:**
- WiFi: WIFI_PS_MIN_MODEM for latency
- Light sleep: not during playback
- Brownout: 2.7V threshold
- USB: Negotiate 2A if available
- Recovery: graceful shutdown on brownout

#### Research Query 9: Field Reliability

```bash
task-master research \
  --query="ESP32 firmware OTA update reliability, factory partition strategy, corruption recovery, field diagnostic logging" \
  --detail=high \
  --save-file \
  --save-to=performance-research
```

**Key Questions:**
- OTA update safety patterns?
- Factory reset mechanism?
- How to detect corruption?
- Remote diagnostics approach?
- Crash dump collection?

**Expected Findings:**
- Dual app partitions with rollback
- Factory partition for recovery
- CRC validation on boot
- Syslog over WiFi for diagnostics
- Coredump partition for crash analysis

#### Deliverable: Performance & Reliability Document

**Save to:** `.taskmaster/docs/research/performance-reliability.md`

**Template:**
```markdown
# PRISM.k1 Performance & Reliability Research

## Real-Time Performance
### Task Priorities
- Playback: tskIDLE_PRIORITY + 10 (highest)
- Network: tskIDLE_PRIORITY + 5
- Storage: tskIDLE_PRIORITY + 4

### 60 FPS Guarantee
- Use hardware timer (esp_timer) not vTaskDelay
- Timer callback triggers frame generation
- Total budget: 16.67ms per frame
- Frame generation: <10ms target
- RMT transfer: ~2ms (DMA, parallel to CPU)

### Jitter Measurement
```c
uint64_t start = esp_timer_get_time();
// ... render frame ...
uint64_t end = esp_timer_get_time();
uint64_t jitter = end - start - 16667; // μs
if (jitter > 1000) ESP_LOGW(TAG, "Frame jitter: %llu μs", jitter);
```

## Power Management
- **WiFi Mode:** WIFI_PS_MIN_MODEM (low latency)
- **Sleep:** Disabled during playback
- **Brownout:** 2.7V threshold
- **USB Power:** Negotiate 2A (LED power)

## Reliability Mechanisms
### OTA Updates
- Dual app partitions (app0 + app1)
- Rollback on boot failure
- Factory partition for recovery

### Corruption Detection
- CRC32 validation on boot
- Pattern file integrity checks
- Flash verify after writes

### Field Diagnostics
- Syslog over WiFi (configurable)
- Crash dumps in dedicated partition
- Heap monitoring with alerts

### Recovery Procedures
1. Corrupted pattern: Delete and re-upload
2. Flash corruption: Factory reset
3. OTA failure: Rollback to previous
4. Brownout: Graceful LED shutdown

## Performance Targets
- Frame Rate: 60 FPS (±0.1%)
- Pattern Switch: <100ms
- Upload Speed: >500KB/s
- Memory: <150KB heap
- CPU: <70% during playback
```

---

## 📚 Post-Research: Task Enhancement

### Update Tasks with Research Context

**After completing research sprint, enhance ALL tasks:**

```bash
# Update storage-related tasks
task-master update-task --id=4 --append \
  --prompt="Research Findings Applied:

Memory Strategy:
- Use heap_caps_malloc(size, MALLOC_CAP_DMA) for buffers
- Monitor fragmentation: heap_caps_get_largest_free_block()
- Alert if <50KB free or <32KB largest block

LittleFS Config:
- Block size: 4KB (ESP32 native)
- Atomic pattern: .tmp + fsync + rename
- Concurrent: single writer safe

See: .taskmaster/docs/research/hardware-constraints.md"

# Update network tasks
task-master update-task --id=10 --append \
  --prompt="Research Findings Applied:

WebSocket Config:
- Buffer size: 8KB (ESP-IDF recommended)
- Chunked upload via httpd_req_recv()
- Stream to file, no RAM buffering

Protocol:
- Binary TLV encoding
- CRC32 validation
- Little-endian

See: .taskmaster/docs/research/protocol-architecture.md"

# Update playback tasks
task-master update-task --id=18 --append \
  --prompt="Research Findings Applied:

Real-Time Config:
- Task priority: tskIDLE_PRIORITY + 10
- Hardware timer for 60 FPS
- RMT with DMA for LED output

Performance Targets:
- Frame budget: 16.67ms
- Generation: <10ms
- Jitter: <1ms

See: .taskmaster/docs/research/performance-reliability.md"
```

---

## 🎯 Per-Task Research Protocol

### Before Implementing ANY Task

**Even after initial research sprint, ALWAYS research before coding:**

```bash
# 1. Get task context
task-master show --tag=<tag> --id=<task-id>

# 2. Research specific implementation
task-master research \
  --task-ids="<task-id>" \
  --tag="<tag>" \
  --query="ESP-IDF 5.x <specific-topic> best practices implementation patterns 2024-2025" \
  --save-to="<task-id>" \
  --detail=medium

# 3. Use Context7 for ESP-IDF docs
# MCP tool: get-library-docs /espressif/esp-idf/v5.3

# 4. Document findings
task-master update-subtask --id="<task-id>" \
  --prompt="Per-Task Research Complete

Key Patterns:
- <pattern-1>
- <pattern-2>

Constraints:
- <constraint-1>
- <constraint-2>

Implementation Approach:
<brief-plan>

Ready to implement."
```

**Time Budget:** 10-30 minutes per task

---

## 🚨 Consequences of Skipping Research

### Without Research Sprint

**Typical Timeline:**
- Day 1-3: Fast progress (implementing blind)
- Day 4-10: Slowdown (hitting issues)
- Day 11-20: Rework (fixing architecture)
- **Total: 20 days**

**Problems Encountered:**
- WebSocket buffer too small → Upload failures
- Memory allocation causes fragmentation → Crashes after 24h
- Wrong file format → Rewrite storage layer
- RMT timing incorrect → LED flickering
- **Result:** 30-40% of time spent on rework

### With Research Sprint

**Timeline:**
- Day 1-3: Research (investment)
- Day 4-17: Steady progress (validated approach)
- Day 18-20: Polish (minor fixes)
- **Total: 20 days** (but cleaner, more reliable)

**Advantages:**
- WebSocket buffer sized correctly from Day 1
- Memory pattern prevents fragmentation
- File format supports all requirements
- RMT timing perfect first try
- **Result:** <10% rework, production-ready code

---

## ✅ Research Quality Gates

### Per-Domain Research Checklist

**Hardware Research Complete When:**
- ✅ Memory budget defined and validated
- ✅ Fragmentation prevention pattern documented
- ✅ LittleFS configuration determined
- ✅ RMT peripheral capabilities confirmed
- ✅ All constraints documented with rationale

**Protocol Research Complete When:**
- ✅ WebSocket buffer size determined
- ✅ Binary protocol format designed
- ✅ Pattern file format specified
- ✅ Optimization strategy chosen (structural vs compression)
- ✅ All design decisions documented with trade-offs

**Performance Research Complete When:**
- ✅ Real-time scheduling strategy defined
- ✅ 60 FPS achievability confirmed
- ✅ Power management approach decided
- ✅ Reliability mechanisms specified
- ✅ All targets validated as feasible

---

## 🎓 Research Best Practices

### DO:
- ✅ Use Context7 for ESP-IDF/FreeRTOS docs (always current)
- ✅ Use Brave Search for recent (2024-2025) best practices
- ✅ Document ALL findings in research documents
- ✅ Update tasks with research context
- ✅ Validate findings with calculations/prototypes
- ✅ Question assumptions, verify constraints

### DON'T:
- ❌ Skip research because "I know ESP32"
- ❌ Rely only on training data (might be outdated)
- ❌ Research during implementation (too late)
- ❌ Assume patterns from other projects apply
- ❌ Skip documentation (findings forgotten)
- ❌ Research without applying (waste of time)

---

**Remember:** 3 days of research prevents 3 weeks of rework. This is not overhead—this is survival. 🎯

---

**Next:** See [mcp-usage.md](./mcp-usage.md) for tool reference.

**Version:** 1.0  
**Last Updated:** 2025-10-15
