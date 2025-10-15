# Multi-Agent Task Assignments - PRISM.K1

**Date:** 2025-10-16
**Project Manager:** Primary Claude Code Agent
**Status:** Task 1 Complete - Deploying parallel execution

## Agent Team Roster

- **Agent 1 (PM)**: Project Manager/Orchestrator (this agent)
- **Agent 2**: Claude Code Instance - Full ESP-IDF development
- **Agent 3**: Claude Code Instance - Full ESP-IDF development
- **Agent 4**: Codex Agent - Capabilities TBD

## Task Dependency Analysis

### Critical Path (Sequential - BLOCKS DOWNSTREAM)
```
Task 1 (DONE) → Task 2 (WiFi) → Task 3 (WebSocket) → Task 4 (TLV Protocol)
```

### Independent Tracks (Can Run in Parallel)
```
Track A: Task 5 (LittleFS) → Task 6 (Parser) → Task 7 (Cache) → Task 10 (Templates)
Track B: Task 8 (RMT LED) → Task 9 (Effects) → [Joins Task 10]
```

### Dependencies Summary
- **Task 2** (WiFi): Depends on Task 1 ✅ READY
- **Task 3** (WebSocket): Depends on Tasks 1, 2
- **Task 4** (TLV Protocol): Depends on Task 3
- **Task 5** (LittleFS): Depends on Task 1 ✅ READY (INDEPENDENT)
- **Task 6** (Pattern Parser): Depends on Task 5
- **Task 7** (Hot Cache): Depends on Tasks 5, 6
- **Task 8** (RMT LED): Depends on Task 1 ✅ READY (INDEPENDENT)
- **Task 9** (Effects): Depends on Tasks 6, 7, 8
- **Task 10** (Templates): Depends on Tasks 5, 6, 7, 9

## Initial Parallel Deployment Strategy

### Phase 1: Three Independent Tracks (START NOW)

**Agent 2 Assignment: Task 2 (WiFi Lifecycle)**
- **Priority:** CRITICAL PATH - blocks Tasks 3 & 4
- **Complexity:** 8/10
- **Subtasks:** 6 subtasks
- **Why Agent 2:** Network infrastructure is critical path; experienced Claude Code agent
- **Deliverable:** WiFi dual-mode, captive portal, mDNS, reconnection logic
- **CANON References:**
  - Network component scaffolded in Task 1
  - Memory pool allocations required (`prism_pool_alloc`)
  - Must use `components/network/network_manager.c`

**Agent 3 Assignment: Task 5 (LittleFS Storage)**
- **Priority:** HIGH - enables Tasks 6, 7, 10
- **Complexity:** 7/10
- **Subtasks:** 5 subtasks
- **Why Agent 3:** Storage track is independent; unblocks 3 downstream tasks
- **Deliverable:** LittleFS mount, pattern CRUD APIs, quota enforcement, CRC validation
- **CANON References:**
  - ADR-001: Partition at 0x320000, 1.5MB
  - ADR-004: 256KB pattern max size
  - ADR-005: Mount path `/littlefs`
  - Component scaffolded in Task 1

**Agent 4 Assignment: Task 8 (RMT LED Driver)**
- **Priority:** HIGH - enables Task 9 (Effects)
- **Complexity:** 8/10
- **Subtasks:** 5 subtasks
- **Why Agent 4:** Hardware-specific; independent track
- **Deliverable:** RMT TX channel, WS2812 encoder, double-buffering, 60 FPS timing
- **CANON References:**
  - ADR-003: 320 LEDs
  - Component scaffolded in Task 1
  - Must use DMA-capable memory (`heap_caps_malloc(MALLOC_CAP_DMA)`)
  - Target: 60 FPS (16.67ms frame time)

---

## Agent Briefs

### AGENT 2: Task 2 - WiFi Lifecycle & Networking

**Objective:** Implement complete WiFi subsystem enabling AP mode captive portal, STA mode connectivity, credential persistence, and mDNS discovery.

**Start Point:**
- Component: `firmware/components/network/`
- Files: `network_manager.c`, `network_manager.h` (already scaffolded)
- Current state: Stub functions return ESP_OK

**Requirements from CANON.md:**
- Use `prism_pool_alloc` for dynamic allocations (avoid fragmentation)
- Integrate with `prism_heap_monitor` for telemetry
- Error propagation via `error_handler` (once implemented)

**Key Implementation Points:**
1. **Dual-mode WiFi**: Configure both AP (`PRISM-SETUP`) and STA modes using `esp_netif`
2. **Captive Portal**: Host HTTP server for credential submission using `esp_http_server`
3. **NVS Persistence**: Store/retrieve credentials using `nvs_open`, `nvs_set_str`
4. **Reconnection Logic**: Exponential backoff with `WIFI_RETRY_MAX`, event handlers for `WIFI_EVENT` and `IP_EVENT`
5. **mDNS Service**: Register `prism-k1.local` with `_prism._tcp` service using `mdns_init()`

**Subtasks (6 total):**
1. Establish WiFi dual-mode initialization
2. Build captive portal HTTP workflow
3. Implement credential persistence with NVS
4. Add reconnect backoff and event handlers
5. Configure mDNS advertisement service
6. Define unit and integration validation coverage

**Test Strategy:**
- Unity tests with mocked `esp_event_loop_run()` for backoff timing
- Hardware verification of captive portal flow
- mDNS query verification (`mdns_query_ptr()`)
- Router power-cycle reconnect testing

**Build Command:**
```bash
cd firmware
idf.py build
idf.py flash monitor  # When ready for hardware testing
```

**Task Management:**
```bash
task-master show 2           # View full task details
task-master show 2.1         # View subtask 1 details
task-master set-status --id=2 --status=in-progress
task-master set-status --id=2.1 --status=in-progress  # As you start each subtask
task-master update-subtask --id=2.1 --prompt="implementation notes..."  # Log progress
task-master set-status --id=2.1 --status=done  # Complete subtask
```

**Coordination:**
- Check in with PM (Agent 1) after completing each subtask
- Task 3 (WebSocket) is blocked on your completion
- Task 4 (TLV Protocol) is transitively blocked

**Success Criteria:**
- WiFi connects in both AP and STA modes
- Captive portal serves credential form and transitions to STA
- Credentials persist across reboots via NVS
- Reconnection logic handles dropouts with exponential backoff
- mDNS advertises `prism-k1.local`
- All 6 subtasks marked `done`
- Build succeeds with no warnings
- Unity tests pass

---

### AGENT 3: Task 5 - LittleFS Storage Integration

**Objective:** Mount LittleFS filesystem, implement pattern CRUD operations, enforce storage quotas, and validate data integrity with CRC checks.

**Start Point:**
- Component: `firmware/components/storage/`
- Files: `pattern_storage.c`, `pattern_storage.h` (already scaffolded)
- Current state: Stub functions return ESP_OK

**Requirements from CANON.md:**
- **ADR-001**: Partition `littlefs` at offset `0x320000`, size `0x180000` (1.5MB)
- **ADR-004**: Pattern max size 256KB
- **ADR-005**: Mount path `/littlefs` (NOT `/prism`)
- Use `prism_pool_alloc` where possible
- CRC validation required for all writes

**Key Implementation Points:**
1. **Mount Configuration**: `esp_vfs_littlefs_register()` with partition label `"littlefs"`, base path `/littlefs`
2. **CRUD APIs**: `storage_init()`, `pattern_storage_open()`, `pattern_storage_write_chunk()`, `pattern_storage_finalize()`, `pattern_storage_list()`
3. **Quota Enforcement**: Track usage, maintain safety margin below 1.5MB
4. **CRC Validation**: Use `esp_rom_crc32()` for all pattern writes before finalize
5. **Atomic Writes**: Stage writes to temp files, `fsync()`, then `rename()` for atomicity
6. **Template Assets**: Manage shared template files under `/littlefs/templates`

**Subtasks (5 total):**
1. Mount LittleFS partition for pattern storage
2. Implement pattern storage CRUD APIs with quotas and CRC
3. Handle template assets and atomic write flow
4. Integrate storage APIs with protocol callbacks
5. Execute persistence validation and endurance tests

**Test Strategy:**
- Unity tests with `esp_littlefs` host stub for write/CRC scenarios
- Power-cycle testing to verify remount and persistence
- Heap monitoring via `prism_heap_monitor_dump_stats()` after repeated uploads

**Build Command:**
```bash
cd firmware
# NOTE: esp_littlefs dependency needed - add via component manager in this task
# idf.py add-dependency esp_littlefs
idf.py build
idf.py flash monitor
```

**Task Management:**
```bash
task-master show 5
task-master set-status --id=5 --status=in-progress
task-master set-status --id=5.1 --status=in-progress
task-master update-subtask --id=5.1 --prompt="progress notes..."
task-master set-status --id=5.1 --status=done
```

**Coordination:**
- Your completion unblocks Task 6 (Pattern Parser)
- Task 6 completion + yours → unblocks Task 7 (Hot Cache)
- Task 7 completion + yours → contributes to Task 10 (Templates)
- Independent of Agent 2's WiFi work

**Dependencies:**
```bash
# Add esp_littlefs component dependency:
cd firmware/components/storage
# Update CMakeLists.txt to add esp_littlefs requirement
# OR use: idf.py add-dependency esp_littlefs
```

**Success Criteria:**
- LittleFS mounts successfully at `/littlefs`
- Pattern CRUD operations work (create, read, list, delete)
- Quota enforcement prevents exceeding 1.5MB limit
- CRC validation catches corrupted writes
- Atomic writes via temp files + rename
- Template directory accessible at `/littlefs/templates`
- Power-cycle persistence verified
- All 5 subtasks marked `done`
- Build succeeds
- Unity tests pass

---

### AGENT 4: Task 8 - RMT LED Driver

**Objective:** Implement RMT-based WS2812B LED driver with double-buffered DMA memory achieving 60 FPS output for 320 LEDs.

**Start Point:**
- Component: `firmware/components/playback/`
- Files: `led_playback.c`, `led_playback.h` (already scaffolded)
- Current state: Stub functions return ESP_OK

**Requirements from CANON.md:**
- **ADR-003**: 320 LEDs (WS2812B protocol)
- Target: 60 FPS (16.67ms frame time)
- Priority: HIGHEST (Priority 10, Core 0)
- Stack: 8KB
- Must use DMA-capable memory

**Key Implementation Points:**
1. **RMT Configuration**: `rmt_new_tx_channel()` at 3.2MHz with WS2812 timing
2. **WS2812 Encoder**: Install LED strip driver or custom translator for bit timing
3. **Double Buffering**: Allocate 2 frame buffers via `heap_caps_malloc(MALLOC_CAP_DMA)`
4. **Buffer Swap**: Implement `led_driver_submit_frame()` and `led_driver_swap_buffers()`
5. **Timing Control**: Hardware timer or animation scheduler for 60 FPS cadence
6. **ISR Safety**: Ensure buffer operations are ISR-safe
7. **Diagnostics**: Log underruns when frame time exceeds 16.6ms

**Frame Buffer Math:**
- 320 LEDs × 3 bytes (RGB) = 960 bytes per frame
- Double-buffered: 1920 bytes total
- Ensure DMA alignment requirements

**Subtasks (5 total):**
1. Configure RMT channel and WS2812 encoder
2. Implement DMA-capable double-frame buffers
3. Design frame submission and swap APIs
4. Integrate timing control and ISR-safe workflow
5. Add underrun diagnostics and validation tests

**Test Strategy:**
- Hardware-in-the-loop: Send gradient frames, verify with logic analyzer
- Simulated tests: IDF RMT mock for buffer swap deadline verification
- CPU utilization monitoring via `esp_pm_lock_type`
- WS2812 timing spec validation (T0H, T0L, T1H, T1L)

**Build Command:**
```bash
cd firmware
idf.py build
idf.py flash monitor  # Hardware testing
```

**Task Management:**
```bash
task-master show 8
task-master set-status --id=8 --status=in-progress
task-master set-status --id=8.1 --status=in-progress
task-master update-subtask --id=8.1 --prompt="implementation details..."
task-master set-status --id=8.1 --status=done
```

**Coordination:**
- Your completion unblocks Task 9 (Effects Engine)
- Independent of Agent 2's WiFi work
- Independent of Agent 3's storage work
- Task 9 depends on your LED driver + Agent 3's parser + cache work

**Hardware Requirements:**
- ESP32-S3 development board
- 320 WS2812B LED strip (or test with smaller strip)
- Logic analyzer (optional but recommended for timing verification)
- Adequate power supply for LED strip

**Success Criteria:**
- RMT transmits WS2812B timing correctly (verify with logic analyzer)
- Frame buffers allocated in DMA-capable memory
- Buffer swap occurs within 16.67ms deadline
- 60 FPS sustained output
- ISR-safe operations verified
- Underrun diagnostics logged when frame time exceeded
- All 5 subtasks marked `done`
- Build succeeds
- Hardware validation passes

---

## Coordination Protocol

### Status Updates
Each agent should provide updates after completing each subtask:
```bash
# After completing a subtask:
task-master set-status --id=X.Y --status=done
# Then report to PM via shared channel
```

### Build Integration
- Each agent works in their component's directory
- PM will monitor overall firmware build health
- Agents should ensure `idf.py build` succeeds before marking task complete

### Conflict Resolution
- If agents touch overlapping files, PM will coordinate merges
- Use git branches per agent if conflicts arise
- Component boundaries minimize conflicts by design

### Blocking Issues
If blocked on dependencies or encountering issues:
1. Document the blocker with `task-master update-subtask`
2. Report to PM immediately
3. PM will reassign or resolve dependency

### CANON Compliance
All agents MUST check `.taskmaster/CANON.md` for specifications:
- Partition layout (ADR-001)
- WebSocket config (ADR-002)
- LED count (ADR-003)
- Pattern size (ADR-004)
- Mount paths (ADR-005)
- Template count (ADR-006)

### Testing Requirements
- Unity tests for all new code
- Hardware verification where applicable
- Memory profiling with `prism_heap_monitor`
- Build verification before completion

---

## Phase 2 Planning (After Phase 1)

Once Tasks 2, 5, and 8 complete:

**Next Parallel Wave:**
- Agent 2: Task 3 (WebSocket Server) - depends on completed Task 2
- Agent 3: Task 6 (Pattern Parser) - depends on completed Task 5
- Agent 4: Continue monitoring or assist with integration

**Subsequent Waves:**
- Task 4 (TLV Protocol) - depends on Task 3
- Task 7 (Hot Cache) - depends on Tasks 5, 6
- Task 9 (Effects) - depends on Tasks 6, 7, 8
- Task 10 (Templates) - depends on Tasks 5, 6, 7, 9

---

## PM Responsibilities (Agent 1)

- Monitor git status and build health
- Resolve merge conflicts
- Coordinate handoffs between tasks
- Update this document as tasks complete
- Manage overall project timeline
- Ensure CANON compliance
- Run integration tests as components complete

---

**Ready to deploy! Each agent should:**
1. Read their task brief above
2. Review CANON.md for specifications
3. Start with `task-master show <task-id>`
4. Begin implementation
5. Report progress after each subtask completion

**Good luck, team! Let's build PRISM.K1! 🚀**
