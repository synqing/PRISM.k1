# PRISM.k1 Development Workflow

**Purpose:** Standard operating procedures for task execution and quality assurance.

---

## 🔄 Standard Task Execution Cycle

### Overview

Each task follows a **5-phase cycle** designed to ensure quality, prevent rework, and maintain architectural consistency:

```
Research → Planning → Implementation → Verification → Completion
(10-30min)  (5-10min)   (1-4 hours)    (15-30min)     (5min)
```

---

## Phase 1: Research & Discovery (10-30 minutes)

### Step 1.1: Claim Task

```bash
# Get next available task
TASK_ID=$(task-master next --tag=<your-tag> | grep -oP 'Task \K[\d.]+' | head -1)

# View full details
task-master show --tag=<your-tag> --id=$TASK_ID
```

**Output Checklist:**
- ✅ Task title and description clear
- ✅ Dependencies completed or available
- ✅ Acceptance criteria understood
- ✅ Test strategy defined

### Step 1.2: Conduct Research (MANDATORY)

**For Hardware/Platform Topics:**
```bash
# Query Context7 for ESP-IDF documentation
# Use MCP tool: resolve-library-id, then get-library-docs
# Example: Get ESP-IDF WebSocket documentation

# Query Brave Search for recent best practices
task-master research \
  --task-ids="$TASK_ID" \
  --tag="<your-tag>" \
  --query="ESP-IDF 5.x <specific-topic> best practices 2024-2025" \
  --save-to="$TASK_ID" \
  --detail=high
```

**For Algorithm/Design Topics:**
```bash
# Research patterns and common solutions
task-master research \
  --task-ids="$TASK_ID" \
  --query="<algorithm-name> implementation patterns embedded systems" \
  --save-to="$TASK_ID"
```

**Research Checklist:**
- ✅ Current ESP-IDF v5.x patterns identified
- ✅ Memory implications understood
- ✅ Performance characteristics known
- ✅ Common pitfalls documented
- ✅ Error handling patterns identified

### Step 1.3: Document Research Findings

```bash
task-master update-subtask \
  --tag="<your-tag>" \
  --id="$TASK_ID" \
  --prompt="Research Phase Complete

Key Findings:
- ESP-IDF pattern: <specific-approach>
- Memory impact: <heap-estimate>
- Performance: <timing-characteristics>
- Common pitfalls: <issues-to-avoid>

Recommended Approach:
<brief-implementation-plan>

References:
- Context7: <doc-references>
- Brave Search: <key-articles>
"
```

**DO NOT proceed to implementation until research is documented.**

---

## Phase 2: Planning & Design (5-10 minutes)

### Step 2.1: Identify Files to Modify

```bash
# Locate relevant source files
find firmware/components -name "*.c" -o -name "*.h" | grep <component>

# Read existing related code
# Use filesystem MCP to read multiple files at once
```

**File Organization:**
```
firmware/components/<component>/
├── include/<component>.h    # Public API
├── src/<component>.c         # Implementation
└── CMakeLists.txt           # Build config
```

### Step 2.2: Design Implementation Plan

**Planning Template:**
```markdown
## Implementation Plan for Task $TASK_ID

### Files to Create/Modify:
- `firmware/components/<component>/include/<header>.h` - API
- `firmware/components/<component>/src/<source>.c` - Implementation
- `firmware/components/<component>/CMakeLists.txt` - Build config

### Memory Allocation Strategy:
- Stack allocation for <data> (<size>KB)
- Heap allocation for <dynamic-data> via heap_caps_malloc()
- No allocations in hot path (60 FPS loop)

### Error Handling:
- Return esp_err_t for all functions
- Check dependencies with ESP_ERROR_CHECK()
- Graceful degradation on <specific-errors>

### Testing Strategy:
- Unit test: <test-cases>
- Integration test: <interaction-scenarios>
- Hardware test: <on-device-verification>

### Estimated Heap Impact:
- Static: <bytes>
- Dynamic: <bytes>
- Peak: <bytes>
```

### Step 2.3: Log Plan to Task

```bash
task-master update-subtask \
  --tag="<your-tag>" \
  --id="$TASK_ID" \
  --prompt="Implementation Plan Defined

Files: <list>
Memory Strategy: <approach>
Error Handling: <pattern>
Testing: <strategy>

Ready to implement."
```

---

## Phase 3: Implementation (1-4 hours)

### Step 3.1: Set Status to In-Progress

```bash
task-master set-status \
  --tag="<your-tag>" \
  --id="$TASK_ID" \
  --status=in-progress
```

### Step 3.2: Iterative Development Loop

**Micro-Cycle (15-30 minutes per iteration):**

```bash
# 1. Write code for one logical unit
# Use filesystem MCP for file operations

# 2. Build and verify
cd firmware
idf.py build

# 3. If build fails, fix immediately
# Never accumulate build errors

# 4. Log progress
task-master update-subtask \
  --tag="<your-tag>" \
  --id="$TASK_ID" \
  --prompt="Implemented <feature>. Build: ✅. Next: <next-step>"

# 5. Repeat for next unit
```

**Code Quality Standards:**

```c
// ✅ GOOD: Clear names, error handling
esp_err_t prism_storage_write(const char *path, const void *data, size_t len) {
    if (!path || !data || len == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    
    FILE *f = fopen(path, "wb");
    if (!f) {
        ESP_LOGE(TAG, "Failed to open %s: %s", path, strerror(errno));
        return ESP_FAIL;
    }
    
    size_t written = fwrite(data, 1, len, f);
    fclose(f);
    
    return (written == len) ? ESP_OK : ESP_FAIL;
}

// ❌ BAD: No error handling, unclear names
int w(char *p, void *d, int l) {
    FILE *f = fopen(p, "wb");
    fwrite(d, 1, l, f);
    fclose(f);
    return 0;
}
```

**Memory Management Rules:**

```c
// ✅ GOOD: heap_caps_malloc with capability
uint8_t *buffer = heap_caps_malloc(size, MALLOC_CAP_DMA);
if (!buffer) {
    ESP_LOGE(TAG, "Failed to allocate DMA buffer");
    return ESP_ERR_NO_MEM;
}
// ... use buffer ...
free(buffer);

// ✅ GOOD: Stack allocation for small, short-lived data
uint8_t header[32];
parse_header(data, header, sizeof(header));

// ❌ BAD: malloc/free in 60 FPS loop
void render_frame() {
    uint8_t *frame = malloc(LED_COUNT * 3);  // FRAGMENTATION RISK
    // ... render ...
    free(frame);
}

// ✅ GOOD: Pre-allocated buffer for hot path
static uint8_t frame_buffer[LED_COUNT * 3];
void render_frame() {
    // ... render into frame_buffer ...
}
```

### Step 3.3: Incremental Progress Logging

**Log every 30-60 minutes:**
```bash
task-master update-subtask \
  --tag="<your-tag>" \
  --id="$TASK_ID" \
  --prompt="Progress Update: <time>

Completed:
- <feature-1> implemented ✅
- <feature-2> implemented ✅

Current:
- Working on <feature-3>

Challenges:
- <issue-if-any> - solved via <solution>

Build: Passing
Heap: +<delta>KB (within budget)"
```

---

## Phase 4: Verification (15-30 minutes)

### Step 4.1: Build Verification (MANDATORY)

```bash
cd firmware

# Clean build
idf.py fullclean
idf.py build

# Verify zero errors
echo $?  # Must be 0

# Check binary size
idf.py size-components
```

**Build Failure Protocol:**
```bash
# If build fails:
# 1. Capture full error
idf.py build 2>&1 | tee build-error.log

# 2. Research the error
task-master research \
  --query="ESP-IDF build error <error-code>" \
  --save-to="$TASK_ID"

# 3. Fix based on research
# 4. Verify fix works
# 5. Document resolution

task-master update-subtask --id="$TASK_ID" \
  --prompt="Build issue resolved: <error> fixed via <solution>"
```

### Step 4.2: Memory Profiling

```bash
# Component memory usage
idf.py size-components | grep <your-component>

# Heap usage check (if applicable)
# Add to your code temporarily:
ESP_LOGI(TAG, "Free heap: %lu, min: %lu", 
         esp_get_free_heap_size(),
         esp_get_minimum_free_heap_size());
```

**Memory Budget Validation:**
- Total heap < 150KB in use
- Minimum free heap > 50KB
- Largest free block > 32KB (no fragmentation)

### Step 4.3: Functional Testing

**Unit Test Template:**
```c
// firmware/components/<component>/test/test_<component>.c
#include "unity.h"
#include "<component>.h"

void test_<feature>_success(void) {
    esp_err_t err = <function>(valid_args);
    TEST_ASSERT_EQUAL(ESP_OK, err);
}

void test_<feature>_invalid_args(void) {
    esp_err_t err = <function>(NULL);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, err);
}

void app_main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_<feature>_success);
    RUN_TEST(test_<feature>_invalid_args);
    UNITY_END();
}
```

**Run tests:**
```bash
cd firmware/components/<component>
idf.py -C test build
idf.py -C test flash monitor
```

### Step 4.4: Integration Verification

**Test with dependent components:**
```bash
# If your component is used by main
cd firmware
idf.py build flash monitor

# Verify behavior in system context
# Check logs for errors/warnings
```

### Step 4.5: Documentation Update

```bash
task-master update-subtask \
  --tag="<your-tag>" \
  --id="$TASK_ID" \
  --prompt="Verification Complete

Build: ✅ Passing (fullclean + build)
Memory: ✅ <usage>KB (<budget>KB budget)
  - Free heap: <X>KB
  - Min heap: <Y>KB  
  - Largest block: <Z>KB
Tests: ✅ <N> unit tests passing
Integration: ✅ Verified with <dependencies>

Implementation Details:
- Files created: <list>
- Files modified: <list>
- API changes: <none|list>
- Known limitations: <any>

Ready for review."
```

---

## Phase 5: Completion (5 minutes)

### Step 5.1: Final Validation

**Pre-Completion Checklist:**
- ✅ Build passes: `idf.py build` succeeds
- ✅ Tests pass: Unit/integration tests verified
- ✅ Memory safe: Within budget, no fragmentation
- ✅ Code quality: Error handling, clear names
- ✅ Documentation: All updates logged to task

### Step 5.2: Mark Task Complete

```bash
task-master set-status \
  --tag="<your-tag>" \
  --id="$TASK_ID" \
  --status=done
```

### Step 5.3: Git Commit (Optional but Recommended)

```bash
cd /Users/spectrasynq/Workspace_Management/Software/PRISM.k1

git add firmware/components/<component>/
git commit -m "feat: implement <feature> (task $TASK_ID)

- <key-change-1>
- <key-change-2>

Tests: <N> passing
Memory: <usage>KB
Build: verified"
```

### Step 5.4: Move to Next Task

```bash
# Get next task
task-master next --tag=<your-tag>

# Start cycle again from Phase 1
```

---

## 🚨 Exception Handling

### Blocked by Dependency

**When a dependency task is not complete:**

```bash
# Check dependency status
task-master show --tag=<dep-tag> --id=<dep-id>

# If pending, two options:

# Option 1: Work on dependency first (if in your domain)
task-master next --tag=<dep-tag>

# Option 2: Request cross-agent help
task-master update-subtask --tag=<your-tag> --id=$TASK_ID \
  --prompt="BLOCKED: Waiting for task <dep-id> (@agent-<domain>)
  
Need: <specific-requirement>
Impact: Cannot proceed with <blocked-feature>
Timeline: <urgency>"

# Set status to blocked
task-master set-status --tag=<your-tag> --id=$TASK_ID --status=blocked

# Work on different task
task-master next --tag=<your-tag>
```

### Technical Blocker

**When stuck >30 minutes on implementation:**

```bash
# 1. Research deeper
task-master research \
  --query="<specific-problem> ESP32-S3 solution" \
  --save-to="$TASK_ID" \
  --detail=high

# 2. Use sequential-thinking for complex problems
# MCP tool: sequential-thinking

# 3. If still stuck after 1 hour, escalate
task-master update-subtask --id="$TASK_ID" \
  --prompt="Technical Blocker

Problem: <clear-description>
Attempted Solutions:
1. <approach-1> - <result>
2. <approach-2> - <result>
Research: <findings>

Need: <specific-help-needed>
Status: BLOCKED"

task-master set-status --id=$TASK_ID --status=blocked
```

### Build Regression

**When changes break previously working build:**

```bash
# 1. Immediate rollback
git diff HEAD  # Review changes
git checkout -- <problematic-files>

# 2. Isolate issue
# Re-apply changes incrementally
# Test build after each change

# 3. Document issue
task-master update-subtask --id=$TASK_ID \
  --prompt="Build regression detected.
  
Original error: <error>
Root cause: <cause>
Resolution: <fix-applied>

Lesson learned: <what-to-avoid-next-time>"
```

---

## 📊 Quality Metrics

### Per-Task Targets

| Metric | Target | Verification |
|--------|--------|--------------|
| Build Success | 100% | `idf.py build` |
| Test Pass Rate | 100% | Unit + integration tests |
| Memory Budget | <150KB | `size-components` |
| Heap Fragmentation | 0% | Largest block >32KB |
| Implementation Time | 1-4 hours | Time tracking |
| Rework Rate | <10% | Rare status=blocked |

### Per-Component Targets

| Metric | Target | Verification |
|--------|--------|--------------|
| Code Coverage | >80% | Test coverage report |
| API Documentation | 100% | Doxygen comments |
| Error Handling | 100% | All esp_err_t checked |
| Memory Leaks | 0 | Valgrind or manual audit |

---

## 🔄 Continuous Improvement

### After Each Task

**Self-Reflection:**
- What went well?
- What slowed me down?
- What would I do differently?
- What new pattern can be reused?

**Update Workflow:**
```bash
# If you discover a better approach
# Update .agent/workflow.md with improvements
# Share with other agents via git commit
```

### Weekly Review

**Team Sync (if applicable):**
- Review completed tasks
- Identify bottlenecks
- Share learnings
- Update quality gates if needed

---

## 🎯 Success Indicators

**You're doing it right when:**
- ✅ Build passes on first try 80%+ of the time
- ✅ Research prevents major rework
- ✅ Memory usage stays within budget
- ✅ Task progress logged regularly
- ✅ Blocked tasks <5% of total
- ✅ Code reviews find minor issues only

**Red flags:**
- ❌ Frequent build failures after changes
- ❌ Memory usage creeping above budget
- ❌ Long gaps in progress logging
- ❌ Many tasks stuck in blocked state
- ❌ Skipping research phase
- ❌ No test verification before completion

---

**Remember:** Quality over speed. A task done right once is faster than a task done twice.

---

**Next:** See [multi-agent.md](./multi-agent.md) for coordination patterns.

**Version:** 1.0  
**Last Updated:** 2025-10-15
