# PRISM K1 - System Handover Document

**Date:** 2025-10-16
**Status:** ✅ READY FOR TASK EXECUTION
**Captain:** SpectraSynq
**System:** Knowledge Fortress + TaskMaster + Multi-Agent Coordination
**Previous Handover:** October 15, 2025 (superseded)

---

## 🚨 CRITICAL: Read This First

**YOU MUST NOT BEGIN CODING UNTIL:**

1. ✅ You have read this ENTIRE document (estimated 30-45 minutes)
2. ✅ You have explored the project source code structure
3. ✅ You have reviewed `.agent/instructions.md` for mission-critical constraints
4. ✅ You have reviewed `.agent/research-first.md` for mandatory methodology
5. ✅ You understand the ESP32-S3 hardware limitations completely
6. ✅ You have familiarized yourself with the multi-agent workflow system

**FAILURE TO COMPLETE THESE STEPS WILL RESULT IN:**
- Memory fragmentation (CATASTROPHIC for ESP32-S3)
- Build failures
- Hardware damage potential
- Loss of field reliability
- Violation of real-time performance requirements

---

## 📋 Table of Contents

1. [Project Overview & Mission](#1-project-overview--mission)
2. [Hardware Architecture & Constraints](#2-hardware-architecture--constraints)
3. [Software Architecture](#3-software-architecture)
4. [Current Project State](#4-current-project-state)
5. [Recent Changes & Migration](#5-recent-changes--migration)
6. [Development Protocols (MANDATORY)](#6-development-protocols-mandatory)
7. [Multi-Agent Workflow System](#7-multi-agent-workflow-system)
8. [Tool Usage & MCP Servers](#8-tool-usage--mcp-servers)
9. [Error Handling Protocol](#9-error-handling-protocol)
10. [Research-First Methodology](#10-research-first-methodology)
11. [Code Change Protocol](#11-code-change-protocol)
12. [Testing & Verification](#12-testing--verification)
13. [Common Pitfalls & Solutions](#13-common-pitfalls--solutions)
14. [Knowledge Resources](#14-knowledge-resources)
15. [Handover Checklist](#15-handover-checklist)

---

## 1. Project Overview & Mission

### 1.1 What is PRISM.k1?

**PRISM.k1** is a production-grade ESP32-S3 firmware that controls LED patterns for a hardware LED controller device. It is NOT a prototype or experiment - this is production firmware that must work reliably in the field.

**Core Functionality:**
- Host local HTTPS + WebSocket services for pattern control (binary TLV protocol)
- Accept pattern uploads and metadata updates over the TLV channel
- Store patterns in the LittleFS filesystem on flash memory
- Play back patterns at 60 FPS to physical LEDs via the RMT peripheral
- Provide reliable, low-latency pattern switching (<100ms)
- Maintain stable operation for weeks/months without restart

**Critical Success Factors:**
1. **Memory Safety:** Absolutely ZERO heap fragmentation tolerance
2. **Real-Time Performance:** 60 FPS LED output, <100ms pattern switch
3. **Field Reliability:** Must run for months without restart
4. **Build Stability:** EVERY commit must build successfully
5. **Resource Efficiency:** <150KB heap usage at runtime

### 1.2 Why This Project Exists

**Business Context:**
- Part of K1 lighting system ecosystem
- Deployed in customer installations (field hardware)
- Firmware updates must be reliable and safe
- Downtime is not acceptable
- Memory leaks are catastrophic (device will crash after hours/days)

**Technical Context:**
- ESP32-S3 is resource-constrained (512KB RAM total)
- FreeRTOS requires careful memory management
- RMT peripheral has specific timing requirements
- Flash wear leveling must be considered (LittleFS)
- WebSocket connections can fail and must recover gracefully

### 1.3 Project Goals

**Primary Goals:**
1. Implement complete firmware architecture (53 main tasks)
2. Achieve 60 FPS LED output consistently
3. Pattern switch latency <100ms (p95)
4. Zero memory fragmentation
5. Network reconnection without restart
6. Multi-month uptime reliability

**Performance Targets:**
- LED Update Rate: 60 FPS (16.67ms frame time)
- Pattern Switch: <100ms (user-perceivable threshold)
- Memory Usage: <150KB heap (leaving headroom for FreeRTOS)
- Command Latency: <50ms for WebSocket control messages
- Storage Access: <10ms for pattern loading
- Boot Time: <3 seconds from power-on to first LED output

---

## 2. Hardware Architecture & Constraints

### 2.1 ESP32-S3 Hardware

**MCU Specifications:**
```
Processor: Xtensa LX7 Dual-Core @ 240MHz
RAM: 512KB SRAM (total, shared between cores)
Flash: 8MB (for code + LittleFS filesystem)
WiFi: 802.11 b/g/n (2.4GHz only)
Bluetooth: BLE 5.0 (not used in this project)
RMT Peripheral: 8 channels for LED driving
GPIO: 45 pins (multiple functions per pin)
```

**CRITICAL MEMORY CONSTRAINT:**
- Total RAM: 512KB
- FreeRTOS overhead: ~50KB
- Task stacks: ~100KB (5 tasks × ~20KB each)
- Network buffers: ~50KB
- LED frame buffer: ~30KB (depends on LED count)
- **Available heap: ~280KB maximum**
- **Runtime target: <150KB heap usage (46% safety margin)**

**Why This Matters:**
- ESP32-S3 does NOT have memory protection unit
- Heap fragmentation WILL crash the system
- Stack overflow WILL crash the system
- Out-of-memory WILL crash the system
- Memory leaks WILL accumulate and crash after hours/days

### 2.2 Hardware Peripherals

**RMT (Remote Control) Peripheral:**
- Purpose: Generate precise timing signals for LEDs (WS2812B, SK6812, etc.)
- Channels: 8 independent channels available
- Timing: Sub-microsecond precision required
- DMA: Direct Memory Access for CPU-free operation
- Constraints: Timing violations will corrupt LED data

**WiFi Radio:**
- Purpose: Provide STA connectivity for local HTTPS + WebSocket services (with AP/captive portal onboarding)
- Constraints:
  - Cannot use while flash write in progress
  - Power consumption spikes during transmit
  - Connection failures must be handled gracefully
  - Reconnection without restart required

**Flash Memory (8MB):**
- Purpose: Store firmware code and LittleFS filesystem
- Wear Leveling: LittleFS handles this automatically
- Constraints:
  - Write cycles limited (~100,000 per block)
  - Cannot read while erasing
  - Must use atomic operations for critical data
  - Dual-OTA layout (app0/app1) with 1.5MB LittleFS partition at `/littlefs`

**GPIO Pins:**
- LED data out (RMT channels)
- Status LED
- Optional: Touch sensors, buttons (future)

### 2.3 ESP-IDF Framework

**Version:** ESP-IDF v5.x (latest stable)

**Key Components Used:**
```
FreeRTOS:          Real-time operating system
LittleFS:          Filesystem for pattern storage
esp_wifi:          WiFi stack
esp_netif:         Network interface abstraction
esp_https_server:  HTTPS + WebSocket server
mbedTLS:           TLS/SSL for secure connections
RMT driver:        LED control peripheral
NVS:               Non-volatile storage for config
```

**Build System:**
```
CMake:             Build configuration
idf.py:            ESP-IDF build tool
Kconfig:           Configuration system
```

**CRITICAL BUILD REQUIREMENT:**
- EVERY task completion MUST verify: `cd firmware && idf.py build`
- Build failures are NOT acceptable
- Commit only after successful build verification

---

## 3. Software Architecture

### 3.1 Component-Based Architecture

The firmware is organized into ESP-IDF components (located in `firmware/components/`):

**Component Structure:**
```
firmware/
├── main/                          # Main application
│   ├── main.c                     # Entry point, task creation
│   ├── CMakeLists.txt
│   └── Kconfig.projbuild
└── components/
    ├── core/                     # Core utilities (memory pools, safety libs)
    │   ├── include/
    │   │   └── prism_memory_pool.h
    │   ├── prism_memory_pool.c
    │   └── CMakeLists.txt
    ├── network/                  # WiFi bring-up, HTTPS server, TLV WebSocket handling
    │   └── CMakeLists.txt        # Source stubs pending implementation
    ├── storage/                  # LittleFS mount, pattern persistence
    │   └── CMakeLists.txt        # Source stubs pending implementation
    ├── playback/                 # LED driver, effects, timeline engine
    │   └── CMakeLists.txt        # Source stubs pending implementation
    └── templates/                # Embedded template catalog
        └── CMakeLists.txt        # Source stubs pending implementation
```

### 3.2 FreeRTOS Task Architecture

**Task Structure (5 main tasks):**

```c
// Task 1: Network Task (Core 1, Priority 5)
void network_task(void *pvParameters) {
    // Responsibilities:
    // - WiFi connection management
    // - Handle AP onboarding + captive portal before STA credentials stored
    // - Run HTTPS + WebSocket servers (binary TLV protocol)
    // - Manage client sessions and retransmit queues
    // - Network error recovery
    // Stack: 8KB
    // Core: 1 (network stack runs on Core 1)
}

// Task 2: Storage Task (Core 0, Priority 4)
void storage_task(void *pvParameters) {
    // Responsibilities:
    // - LittleFS filesystem operations
    // - Pattern download and caching
    // - Metadata management
    // Stack: 6KB
    // Core: 0
}

// Task 3: Playback Task (Core 0, Priority 10 - HIGHEST)
void playback_task(void *pvParameters) {
    // Responsibilities:
    // - 60 FPS LED frame generation
    // - RMT peripheral control
    // - Effects engine execution
    // Stack: 8KB
    // Core: 0 (dedicated for real-time performance)
    // CRITICAL: This task must NEVER block for >16ms
}

// Task 4: Template Task (Core 0, Priority 3)
void template_task(void *pvParameters) {
    // Responsibilities:
    // - Template pattern generation
    // - Pattern metadata extraction
    // - Cache management
    // Stack: 6KB
    // Core: 0
}

// Task 5: Monitoring Task (Core 1, Priority 2)
void monitoring_task(void *pvParameters) {
    // Responsibilities:
    // - System health checks
    // - Memory usage monitoring
    // - Watchdog feeding
    // - Status LED updates
    // Stack: 4KB
    // Core: 1
}
```

**Task Communication:**
- FreeRTOS Queues (for inter-task messaging)
- Mutexes (for shared resource protection)
- Event Groups (for synchronization)
- Semaphores (for resource counting)

**CRITICAL TASK RULES:**
1. Playback task has highest priority (real-time LED output)
2. Network task on Core 1 (isolate from real-time work)
3. Never call blocking operations in high-priority tasks
4. Always check queue/mutex return values
5. Use timeout parameters, never wait forever

### 3.3 Memory Management Strategy

**MANDATORY MEMORY RULES:**

```c
// ✅ CORRECT: Static allocation where possible
static uint8_t led_buffer[NUM_LEDS * 3];
static pattern_t current_pattern;

// ✅ CORRECT: One-time allocation at startup
void *buffer = heap_caps_malloc(size, MALLOC_CAP_DMA);

// ❌ WRONG: Repeated malloc/free in loops
for (int i = 0; i < 1000; i++) {
    void *temp = malloc(100);  // WILL FRAGMENT HEAP
    // ... work ...
    free(temp);
}

// ✅ CORRECT: Reuse buffers
static uint8_t work_buffer[1024];
for (int i = 0; i < 1000; i++) {
    // Reuse work_buffer
}

// ✅ CORRECT: Memory pool pattern
typedef struct {
    uint8_t buffer[256];
    bool in_use;
} buffer_pool_t;

static buffer_pool_t pool[10];  // Pre-allocated pool
```

**Heap Monitoring:**
```c
// Check heap before and after operations
size_t free_before = esp_get_free_heap_size();
size_t min_free_before = esp_get_minimum_free_heap_size();

// ... perform operation ...

size_t free_after = esp_get_free_heap_size();
size_t min_free_after = esp_get_minimum_free_heap_size();

ESP_LOGI(TAG, "Heap: %zu -> %zu (min: %zu -> %zu)",
         free_before, free_after, min_free_before, min_free_after);

// ALERT if minimum free heap drops below 100KB
if (min_free_after < 100*1024) {
    ESP_LOGE(TAG, "CRITICAL: Low heap detected!");
}
```

### 3.4 Data Flow Architecture

**Pattern Download Flow:**
```
K1 Host (WebSocket)
    ↓
Network Task (receive)
    ↓ (Queue: pattern_download_queue)
Storage Task (validate + save)
    ↓ (LittleFS write)
Flash Memory
    ↓ (Signal: pattern_ready_event)
Template Task (cache)
    ↓ (Queue: pattern_load_queue)
Playback Task (load + render)
    ↓ (RMT peripheral)
LEDs (physical output)
```

**Command Flow (Binary TLV):**
```
Client (WebSocket TLV frame)
    ↓
Network Task (decode + validate)
    ↓ (Queue: control_command_queue)
Playback Task (apply command)
    ↓ (Shared status struct protected by mutex)
Network Task (status frame encode)
    ↓
Client (status/ack frame)
```

---

## 4. Current Project State

### 4.1 Task Breakdown

**Total Tasks:** 56 main tasks (expandable to ~80-100 subtasks)

**Task Distribution by Component:**
```
core:             Tasks 1-5    (initialization, config, memory pools)
network:          Tasks 6-17   (WiFi, HTTPS server, WebSocket TLV, mDNS)
storage:          Tasks 18-25  (LittleFS, pattern safety, caching)
playback:         Tasks 26-40  (LED driver, timeline, effects)
templates:        Tasks 41-48  (template system, metadata)
integration:      Tasks 49-53  (testing, optimization, docs)
resilience:       Tasks 54-56  (memory pools, heap monitoring, bounds checks)
```

**Task Status:** Use `task-master list` for live counts. At handover time, baseline setup tasks (1-3) were complete and the new resilience tasks (54-56) were queued but not yet in progress.

**Critical Path Tasks:**
1. Task 54: Memory pool manager (must land before any dynamic allocation-heavy work)
2. Task 55: Heap monitoring system (keeps memory issues visible in real time)
3. Task 56: Bounds checking utilities (input hardening)
4. Task 6: WiFi connection management (network stack baseline)
5. Task 18: LittleFS initialization (storage baseline)
6. Task 26: LED driver initialization (playback baseline)

### 4.2 Code Completeness

**What EXISTS (already implemented):**
```
firmware/
├── main/main.c                    # ✅ Entry point skeleton + heap monitor placeholder
├── components/core/
│   ├── prism_memory_pool.c        # ✅ Memory pool manager (Task 54 implementation)
│   └── include/prism_memory_pool.h
└── CMakeLists.txt                 # ✅ Project configuration (component directories registered)
```

**What DOES NOT EXIST (needs implementation):**
```
firmware/components/
├── network/                       # ❌ Awaiting WiFi + HTTPS/WebSocket TLV sources
├── storage/                       # ❌ Awaiting LittleFS + validation sources
├── playback/                      # ❌ Awaiting LED/timeline sources
└── templates/                     # ❌ Awaiting embedded template catalog sources
```

**Build Status:**
- ✅ Basic firmware builds successfully
- ✅ ESP-IDF environment configured
- ✅ Component structure defined
- ❌ Functional components not yet implemented

### 4.3 Recent Changes & Configuration Migration

**CRITICAL: Major configuration consolidation completed on October 15, 2025**

**What Changed:**
1. Created `.agent/` directory with centralized documentation (9 files)
2. Added 7 new MCP servers (total: 8 servers)
3. Configured multi-agent workflow system
4. Created verification and automation scripts
5. Fixed GitHub Copilot instruction references

**Key Files Added:**
```
.agent/
├── instructions.md              # Master agent instructions
├── workflow.md                  # Development workflow
├── multi-agent.md               # Multi-agent coordination
├── research-first.md            # MANDATORY methodology
├── mcp-usage.md                 # Tool reference
├── cursor-workflow.md           # Cursor-specific patterns
├── taskmaster-reference.md      # Complete command reference
├── multi-agent-quickstart.md    # 15-min deployment guide
└── dashboard-proposals.md       # Monitoring options

.taskmaster/
├── scripts/
│   ├── multi-agent.sh           # 851-line automation suite
│   └── verify-setup.sh          # 67-test verification script
└── agent-capabilities.json      # Agent configuration profiles

CLAUDE.md                         # Pointer to .agent/instructions.md
AGENT.md                          # Pointer to .agent/instructions.md
MIGRATION.md                      # Complete changelog
STATUS.md                         # System status
DELIVERABLES.md                   # Recent deliverables summary
```

**Impact on Your Work:**
- ✅ All documentation is now centralized in `.agent/`
- ✅ MCP servers are configured and ready to use
- ✅ Multi-agent workflow system is operational
- ✅ Verification script can validate setup
- ⚠️ You MUST review `.agent/instructions.md` before coding

---

## 5. Recent Changes & Migration

### 5.1 Configuration Consolidation (Oct 15, 2025)

**Changes Made:**
- Consolidated 4 duplicate files into single `.agent/` directory
- Added 7 new MCP servers (filesystem, context7, brave-search, etc.)
- Created multi-agent automation system
- Fixed broken GitHub Copilot references
- Saved ~19.6KB of duplicate content

**Files Deleted (redundant):**
- ❌ `AGENTS.md`
- ❌ `.rules`
- ❌ `.taskmaster/CLAUDE.md`
- ❌ `.taskmaster/AGENT.md`
- ❌ `.cursor/rules/taskmaster/` directory

**Files Created (new):**
- ✅ `.agent/` directory (9 comprehensive files)
- ✅ `.taskmaster/scripts/multi-agent.sh` (automation)
- ✅ `.taskmaster/scripts/verify-setup.sh` (verification)
- ✅ `.taskmaster/agent-capabilities.json` (agent configs)

**IMPORTANT: No Breaking Changes**
- All changes are additive or consolidating
- Old workflows still work
- Single-agent development still supported
- Can rollback via git if needed

### 5.2 Multi-Agent System Deployment

**New Capability: 5 Concurrent Agents**

The project now supports multi-agent collaborative development:

**Agent Specializations:**
```
Agent 1 (network):      WebSocket, WiFi, protocols (max 2 concurrent tasks)
Agent 2 (storage):      LittleFS, patterns, caching (max 2 concurrent tasks)
Agent 3 (playback):     LED driver, effects (max 1 task - CRITICAL PATH)
Agent 4 (integration):  Testing, debugging (max 3 tasks - blocker resolver)
Agent 5 (templates):    Pattern templates (max 2 tasks - parallel safe)
```

**Velocity Improvement:**
- Single agent: 3-5 tasks/day → 10-15 days total
- 5 concurrent agents: 12-20 tasks/day → 3-5 days total
- **Speedup: 3x faster!**

**System Features:**
- ✅ Atomic task claiming (no conflicts)
- ✅ Automatic health monitoring
- ✅ Real-time dashboard
- ✅ Build verification per task
- ✅ Stale task auto-recovery
- ✅ Research integration

**Deployment Status:**
- ⏳ Ready to deploy (not yet activated)
- ⏳ Verification script available
- ⏳ Automation suite ready
- ⏳ Documentation complete

---

## 6. Development Protocols (MANDATORY)

### 6.1 Research-First Methodology

**⚠️ CRITICAL: You MUST research before coding**

**Protocol:**

```
1. RECEIVE TASK
   └→ Read task details from taskmaster
   └→ Understand requirements completely
   └→ Identify unknowns and uncertainties

2. RESEARCH PHASE (MANDATORY)
   └→ Use brave-search MCP for latest best practices
   └→ Use context7 MCP for ESP-IDF/FreeRTOS docs
   └→ Search for specific patterns/solutions
   └→ Collect multiple sources (3-5 minimum)
   └→ Narrow down to best approach

3. PLANNING PHASE
   └→ Map out all code changes required
   └→ Identify injection points
   └→ List all dependencies
   └→ Document expected behavior
   └→ Log plan via: task-master update-subtask

4. VERIFICATION PHASE
   └→ Confirm plan with user if ANY uncertainty
   └→ Get explicit approval before coding
   └→ Document any assumptions

5. IMPLEMENTATION PHASE
   └→ Make code changes following plan
   └→ Test incrementally
   └→ Verify build: cd firmware && idf.py build

6. DOCUMENTATION PHASE
   └→ Log what worked via: task-master update-subtask
   └→ Log what didn't work
   └→ Update task status: task-master set-status --id=X --status=done
```

**Research Query Strategy:**

```bash
# ❌ WRONG: Vague query
brave-search: "esp32 wifi"

# ✅ CORRECT: Specific query with context
brave-search: "ESP32-S3 WebSocket server keepalive strategy ESP-IDF v5"

# ❌ WRONG: Generic query
brave-search: "led control"

# ✅ CORRECT: Precise technical query
brave-search: "ESP32 RMT peripheral WS2812B timing 60fps DMA buffer"

# ❌ WRONG: Broad topic
brave-search: "memory management"

# ✅ CORRECT: Specific to constraints
brave-search: "ESP32 FreeRTOS heap fragmentation prevention patterns"
```

**When to Research:**
- ✅ Before implementing any new component
- ✅ When encountering unfamiliar ESP-IDF APIs
- ✅ When choosing between multiple approaches
- ✅ When performance optimization is needed
- ✅ When debugging complex issues
- ✅ When ANY uncertainty exists

### 6.2 Stop Conditions (When to STOP and Ask)

**🛑 MANDATORY STOP CONDITIONS:**

You MUST stop work and request user assistance when:

**1. Uncertainty About Approach:**
```
"I see two possible ways to handle disconnected WebSocket clients:
 A) Implement heartbeat timeout using httpd_ws_send_frame_async()
 B) Force-close sockets and require clients to reconnect

 I'm not 100% certain which is better for our constraints.
 Should I research more or do you have a preference?"
```

**2. Unclear Requirements:**
```
"Task says 'implement pattern caching' but doesn't specify:
 - Cache size limit
 - Eviction policy (LRU, FIFO, etc.)
 - Where to store cache (RAM vs Flash)
 
 I need clarification before proceeding."
```

**3. Build Failures (After 2+ Attempts):**
```
"Build failing with error: undefined reference to 'esp_wifi_init'
 
 Attempted fixes:
 1. Added esp_wifi component to CMakeLists.txt
 2. Checked idf_component_register() includes
 
 Both failed. I don't have a clear solution. Requesting assistance."
```

**4. Constraint Violations:**
```
"Implementation requires 200KB heap allocation but constraint is <150KB.
 
 I'm uncertain how to proceed - should I:
 - Redesign to use less memory?
 - Request constraint relaxation?
 - Use different approach?
 
 Need guidance."
```

**5. Missing Context:**
```
"I've lost context on why we're using RMT peripheral instead of SPI.
 
 I see the PRD mentions it but I'm unsure of the trade-offs.
 Should I research or is there existing documentation?"
```

**6. Test Failures:**
```
"LED output test shows artifacts at 60 FPS but works at 30 FPS.
 
 Possible causes:
 - RMT timing configuration
 - DMA buffer size
 - FreeRTOS task priority
 
 Not certain which is root cause. Need to discuss approach."
```

**7. Repeated Failures (3+ consecutive):**
```
"Attempted to fix WebSocket disconnect issue 3 times:
 1. Increased timeout - failed
 2. Added error handlers - failed  
 3. Changed keepalive settings - failed
 
 I don't have a clear solution. Stopping for assistance."
```

**How to Request Assistance:**

```markdown
## Issue Summary
[One sentence description]

## Current Task
Task ID: X.X
Title: [task title]

## What I Was Doing
[Step-by-step what you attempted]

## What Went Wrong
[Specific error/failure/uncertainty]

## What I've Tried
1. [Attempt 1] - [Result]
2. [Attempt 2] - [Result]
3. [Research performed] - [Findings]

## Why I'm Stuck
[Explain uncertainty or lack of clear solution]

## Possible Paths Forward
A) [Option 1 with pros/cons]
B) [Option 2 with pros/cons]

## Request
[What specific help you need]
```

### 6.3 Build Verification Protocol

**⚠️ MANDATORY: Build verification after EVERY code change**

**Protocol:**

```bash
# 1. Navigate to firmware directory
cd firmware

# 2. Clean build (if major changes)
idf.py fullclean
idf.py build

# 3. Incremental build (for minor changes)
idf.py build

# 4. Check build output
# ✅ Should end with: "Project build complete."
# ❌ Any warnings about memory usage are CRITICAL
# ❌ Linker errors are CRITICAL

# 5. Verify binary size
ls -lh build/*.bin

# Expected sizes:
# bootloader.bin:    ~30KB
# partition-table.bin: ~3KB  
# prism-firmware.bin: <1MB (should grow as features added)

# 6. If build fails, check:
- CMakeLists.txt syntax
- Component dependencies (idf_component_register)
- Missing includes
- Linker script issues
```

**Build Failure Response:**

```bash
# Attempt 1: Check syntax and dependencies
idf.py build 2>&1 | tee build.log
grep -i error build.log

# Attempt 2: Clean build
idf.py fullclean
idf.py build 2>&1 | tee build-clean.log

# If still failing after 2 attempts:
# 🛑 STOP and request assistance
```

### 6.4 Git Commit Protocol

**Commit ONLY After:**
1. ✅ Build verification passed
2. ✅ Code tested (if testable at this stage)
3. ✅ Task status updated
4. ✅ No compiler warnings introduced
5. ✅ Memory usage within limits

**Commit Message Format:**

```
feat(component): Brief description of change (task X.X)

- Detailed point 1
- Detailed point 2
- Detailed point 3

Task: X.X
Status: done
Build: verified
Memory: no regression
```

**Example:**

```
feat(network): Implement WiFi connection management (task 6.1)

- Added WiFi initialization in network_init()
- Implemented reconnection logic with exponential backoff
- Added event handlers for disconnect/connect events
- Configured WiFi as STA mode with DHCP

Task: 6.1
Status: done
Build: verified ✓
Memory: 148KB heap (within 150KB limit)
```

---

## 7. Multi-Agent Workflow System

### 7.1 Tagged Work Streams

**5 Isolated Contexts:**

```
master:       Original task list (53 main tasks)
network:      Tasks 6-17 (WiFi, HTTPS server, WebSocket TLV, mDNS)
storage:      Tasks 18-25 (LittleFS, patterns, cache)
playback:     Tasks 26-40 (LED driver, effects, RMT)
templates:    Tasks 41-48 (Template system, metadata)
integration:  Tasks 49-53 (Testing, optimization)
```

**Tag Isolation Benefits:**
- ✅ No task conflicts between agents
- ✅ Parallel execution of independent modules
- ✅ Domain-specific agent assignment
- ✅ Easy load balancing

**Tag Operations:**

```bash
# List all tags
task-master tags

# Switch to specific tag
task-master use-tag network

# Create new tag
task-master add-tag <name> --description="..."

# Copy tasks from another tag
task-master add-tag my-work --copy-from-current
```

### 7.2 Multi-Agent Commands

**Task Allocation:**

```bash
# Option 1: Automated allocation
./taskmaster allocate network agent-1

# Option 2: Manual task selection
task-master next --tag=network
task-master show 6.1
task-master set-status --tag=network --id=6.1 --status=in-progress
```

**Task Release:**

```bash
# Complete task
./taskmaster release network 6.1 done agent-1

# Or manually
task-master set-status --tag=network --id=6.1 --status=done
rm .taskmaster/.locks/task-network-6.1.claim
```

**Health Monitoring:**

```bash
# Check system health
./taskmaster health-check

# View dashboard
./taskmaster dashboard

# Collect metrics
./taskmaster metrics
```

### 7.3 Agent Specialization

**Your Agent Profile:**

If you're assigned to a specific agent role, follow these guidelines:

**Agent 1 (Network Specialist):**
- Skills: websocket, wifi, protocols, mDNS
- Tag: network
- Max concurrent: 2 tasks
- Research: enabled
- Focus: Connection reliability, error recovery

**Agent 2 (Storage Specialist):**
- Skills: filesystem, littlefs, caching, compression
- Tag: storage
- Max concurrent: 2 tasks
- Research: enabled
- Focus: File integrity, wear leveling

**Agent 3 (Playback Specialist):**
- Skills: led-driver, effects, animation, rmt-peripheral
- Tag: playback
- Max concurrent: 1 task (CRITICAL PATH)
- Research: enabled
- Focus: Real-time performance, 60 FPS guarantee

**Agent 4 (Integration Lead):**
- Skills: testing, debugging, optimization, profiling
- Tag: integration, master
- Max concurrent: 3 tasks
- Role: Blocker resolver
- Research: enabled
- Focus: System-wide issues, performance bottlenecks

**Agent 5 (Template Designer):**
- Skills: creative, patterns, effects-design, metadata
- Tag: templates
- Max concurrent: 2 tasks
- Parallel safe: yes
- Research: enabled
- Focus: Pattern quality, metadata accuracy

---

## 8. Tool Usage & MCP Servers

### 8.1 Available MCP Servers

**8 MCP Servers Configured:**

```json
{
  "task-master-ai": {
    "purpose": "Task management and workflow",
    "priority": "⭐⭐⭐⭐⭐",
    "usage": "Always for task operations"
  },
  "filesystem": {
    "purpose": "Direct file operations on project",
    "priority": "⭐⭐⭐⭐⭐",
    "usage": "Read/write code files",
    "constraint": "Limited to: /Users/spectrasynq/.../PRISM.k1"
  },
  "context7": {
    "purpose": "ESP-IDF and FreeRTOS documentation",
    "priority": "⭐⭐⭐⭐⭐",
    "usage": "Research ESP32 APIs, FreeRTOS patterns",
    "cost": "FREE (rate limited without API key)"
  },
  "brave-search": {
    "purpose": "Web research for best practices",
    "priority": "⭐⭐⭐⭐⭐",
    "usage": "Research before implementation",
    "api_key": "configured"
  },
  "sequential-thinking": {
    "purpose": "Complex problem solving",
    "priority": "⭐⭐⭐⭐",
    "usage": "Architecture decisions, debugging",
    "cost": "FREE"
  },
  "memory": {
    "purpose": "Agent state persistence",
    "priority": "⭐⭐⭐⭐",
    "usage": "Save context between sessions",
    "cost": "FREE"
  },
  "git": {
    "purpose": "Version control operations",
    "priority": "⭐⭐⭐",
    "usage": "Commit, diff, log",
    "cost": "FREE"
  },
  "sqlite": {
    "purpose": "Data operations",
    "priority": "⭐⭐",
    "usage": "Metrics storage (if needed)",
    "cost": "FREE"
  }
}
```

### 8.2 Tool Usage Guidelines

**Task Management (task-master-ai):**

```bash
# Get next task
task-master next --tag=<your-tag>

# View task details
task-master show <id>

# Research task context
task-master research \
  --task-ids=<id> \
  --query="ESP32 implementation best practices" \
  --save-to=<id>

# Log implementation notes
task-master update-subtask \
  --id=<id> \
  --prompt="Implementation plan:
    1. Initialize WiFi driver
    2. Configure STA mode
    3. Add event handlers
    ..."

# Update task status
task-master set-status --id=<id> --status=in-progress
task-master set-status --id=<id> --status=done
```

**Filesystem Operations:**

```bash
# Read source code
filesystem:read_file "firmware/main/main.c"

# Write new component (example)
filesystem:write_file \
  "firmware/components/network/network_manager.c" \
  "<code content>"

# List directory structure
filesystem:list_directory "firmware/components"

# Search for patterns
filesystem:search_files "firmware" "esp_wifi"
```

**Research Operations:**

```bash
# Web search for best practices
brave-search: "ESP32-S3 WebSocket server TLV implementation ESP-IDF v5"

# ESP-IDF documentation
context7: "esp_wifi_init API reference FreeRTOS thread safety"

# FreeRTOS patterns
context7: "FreeRTOS queue usage examples ESP32 inter-task communication"
```

**Git Operations:**

```bash
# Check current status
git:status

# View diff
git:diff

# Commit changes
git:commit -m "feat(network): implement WiFi (task 6.1)"

# View log
git:log -n 10
```

### 8.3 Tool Priority for Common Tasks

**When Starting a Task:**
1. task-master show <id>
2. brave-search (research best practices)
3. context7 (ESP-IDF API documentation)
4. task-master update-subtask (log plan)

**When Implementing:**
1. filesystem:read_file (understand existing code)
2. filesystem:write_file (make changes)
3. Build verification (cd firmware && idf.py build)
4. task-master update-subtask (log progress)

**When Stuck:**
1. brave-search (search for solutions)
2. context7 (check API documentation)
3. sequential-thinking (analyze problem)
4. STOP and request assistance (if still stuck)

**When Completing:**
1. Build verification (final check)
2. git:commit (if build passes)
3. task-master set-status --status=done
4. task-master next (get next task)

---

## 9. Error Handling Protocol

### 9.1 Error Categories

**Category 1: Build Errors**

Symptoms:
- idf.py build fails
- Linker errors
- Missing includes
- CMakeLists.txt issues

Response:
```bash
# Attempt 1: Check syntax
idf.py build 2>&1 | grep -i error

# Attempt 2: Clean build
idf.py fullclean && idf.py build

# If fails after 2 attempts:
# 🛑 STOP - Search for specific error
brave-search: "<exact error message> ESP-IDF v5"

# If no solution found:
# 🛑 STOP - Request user assistance
```

**Category 2: Runtime Errors**

Symptoms:
- Firmware crashes
- Watchdog resets
- Memory corruption
- Task hangs

Response:
```bash
# Check logs for panic/abort
idf.py monitor | grep -i "abort\|panic\|assert"

# Analyze backtrace
# Look for:
- Stack overflow (increase task stack size)
- Heap corruption (check malloc/free pairs)
- Mutex deadlock (check lock ordering)
- NULL pointer dereference (add NULL checks)

# If root cause unclear:
# 🛑 STOP - Research specific crash pattern
brave-search: "ESP32 <crash type> FreeRTOS debugging"

# If still unclear:
# 🛑 STOP - Request user assistance
```

**Category 3: Performance Issues**

Symptoms:
- LED frame rate <60 FPS
- Pattern switch >100ms
- High heap usage

Response:
```bash
# Profile performance
# Add timing measurements:
int64_t start = esp_timer_get_time();
// ... code block ...
int64_t duration = esp_timer_get_time() - start;
ESP_LOGI(TAG, "Duration: %lld us", duration);

# Check heap usage
ESP_LOGI(TAG, "Free heap: %zu", esp_get_free_heap_size());
ESP_LOGI(TAG, "Min free: %zu", esp_get_minimum_free_heap_size());

# Analyze findings and research optimizations
brave-search: "ESP32 <specific bottleneck> optimization"

# If optimization unclear:
# 🛑 STOP - Discuss with user
```

**Category 4: Integration Errors**

Symptoms:
- Components don't interact correctly
- Data corruption between tasks
- Timing issues

Response:
```bash
# Check inter-task communication
# - Queue full/empty handling
# - Mutex lock timeouts
# - Event group synchronization

# Add debug logging
ESP_LOGD(TAG, "Queue send: %d items pending", uxQueueMessagesWaiting(queue));
ESP_LOGD(TAG, "Mutex lock: holder=%p, count=%d", mutex_holder, lock_count);

# Research integration patterns
context7: "ESP32 FreeRTOS task communication patterns"

# If integration unclear:
# 🛑 STOP - Discuss architecture with user
```

### 9.2 Debug Logging Strategy

**Logging Levels:**

```c
// Use appropriate levels
ESP_LOGE(TAG, "CRITICAL ERROR: %s", error_msg);  // Errors only
ESP_LOGW(TAG, "Warning: %s", warning_msg);       // Important warnings
ESP_LOGI(TAG, "Info: %s", info_msg);             // General info
ESP_LOGD(TAG, "Debug: %s", debug_msg);           // Debug (disabled in release)
ESP_LOGV(TAG, "Verbose: %s", verbose_msg);       // Very verbose (disabled normally)
```

**Strategic Logging Points:**

```c
// Component initialization
ESP_LOGI(TAG, "Initializing WiFi...");
esp_err_t ret = esp_wifi_init(&wifi_init_config);
if (ret != ESP_OK) {
    ESP_LOGE(TAG, "WiFi init failed: %s", esp_err_to_name(ret));
    return ret;
}
ESP_LOGI(TAG, "WiFi initialized successfully");

// Task entry/exit
void network_task(void *pvParameters) {
    ESP_LOGI(TAG, "Network task started on core %d", xPortGetCoreID());
    
    while (1) {
        // Task work...
    }
    
    ESP_LOGW(TAG, "Network task exiting (unexpected!)");
    vTaskDelete(NULL);
}

// Critical operations
ESP_LOGI(TAG, "Loading pattern: %s", pattern_name);
size_t heap_before = esp_get_free_heap_size();
load_pattern(pattern_name);
size_t heap_after = esp_get_free_heap_size();
ESP_LOGI(TAG, "Pattern loaded. Heap: %zu -> %zu (delta: %zd)",
         heap_before, heap_after, (ssize_t)heap_after - (ssize_t)heap_before);
```

### 9.3 Escalation Path

**Level 1: Self-Resolution (0-2 failures)**
- Research the specific error
- Try documented solutions
- Test incrementally

**Level 2: Deep Research (2-3 failures)**
- Search ESP-IDF forums/GitHub issues
- Check ESP32 community resources
- Try alternative approaches

**Level 3: User Assistance (3+ failures)**
- 🛑 STOP work
- Document all attempts
- Present findings to user
- Request guidance

**Never Proceed Beyond Level 3 Without User Input**

---

## 10. Research-First Methodology

### 10.1 Why Research First?

**Critical Reasons:**

1. **ESP-IDF APIs Change Between Versions**
   - ESP-IDF v5.x has different APIs than v4.x
   - Always research current best practices

2. **FreeRTOS Patterns Vary by Platform**
   - ESP32 FreeRTOS has custom modifications
   - Standard FreeRTOS patterns may not work

3. **Hardware-Specific Constraints**
   - RMT peripheral has specific requirements
   - WiFi coexistence affects timing
   - DMA has alignment requirements

4. **Memory Management is Critical**
   - Wrong patterns cause fragmentation
   - One mistake can crash device after hours

5. **Real-Time Requirements**
   - 60 FPS LED output is non-negotiable
   - Wrong task priority breaks real-time

### 10.2 Research Process

**Step 1: Identify Unknowns**

```
Task: "Implement WebSocket TLV server"

Unknowns:
- Which ESP-IDF server APIs best support HTTPS + WebSocket?
- How to manage client session lifetimes and heartbeats?
- How to detect dropped connections promptly?
- What frame buffer sizes align with protocol constraints?
- How to validate TLV payloads efficiently?
```

**Step 2: Research Each Unknown**

```bash
# Query 1: API selection
brave-search: "ESP-IDF v5 esp_https_server websocket binary example"

# Query 2: Session + heartbeat strategy
brave-search: "ESP32 httpd_ws heartbeat keepalive design"

# Query 3: Connection monitoring
context7: "httpd_ws_send_frame_async usage disconnect detection"

# Query 4: Buffer sizing
brave-search: "ESP-IDF WebSocket frame buffer size 4096 best practice"

# Query 5: TLV parsing
context7: "ESP-IDF binary protocol validation techniques"
```

**Step 3: Synthesize Findings**

```
Research Summary:

1. API: Use esp_https_server with httpd_ws module (official support)
2. Session Life Cycle: Track clients + heartbeat timer per connection
3. Detection: Monitor httpd return codes + heartbeat ack
4. Buffers: 4KB static frame buffers satisfy WS_BUFFER_SIZE from spec
5. TLV Validation: Validate TYPE/LENGTH/CRC before dispatch

Approach:
- Create websocket_task on Core 1
- Register /ws route with httpd_ws_handler
- Implement heartbeat timer + stale client cleanup
- Allocate 4KB double-buffered frame workspace
- Validate TLV payloads before enqueuing commands
```

**Step 4: Document Plan**

```bash
task-master update-subtask --id=6.2 --prompt="
WebSocket Implementation Plan (post-research):

Research Findings:
- esp_https_server + httpd_ws support binary WebSocket on ESP32-S3
- TLV frames must be validated per websocket_protocol.md
- Keepalive handled via 10s heartbeat frames
- 4KB frame buffers align with WS_BUFFER_SIZE (spec section 3.4)
- Disconnect detection via HTTPD_WS_CLOSE_FRAME + event callbacks

Implementation:
1. Create network_tlv_server.c within components/network/
2. Implement websocket_task (Core 1, Priority 5) to manage sessions
3. Configure httpd_ssl_config_t with embedded certificate
4. Register /ws handler using httpd_register_uri_handler()
5. Add TLV validation + exponential reconnect for WiFi STA
6. Allocate 4KB static frame buffers (double-buffered)
7. Test disconnect/reconnect scenarios with binary TLV frames

Build verification required before commit.
"
```

### 10.3 Research Query Patterns

**Effective Queries:**

```bash
# ✅ GOOD: Specific version + feature
"ESP-IDF v5 WebSocket server binary TLV implementation"

# ✅ GOOD: Hardware + constraint + feature
"ESP32-S3 RMT peripheral 60fps WS2812B"

# ✅ GOOD: Platform + issue + solution
"FreeRTOS ESP32 heap fragmentation prevention"

# ✅ GOOD: API + version + example
"esp_wifi_init ESP-IDF v5 example code"

# ❌ BAD: Too generic
"websocket client"

# ❌ BAD: No version specified
"ESP32 wifi"

# ❌ BAD: No platform context
"LED control"
```

**Research Sources Priority:**

1. **Espressif Official Docs** (docs.espressif.com)
   - API references
   - Programming guides
   - Migration guides

2. **ESP-IDF GitHub** (github.com/espressif/esp-idf)
   - Example code
   - Issue discussions
   - Component code

3. **ESP32 Forums** (esp32.com)
   - Community solutions
   - Real-world issues
   - Debugging tips

4. **Technical Blogs** (Verified sources only)
   - Implementation guides
   - Performance tips
   - Best practices

---

## 11. Code Change Protocol

### 11.1 Pre-Change Checklist

**Before making ANY code changes:**

- [ ] Task details understood 100%
- [ ] Research completed for unknowns
- [ ] Implementation plan documented
- [ ] User approval if ANY uncertainty
- [ ] Existing code reviewed
- [ ] Dependencies identified
- [ ] Injection points mapped
- [ ] Build currently passing

**If ANY box unchecked → STOP and complete it first**

### 11.2 Code Change Process

**Step 1: Map Injection Points**

```c
// Document BEFORE changing:
/*
INJECTION POINTS FOR WIFI INITIALIZATION:

1. system_init() in main.c
   - Add call to network_init()
   - Check return value
   
2. Create network component
   - File: components/network/network.c
   - Function: esp_err_t network_init(void)
   
3. CMakeLists.txt dependencies
   - Add esp_wifi to REQUIRES
   - Add esp_netif to REQUIRES
   
4. Kconfig
   - Add WiFi SSID config
   - Add WiFi password config (encrypted)
*/
```

**Step 2: Identify Dependencies**

```c
// Document ALL dependencies:
/*
DEPENDENCIES FOR WIFI MODULE:

ESP-IDF Components:
- esp_wifi         (WiFi driver)
- esp_netif        (Network interface)
- esp_event        (Event loop)
- esp_https_server (HTTPS + WebSocket server)

Project Components:
- core             (For memory pools + init sequencing)

FreeRTOS:
- Event groups     (For connection sync)
- Mutexes          (If shared state)
*/
```

**Step 3: Create Component Structure**

```bash
# Create files first, then add code
mkdir -p firmware/components/network/include
touch firmware/components/network/network.c
touch firmware/components/network/include/network.h
touch firmware/components/network/CMakeLists.txt
```

**Step 4: Implement Header First**

```c
// network.h
#ifndef PRISM_NETWORK_MANAGER_H
#define PRISM_NETWORK_MANAGER_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize network subsystem
 * 
 * Initializes WiFi station mode, starts HTTPS/WebSocket servers, and
 * registers binary TLV handlers.
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t network_init(void);

/**
 * @brief Deinitialize network subsystem
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t network_deinit(void);

#ifdef __cplusplus
}
#endif

#endif // PRISM_NETWORK_MANAGER_H
```

**Step 5: Implement Source Incrementally**

```c
// network.c
#include "network.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_https_server.h"
#include "esp_log.h"

static const char *TAG = "network";

// Step 5a: Basic structure
esp_err_t network_init(void) {
    ESP_LOGI(TAG, "Initializing network...");
    
    // TODO: Implementation
    
    ESP_LOGI(TAG, "Network initialized");
    return ESP_OK;
}

esp_err_t network_deinit(void) {
    ESP_LOGI(TAG, "Deinitializing network...");
    
    // TODO: Implementation
    
    return ESP_OK;
}

// Step 5b: Add WiFi init
// Step 5c: Add event handlers
// Step 5d: Add connection logic
// ... (incremental implementation)
```

**Step 6: Build After Each Increment**

```bash
cd firmware
idf.py build

# ✅ If builds: Continue to next increment
# ❌ If fails: Fix immediately before proceeding
```

**Step 7: Test Incrementally**

```bash
# Flash to device
idf.py flash monitor

# Verify logs show expected behavior
# Check for errors/warnings
# Ctrl+] to exit monitor
```

### 11.3 Code Style Guidelines

**Naming Conventions:**

```c
// Components: <domain>/<feature>
network
storage
playback

// Functions: <component>_<action>_<object>
network_init()
storage_save_pattern()
playback_set_effect()

// Types: <name>_t
typedef struct {
    uint8_t r, g, b;
} rgb_color_t;

// Constants: UPPER_CASE
#define MAX_PATTERN_SIZE 1024
#define DEFAULT_TIMEOUT_MS 5000

// Static functions: lowercase with _
static esp_err_t connect_to_wifi(void) { ... }
static void event_handler(void *arg, ...) { ... }
```

**Error Handling:**

```c
// ✅ CORRECT: Always check return values
esp_err_t ret = esp_wifi_init(&cfg);
if (ret != ESP_OK) {
    ESP_LOGE(TAG, "WiFi init failed: %s", esp_err_to_name(ret));
    return ret;
}

// ✅ CORRECT: Use goto for cleanup
esp_err_t network_init(void) {
    esp_err_t ret = ESP_OK;
    
    ret = init_step_1();
    if (ret != ESP_OK) goto cleanup;
    
    ret = init_step_2();
    if (ret != ESP_OK) goto cleanup;
    
    return ESP_OK;
    
cleanup:
    // Cleanup code
    return ret;
}

// ❌ WRONG: Ignoring return values
esp_wifi_init(&cfg);  // NO ERROR CHECK!

// ❌ WRONG: Silent failures
if (ret != ESP_OK) {
    return ret;  // No logging!
}
```

**Memory Management:**

```c
// ✅ CORRECT: Static allocation
static uint8_t buffer[1024];

// ✅ CORRECT: One-time dynamic allocation
static void *global_buffer = NULL;

void init(void) {
    if (global_buffer == NULL) {
        global_buffer = heap_caps_malloc(size, MALLOC_CAP_DMA);
    }
}

// ✅ CORRECT: Immediate free after use (rare cases)
void *temp = malloc(size);
process(temp);
free(temp);

// ❌ WRONG: Repeated malloc/free
for (int i = 0; i < 1000; i++) {
    void *temp = malloc(100);  // FRAGMENTS HEAP!
    process(temp);
    free(temp);
}

// ❌ WRONG: No free (memory leak)
void *buffer = malloc(size);
// ... use buffer ...
// return; // LEAKED!
```

**Logging:**

```c
// ✅ CORRECT: Appropriate levels
ESP_LOGE(TAG, "Critical: %s", error);    // Errors
ESP_LOGW(TAG, "Warning: %s", warning);   // Warnings
ESP_LOGI(TAG, "Info: %s", info);         // Status updates
ESP_LOGD(TAG, "Debug: %s", debug);       // Debug info

// ✅ CORRECT: Structured messages
ESP_LOGI(TAG, "WiFi connected: SSID=%s, IP=%s", ssid, ip);

// ❌ WRONG: No context
ESP_LOGI(TAG, "Done");  // Done with what?

// ❌ WRONG: Wrong level
ESP_LOGE(TAG, "Task started");  // Not an error!
```

### 11.4 Post-Change Checklist

**After making code changes:**

- [ ] Code follows style guidelines
- [ ] All error paths handled
- [ ] Memory management correct (no leaks/fragments)
- [ ] Build verification passed
- [ ] No new compiler warnings
- [ ] Appropriate logging added
- [ ] Code commented where non-obvious
- [ ] CMakeLists.txt updated if new files
- [ ] Headers include guards present
- [ ] Task status updated

**If ALL boxes checked → Commit**  
**If ANY box unchecked → Fix before committing**

---

## 12. Testing & Verification

### 12.1 Build Testing

**Mandatory Build Tests:**

```bash
# Test 1: Clean build
cd firmware
idf.py fullclean
idf.py build

# Expected: "Project build complete."
# Time: ~2 minutes (full build)

# Test 2: Incremental build
# Make a small change
idf.py build

# Expected: "Project build complete."
# Time: ~10 seconds (incremental)

# Test 3: Flash size check
ls -lh build/*.bin

# Expected sizes:
# bootloader.bin:        ~30KB
# partition-table.bin:   ~3KB
# prism-firmware.bin:    <1MB (grows with features)

# Test 4: No warnings
idf.py build 2>&1 | grep -i warning

# Expected: No warnings (or only acceptable ones)
```

**Build Failure Analysis:**

```bash
# Capture full build output
idf.py build 2>&1 | tee build.log

# Search for errors
grep -i error build.log

# Common errors and fixes:
# "undefined reference" → Missing component in CMakeLists
# "No such file" → Missing include or wrong path
# "expected ';' before" → Syntax error
# "linker script" → Partition table or memory issue
```

### 12.2 Runtime Testing

**Flash and Monitor:**

```bash
# Flash firmware
idf.py flash

# Monitor serial output
idf.py monitor

# Expected output:
# - Boot messages
# - Component initialization logs
# - Task startup messages
# - No panic/abort/assert
# - Ctrl+] to exit
```

**Manual Testing Checklist:**

```
[ ] Device boots without panic
[ ] All tasks start successfully
[ ] WiFi connects (if implemented)
[ ] LEDs output (if implemented)
[ ] WebSocket connects (if implemented)
[ ] Patterns load (if implemented)
[ ] No watchdog resets
[ ] Stable for 5+ minutes
```

### 12.3 Performance Testing

**Memory Monitoring:**

```c
// Add to monitoring task
void monitoring_task(void *pvParameters) {
    while (1) {
        size_t free_heap = esp_get_free_heap_size();
        size_t min_heap = esp_get_minimum_free_heap_size();
        
        ESP_LOGI(TAG, "Heap: %zu free, %zu minimum", free_heap, min_heap);
        
        // Alert if low
        if (min_heap < 100 * 1024) {
            ESP_LOGW(TAG, "LOW HEAP WARNING: %zu bytes", min_heap);
        }
        
        vTaskDelay(pdMS_TO_TICKS(10000));  // Every 10s
    }
}
```

**Timing Monitoring:**

```c
// Add to playback task
void playback_task(void *pvParameters) {
    while (1) {
        int64_t frame_start = esp_timer_get_time();
        
        // Render frame
        render_led_frame();
        
        int64_t frame_duration = esp_timer_get_time() - frame_start;
        
        // Log if exceeds budget (16.67ms for 60fps)
        if (frame_duration > 16670) {
            ESP_LOGW(TAG, "Frame overrun: %lld us", frame_duration);
        }
        
        vTaskDelay(pdMS_TO_TICKS(16));  // 60 FPS
    }
}
```

### 12.4 Integration Testing

**Component Integration Tests:**

```
Test: Network + Storage
1. Connect to WiFi
2. Download pattern via WebSocket
3. Save pattern to LittleFS
4. Verify file exists
5. Load pattern from storage
Expected: Pattern matches downloaded data

Test: Storage + Playback
1. Load pattern from LittleFS
2. Send pattern to playback task
3. Verify LEDs display pattern
Expected: Visual output matches pattern

Test: Network + Playback
1. Receive TLV control command
2. Switch pattern
3. Measure latency
Expected: Pattern switch <100ms
```

---

## 13. Common Pitfalls & Solutions

### 13.1 ESP32-S3 Specific Pitfalls

**Pitfall 1: WiFi During Flash Operations**

```c
// ❌ PROBLEM: WiFi active during flash write
esp_wifi_start();
littlefs_write();  // Flash write while WiFi active → CRASH!

// ✅ SOLUTION: Disable WiFi during flash writes
esp_wifi_stop();
littlefs_write();
esp_wifi_start();
```

**Pitfall 2: DMA Buffer Alignment**

```c
// ❌ PROBLEM: Unaligned DMA buffer
uint8_t buffer[1024];  // Stack allocated, may not be aligned

// ✅ SOLUTION: Use heap_caps_malloc with alignment
uint8_t *buffer = heap_caps_aligned_alloc(4, 1024, MALLOC_CAP_DMA);
```

**Pitfall 3: RMT Timing Violations**

```c
// ❌ PROBLEM: Incorrect RMT timing for WS2812B
rmt_config.clk_div = 8;  // Wrong divider!

// ✅ SOLUTION: Calculate correct timing
// WS2812B: 800kHz, 1.25us period
// APB clock: 80MHz
// Divider: 80MHz / 800kHz = 100
rmt_config.clk_div = 100;
```

**Pitfall 4: Task Stack Overflow**

```c
// ❌ PROBLEM: Stack too small
xTaskCreate(network_task, "network", 2048, ...);  // Only 2KB!
// Task crashes with stack overflow

// ✅ SOLUTION: Increase stack size
xTaskCreate(network_task, "network", 8192, ...);  // 8KB
```

### 13.2 FreeRTOS Pitfalls

**Pitfall 5: Blocking in High-Priority Task**

```c
// ❌ PROBLEM: Blocking call in playback task (priority 10)
void playback_task(void *pvParameters) {
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));  // Blocks for 1 second!
        render_frame();
    }
}

// ✅ SOLUTION: Non-blocking timing
void playback_task(void *pvParameters) {
    TickType_t last_wake = xTaskGetTickCount();
    while (1) {
        render_frame();
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(16));  // 60 FPS
    }
}
```

**Pitfall 6: Mutex Deadlock**

```c
// ❌ PROBLEM: Mutex taken in wrong order
// Task A: lock1 → lock2
// Task B: lock2 → lock1  // DEADLOCK!

// ✅ SOLUTION: Always lock in same order
// Both tasks: lock1 → lock2
```

**Pitfall 7: Queue Full Not Handled**

```c
// ❌ PROBLEM: Ignoring queue full
xQueueSend(queue, &data, 0);  // May fail if full, no error check!

// ✅ SOLUTION: Handle queue full
if (xQueueSend(queue, &data, pdMS_TO_TICKS(100)) != pdTRUE) {
    ESP_LOGW(TAG, "Queue full, dropping message");
}
```

### 13.3 Memory Management Pitfalls

**Pitfall 8: Heap Fragmentation**

```c
// ❌ PROBLEM: Repeated malloc/free
for (int i = 0; i < 1000; i++) {
    void *buf = malloc(100);
    process(buf);
    free(buf);  // Fragments heap!
}

// ✅ SOLUTION: Allocate once, reuse
void *buf = malloc(100);
for (int i = 0; i < 1000; i++) {
    process(buf);
}
free(buf);
```

**Pitfall 9: Memory Leaks in Error Paths**

```c
// ❌ PROBLEM: Leak on error
esp_err_t func(void) {
    void *buf = malloc(1024);
    if (error_condition) {
        return ESP_FAIL;  // LEAKED!
    }
    free(buf);
    return ESP_OK;
}

// ✅ SOLUTION: Free before all return paths
esp_err_t func(void) {
    void *buf = malloc(1024);
    esp_err_t ret = ESP_OK;
    
    if (error_condition) {
        ret = ESP_FAIL;
        goto cleanup;
    }
    
    // ... work ...
    
cleanup:
    free(buf);
    return ret;
}
```

**Pitfall 10: Stack Allocation Too Large**

```c
// ❌ PROBLEM: Large stack array
void func(void) {
    uint8_t buffer[10000];  // 10KB on stack!
    // Stack overflow likely
}

// ✅ SOLUTION: Static or heap allocation
static uint8_t buffer[10000];  // Static
// OR
uint8_t *buffer = malloc(10000);  // Heap (but avoid repeated alloc!)
```

### 13.4 Build System Pitfalls

**Pitfall 11: Missing Component Dependencies**

```cmake
# ❌ PROBLEM: Missing dependencies in CMakeLists.txt
idf_component_register(
    SRCS "network.c"
    INCLUDE_DIRS "include"
)

# ✅ SOLUTION: Add all dependencies
idf_component_register(
    SRCS "network.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_wifi esp_netif esp_event nvs_flash
)
```

**Pitfall 12: Include Path Issues**

```c
// ❌ PROBLEM: Wrong include path
#include "network.h"  // Not found!

// ✅ SOLUTION: Verify include directory structure
components/network/
├── include/
│   └── network.h  // Must be in include/
└── network.c
```

---

## 14. Knowledge Resources

### 14.1 Project Documentation

**Must-Read Documents:**

```
Priority 1 (Read BEFORE coding):
├── .agent/instructions.md           ⭐⭐⭐⭐⭐ (30 min)
├── .agent/research-first.md         ⭐⭐⭐⭐⭐ (15 min)
├── .agent/workflow.md               ⭐⭐⭐⭐⭐ (20 min)
└── THIS DOCUMENT                    ⭐⭐⭐⭐⭐ (45 min)

Priority 2 (Read as needed):
├── .agent/multi-agent.md            ⭐⭐⭐⭐ (if multi-agent)
├── .agent/mcp-usage.md              ⭐⭐⭐⭐ (for tool reference)
├── .agent/taskmaster-reference.md   ⭐⭐⭐ (for commands)
└── .agent/cursor-workflow.md        ⭐⭐⭐ (if using Cursor)

Reference Documents:
├── MIGRATION.md                     ⭐⭐ (recent changes)
├── STATUS.md                        ⭐⭐ (current state)
└── DELIVERABLES.md                  ⭐⭐ (recent work)
```

**Project Files to Explore:**

```
Code Structure:
firmware/
├── main/main.c                      # Start here
├── components/core/                 # Core utilities (memory pools, safety)
└── CMakeLists.txt                   # Build config

Task Database:
.taskmaster/
├── tasks/tasks.json                 # All tasks
└── config.json                      # Taskmaster config

Documentation:
.agent/                              # All agent docs
docs/                                # Project docs
README.md                            # Project overview
```

### 14.2 ESP-IDF Resources

**Official Documentation:**

```
API Reference:
https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/

Key Sections:
- WiFi: api-reference/network/esp_wifi.html
- FreeRTOS: api-reference/system/freertos.html
- LittleFS: api-reference/storage/littlefs.html
- RMT: api-reference/peripherals/rmt.html

Programming Guides:
https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-guides/

Key Guides:
- Build System: build-system.html
- Error Handling: error-handling.html
- Memory Types: memory-types.html
- Thread Safety: thread-safety.html
```

**ESP-IDF GitHub:**

```
Examples:
https://github.com/espressif/esp-idf/tree/master/examples

Relevant Examples:
- WiFi: examples/wifi/
- WebSocket: examples/protocols/websocket/
- Storage: examples/storage/littlefs/
- RMT: examples/peripherals/rmt/
```

### 14.3 Community Resources

**ESP32 Forum:**
```
URL: https://esp32.com
Use For:
- Troubleshooting specific issues
- Community solutions
- Real-world experiences
```

**Reddit r/esp32:**
```
URL: reddit.com/r/esp32
Use For:
- Project ideas
- Community discussions
- Quick questions
```

**Stack Overflow:**
```
Tags: esp32, esp-idf, freertos
Use For:
- Specific technical questions
- Code issues
- Best practices
```

### 14.4 Search Strategies

**Effective Searches:**

```bash
# For API usage
"<API name> ESP-IDF v5 example"
Example: "esp_wifi_init ESP-IDF v5 example"

# For troubleshooting
"ESP32 <error message> solution"
Example: "ESP32 guru meditation error LoadProhibited solution"

# For patterns
"ESP32 FreeRTOS <pattern> example"
Example: "ESP32 FreeRTOS task queue communication example"

# For performance
"ESP32 <operation> optimization"
Example: "ESP32 RMT LED output 60fps optimization"
```

---

## 15. Handover Checklist

### 15.1 Pre-Work Checklist

**Complete BEFORE starting any work:**

- [ ] Read this ENTIRE handover document (45 min)
- [ ] Read `.agent/instructions.md` (30 min)
- [ ] Read `.agent/research-first.md` (15 min)
- [ ] Read `.agent/workflow.md` (20 min)
- [ ] Explore `firmware/` source code structure (30 min)
- [ ] Review `.taskmaster/tasks/tasks.json` (15 min)
- [ ] Run verification script: `./.taskmaster/scripts/verify-setup.sh` (5 min)
- [ ] Verify build passes: `cd firmware && idf.py build` (2 min)
- [ ] Understand ESP32-S3 constraints completely
- [ ] Understand multi-agent workflow (if applicable)

**Total Time: ~2.5 hours**

**Do NOT skip this checklist. Skipping will lead to:**
- Memory fragmentation (catastrophic)
- Build failures
- Wrong approach selection
- Wasted time fixing mistakes
- Violation of real-time constraints

### 15.2 Development Checklist

**For EACH task:**

```
[ ] Task details read completely
[ ] Unknowns identified
[ ] Research completed for unknowns
[ ] Implementation plan documented
[ ] User approval obtained (if ANY uncertainty)
[ ] Existing code reviewed
[ ] Dependencies mapped
[ ] Injection points identified
[ ] Code changes made incrementally
[ ] Build verified after each increment
[ ] Testing completed
[ ] Task status updated
[ ] Code committed (if build passes)
```

### 15.3 Stop Conditions Checklist

**STOP immediately if:**

```
[ ] ANY uncertainty about approach
[ ] Requirements unclear
[ ] Build fails after 2+ attempts
[ ] Runtime error not understood
[ ] Constraint violation detected
[ ] Lost context on project
[ ] 3+ consecutive failures
[ ] Research yields no clear solution
[ ] Unfamiliar API without documentation
[ ] Performance issue without clear cause
```

**When stopped:**

```
1. Document what you've done
2. Document what failed
3. Document what you've researched
4. Present findings to user
5. Request specific guidance
6. Wait for user response
```

### 15.4 Quality Checklist

**Before marking task complete:**

```
[ ] Build passes (cd firmware && idf.py build)
[ ] No new compiler warnings
[ ] Code follows style guidelines
[ ] All error paths handled
[ ] Memory management verified (no leaks)
[ ] Logging added appropriately
[ ] Comments added where non-obvious
[ ] Headers have include guards
[ ] CMakeLists.txt updated if needed
[ ] Testing completed (if applicable)
[ ] Performance within limits
[ ] Task documented via update-subtask
```

---

## 16. Final Instructions

### 16.1 Your Mission

You are now responsible for implementing production-grade ESP32-S3 firmware for the PRISM.k1 LED controller. This is NOT a prototype - it's production firmware that MUST work reliably in the field.

**Success Criteria:**
- ✅ 60 FPS LED output (16.67ms frame time)
- ✅ Pattern switch <100ms (p95)
- ✅ Memory usage <150KB heap
- ✅ Zero heap fragmentation
- ✅ Network reconnection without restart
- ✅ Multi-month uptime reliability
- ✅ EVERY commit builds successfully

### 16.2 Your First Steps

**Step 1: Verification (5 minutes)**

```bash
cd /Users/spectrasynq/Workspace_Management/Software/PRISM.k1

# Run verification script
./.taskmaster/scripts/verify-setup.sh

# Expected: ✓ ALL CRITICAL TESTS PASSED
# If any failures: Fix them before proceeding
```

**Step 2: Get Your First Task (5 minutes)**

```bash
# If single-agent workflow
task-master next

# If multi-agent workflow (check agent assignment)
task-master use-tag <your-tag>
task-master next --tag=<your-tag>

# View task details
task-master show <task-id>
```

**Step 3: Research Phase (15-60 minutes)**

```bash
# Research unknowns
brave-search: "<specific technical query>"
context7: "<ESP-IDF API documentation query>"

# Document findings
task-master update-subtask --id=<task-id> --prompt="
Research Findings:
- Finding 1
- Finding 2
- Finding 3

Implementation Plan:
1. Step 1
2. Step 2
3. Step 3
"
```

**Step 4: Implementation (varies)**

```bash
# Set status
task-master set-status --id=<task-id> --status=in-progress

# Make code changes incrementally
# Build after each change
cd firmware && idf.py build

# Test if possible
idf.py flash monitor

# Document progress
task-master update-subtask --id=<task-id> --prompt="
Progress Update:
- Completed step 1
- Working on step 2
- Issue encountered: <description>
"
```

**Step 5: Completion (5 minutes)**

```bash
# Final build verification
cd firmware && idf.py fullclean && idf.py build

# Commit if passes
git add .
git commit -m "feat(component): description (task X.X)"

# Update status
task-master set-status --id=<task-id> --status=done

# Get next task
task-master next
```

### 16.3 Remember

**ALWAYS:**
- ✅ Research before implementing
- ✅ Build after every code change
- ✅ Document your work via update-subtask
- ✅ Check memory usage
- ✅ Handle all error paths
- ✅ Stop when uncertain

**NEVER:**
- ❌ Skip research phase
- ❌ Ignore build warnings
- ❌ Commit without build verification
- ❌ Use malloc/free in loops
- ❌ Make assumptions
- ❌ Proceed when uncertain

### 16.4 Contact

**When you need help:**

1. Document your situation clearly
2. Show what you've tried
3. Explain why you're stuck
4. Present possible paths forward
5. Request specific guidance

**User will provide:**
- Clarification on requirements
- Guidance on approach selection
- Help with blockers
- Architecture decisions
- Priority changes

---

## 17. Context Transfer Complete

**You now have:**
- ✅ Complete project context
- ✅ Technical architecture understanding
- ✅ Development protocols
- ✅ Error handling procedures
- ✅ Research methodology
- ✅ Code change protocols
- ✅ Testing procedures
- ✅ Tool usage guidelines
- ✅ Common pitfalls knowledge
- ✅ Resource references

**Estimated reading time: 45 minutes**  
**Estimated exploration time: 60 minutes**  
**Total onboarding time: ~2 hours**

**This investment will:**
- Save 10+ hours of mistakes
- Prevent memory fragmentation issues
- Ensure build stability
- Maintain code quality
- Achieve performance targets
- Enable field reliability

---

## 🚀 You Are Ready

**Your mission begins now. Follow the protocols. Research first. Build often. Stop when uncertain.**

**Questions before starting?** → Ask now.  
**Ready to begin?** → Start with Step 1: Verification.  
**Uncertain about anything?** → Request clarification.

**Remember: This is production firmware. Every decision matters. Take your time. Do it right.**

---

**Handover Date:** October 15, 2025  
**Handover By:** Previous Agent  
**Handover To:** You (Next Agent)  
**Project:** PRISM.k1 ESP32-S3 Firmware  
**Status:** Ready for Implementation  

**Good luck, Agent. Build something great. 🎯**
