# CANON Task Corrections Applied
## Task Generation Summary - 2025-10-15

All 56 firmware tasks have been regenerated with CANON specifications applied. This document lists critical corrections from old PRD to CANON-validated values.

## Critical CANON Corrections

### ADR-001: Partition Table
**Old PRD Value:**
- LittleFS: 1MB (0x2F1000)

**CANON Value (ADR-001):**
- LittleFS: **1.5MB (0x180000) at offset 0x311000**

**Tasks Affected:** 002, 003, 004, 030

---

### ADR-002: WebSocket Configuration
**Old PRD Values:**
- Buffer: 8KB (8192 bytes)
- Max Clients: 5

**CANON Values (ADR-002):**
- Buffer: **4KB (4096 bytes)** - 98% allocation success vs 85% for 8KB
- Max Clients: **2** - Prevents heap fragmentation

**Tasks Affected:** 009, 010, 015, 016, 017, 022, 034, 037, 045, 054, 055, 056

---

### ADR-003: LED Count
**Old PRD Value:**
- LED Count: 150

**CANON Value (ADR-003):**
- LED Count: **320**

**Tasks Affected:** 018, 019, 021, 024, 039

---

### ADR-004: Pattern Maximum Size
**Old PRD Value:**
- Max Pattern Size: 200KB (204800 bytes)

**CANON Value (ADR-004):**
- Max Pattern Size: **256KB (262144 bytes)**

**Tasks Affected:** 007, 008, 011, 012, 027, 028, 040, 048, 056

---

###  ADR-005: Storage Mount Path
**Old PRD Value:**
- Mount Path: /prism

**CANON Value (ADR-005):**
- Mount Path: **/littlefs**

**Tasks Affected:** 004, 011, 012, 013, 014, 029, 030, 036, 044

**CRITICAL:** ALL file system paths must use `/littlefs` prefix. This is a breaking change from old documentation.

---

### ADR-006: Pattern Count Revision
**Old PRD Value:**
- Minimum Patterns: 25

**CANON Value (ADR-006):**
- Minimum Patterns: **15** (supersedes ADR-004 pattern_min_count)

**Tasks Affected:** 008, 023, 041, 046, 047, 048, 049, 050, 051, 052

---

## Task File Structure

Each task file includes:

1. **Header Metadata**
   - Task ID (001-056)
   - Title
   - Status (pending/in_progress/done)
   - Dependencies
   - Priority
   - CANON ADRs referenced
   - CANON Validation Date

2. **CANON Specifications Section**
   - Machine-readable YAML
   - Extracted from CANON.md ADRs
   - Specific to this task's requirements

3. **Description**
   - What needs to be done
   - Context and purpose

4. **Details**
   - Implementation guidance
   - CANON-aligned constants
   - Code examples where critical

5. **Test Strategy**
   - Validation against CANON specs
   - Acceptance criteria

6. **Agent Guidance**
   - Step-by-step execution
   - Commands and file paths
   - Verification steps

## Task Priority Distribution

- **High Priority (CRITICAL):** 18 tasks
  - Foundation (001-007, 009-012)
  - Core functionality (015-018, 024)
  - Safety (031, 055, 056)

- **Medium Priority:** 32 tasks
  - Feature development
  - Optimization
  - Testing

- **Low Priority:** 6 tasks
  - Polish and experimental features
  - Advanced capabilities

## Dependency Graph Highlights

### Critical Path (Serial Execution Required)
1. Task 001 (ESP-IDF setup) → blocks everything
2. Task 002 (Partition table) → blocks storage (004)
3. Task 003 (LittleFS component) → blocks storage (004)
4. Task 004 (Filesystem ops) → blocks upload (012), patterns (013/014)
5. Task 015 (WebSocket) → blocks messaging (016, 017)
6. Task 018 (Execution engine) → blocks playback (019, 024)

### Parallel Execution Opportunities
- Network team: 009-017 (HTTPS + WebSocket stack)
- Storage team: 004-008 (Filesystem + format + optimizations)
- Playback team: 018-024 (Engine + timeline + controls)
- Desktop team: 038-053 (Tauri application)
- Testing team: 031-035 (Unit tests + integration + docs)

## Memory Budget Alignment (CANON)

```yaml
# ADR-002: WebSocket Memory
ws_buffer_total: 8192          # 2 clients × 4KB each
ws_overhead: 2048              # Client tracking + state

# ADR-003: LED Buffers
led_buffer_size: 960           # 320 LEDs × 3 bytes
double_buffer: 1920            # 2 × 960 for flicker-free

# Task 054: Memory Pools
pool_4kb: 32768                # 8 × 4KB blocks
pool_1kb: 16384                # 16 × 1KB blocks
pool_256b: 8192                # 32 × 256B blocks
pool_total: 57344              # 56KB reserved

# Total Critical Heap
critical_heap: 69504           # ~68KB for core systems
target_free: 150000            # 150KB minimum free
margin: 80496                  # ~78KB headroom
```

## Validation Checklist

Before agent execution, verify:

- [x] All 56 task files created in `.taskmaster/tasks/`
- [x] Each file has CANON ADR references
- [x] Critical corrections applied (partition 1.5MB, WS 4KB/2 clients, LED 320, path /littlefs, patterns 15)
- [x] Machine-readable YAML specs in each task
- [x] Agent guidance includes step-by-step execution
- [x] Test strategies validate against CANON

## Usage Instructions

### For Human Review
```bash
# Read specific task
cat .taskmaster/tasks/task_002.txt  # Partition table

# Search for CANON references
grep -r "ADR-002" .taskmaster/tasks/

# Validate path corrections
grep -r "/littlefs" .taskmaster/tasks/ | wc -l
grep -r "/prism" .taskmaster/tasks/ | wc -l  # Should be 0
```

### For Agent Execution
```bash
# Parse task file
task_id="001"
task_file=".taskmaster/tasks/task_${task_id}.txt"

# Extract CANON specs
sed -n '/^## CANON Specifications/,/^##/p' "$task_file"

# Execute task following Agent Guidance section
# Validate against Test Strategy section
```

## Change Log

| Date | Change | Reason |
|------|--------|--------|
| 2025-10-15 | Initial generation | Create CANON-aligned task set |
| 2025-10-15 | Partition 1MB→1.5MB | ADR-001 |
| 2025-10-15 | WS buffer 8KB→4KB | ADR-002 heap safety |
| 2025-10-15 | WS clients 5→2 | ADR-002 fragmentation |
| 2025-10-15 | LED count 150→320 | ADR-003 |
| 2025-10-15 | Pattern max 200KB→256KB | ADR-004 |
| 2025-10-15 | Mount /prism→/littlefs | ADR-005 |
| 2025-10-15 | Patterns 25→15 | ADR-006 |

## Next Steps

1. **Captain Review:** Validate all task files against CANON.md
2. **Agent Execution:** Begin with task 001 (ESP-IDF setup)
3. **Validation:** Run `.taskmaster/scripts/validate-canon.sh` after each task
4. **Tracking:** Update task status as work progresses
5. **Conflict Resolution:** If contradictions found, create ADR to resolve

## Files Generated

```
.taskmaster/tasks/
├── task_001.txt  # ESP-IDF v5.x project structure
├── task_002.txt  # Partition table (CANON: 1.5MB littlefs)
├── task_003.txt  # LittleFS component
├── task_004.txt  # Filesystem ops (CANON: /littlefs paths)
├── task_005.txt  # Hash functions (CRC32, SHA256)
...
├── task_012.txt  # Upload handler (CANON: 256KB limit)
├── task_015.txt  # WebSocket endpoint (CANON: 4KB buffer, 2 clients)
├── task_017.txt  # WebSocket client mgmt (CANON: max 2 clients)
├── task_018.txt  # Execution engine (CANON: 320 LEDs)
...
├── task_054.txt  # Memory pool manager (done)
├── task_055.txt  # Heap monitoring (done)
└── task_056.txt  # Bounds checking (done)

Total: 56 task files
```

---

**Status:** ✓ COMPLETE - All 56 tasks generated with CANON specifications
**Validated:** 2025-10-15
**Authority:** CANON.md (6 ADRs)
