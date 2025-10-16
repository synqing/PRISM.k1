# PRISM.k1 Agent Instructions v1.0

**Auto-loaded by:** Claude Desktop, Claude Chat, Cursor, VSCode (via CLAUDE.md/AGENT.md)

---

## 🎯 Primary Mission

Build production-ready ESP32-S3 firmware for PRISM LED controller supporting:
- **60-second setup** from unboxing to first pattern
- **15-20 template patterns** pre-loaded and ready
- **WebSocket binary protocol** for real-time control
- **<100ms pattern switching** via structural efficiency
- **60 FPS LED output** with zero flicker
- **<150KB heap usage** with zero fragmentation after 24h runtime

---

## 🏗️ Project Context

### Hardware Platform
- **MCU:** ESP32-S3 (dual-core 240MHz Xtensa LX7)
- **RAM:** 512KB total (300KB usable after OS/system)
- **Flash:** 8MB (6.5MB for firmware + storage)
- **LED Output:** WS2812B protocol via RMT peripheral
- **Connectivity:** WiFi 802.11 b/g/n, AP + STA mode

### Technology Stack
- **Framework:** ESP-IDF v5.x
- **RTOS:** FreeRTOS (included in ESP-IDF)
- **Filesystem:** LittleFS (1.47MB partition)
- **Protocol:** WebSocket binary with TLV encoding
- **Build System:** CMake + ESP-IDF build system

### Critical Constraints
1. **Memory Safety:** ESP32 heap fragmentation WILL brick devices in production
2. **Real-Time Performance:** 60 FPS LED output is non-negotiable
3. **Pattern Switch Speed:** <100ms requirement for user experience
4. **Storage Efficiency:** Fit 25-35 patterns in 1.47MB via structural optimization
5. **Field Reliability:** Zero tolerance for crashes, must recover gracefully

---

## 🔬 Research-First Methodology (MANDATORY)

**THIS IS NOT OPTIONAL. THIS IS SURVIVAL.**

### Why Research-First?

ESP32 firmware is **unforgiving:**
- Wrong memory allocation pattern → Production devices brick after 24h
- Wrong WebSocket buffer size → Uploads fail unpredictably
- Wrong compression choice → Flash wear-out in months
- Wrong RMT timing → Flickering LEDs or dropped frames

**K1.juce lesson:** 5-day research phase prevented critical architectural mistakes.  
**PRISM.k1:** 2-3 day focused research prevents field disasters.

### Enforcement Protocol

**BEFORE implementing ANY task, you MUST:**

1. **Research Phase** (10-30 minutes)
   ```bash
   # Use Context7 for ESP-IDF/FreeRTOS documentation
   # Use Brave Search for current best practices
   task-master research \
     --task-ids="<current-task-id>" \
     --tag="<current-tag>" \
     --query="ESP-IDF 5.x best practices for <task-topic>" \
     --save-to="<current-task-id>"
   ```

2. **Document Findings**
   ```bash
   task-master update-subtask \
     --tag="<current-tag>" \
     --id="<current-task-id>" \
     --prompt="Research completed. Key findings: [summarize constraints, recommended approach, common pitfalls]"
   ```

3. **Validate Approach**
   - Does this fit in memory budget?
   - Will this cause fragmentation?
   - Is this the current ESP-IDF v5.x pattern?
   - What are the failure modes?

4. **ONLY THEN** proceed to implementation

### Research Topics by Domain

**Network Tasks (3-11, 15-17):**
- ESP-IDF WebSocket server implementation patterns
- Binary protocol design for embedded systems
- WiFi reconnection and stability best practices
- mDNS reliability on ESP32-S3

**Storage Tasks (4, 7-8, 29-30):**
- LittleFS on ESP32: atomic operations, wear leveling
- Pattern file format: structural vs compression efficiency
- Flash partition layout optimization
- Power-loss recovery mechanisms

**Playback Tasks (18-22):**
- ESP32 RMT peripheral for WS2812B at 60 FPS
- FreeRTOS real-time scheduling and priority management
- Double-buffering techniques for LED output
- Fixed-point math for animation interpolation

**Template Tasks (46-53):**
- LED effect algorithm design patterns
- Color palette optimization strategies
- Animation curve mathematics (ease-in/out, bezier)

**Consequences of Skipping Research:**
- 30-40% rework rate (vs <10% with research)
- Architectural inconsistencies across modules
- Production bugs discovered post-deployment
- Customer returns and RMAs

---

## 🤝 Multi-Agent Coordination

### Tagged Work Streams

Agents work in **isolated contexts** to prevent conflicts:

| Tag | Domain | Agent Responsibility |
|-----|--------|---------------------|
| **network** | WebSocket, WiFi, protocols | Agent 1: Network Specialist |
| **storage** | LittleFS, patterns, caching | Agent 2: Storage Specialist |
| **playback** | LED driver, effects, animation | Agent 3: Playback Specialist |
| **templates** | Template patterns, deployment | Agent 4: Template Designer |
| **integration** | Testing, debugging, optimization | Agent 5: Integration Lead |

### Task Claiming Protocol

**DO NOT manually grab tasks.** Use atomic allocation:

```bash
# Claim next available task in your tag
TASK=$(task-master next --tag=<your-tag>)

# Mark as in-progress
task-master set-status --tag=<your-tag> --id=$TASK --status=in-progress

# Work on task...

# Release when done
task-master set-status --tag=<your-tag> --id=$TASK --status=done
```

### Cross-Agent Communication

When you need info from another domain:

```bash
# Block your task with specific request
task-master update-subtask --tag=network --id=12.3 \
  --prompt="BLOCKED: Need prism_fs_write() API signature from @agent-2-storage (task 4.2)"

# Other agent responds in their subtask
task-master update-subtask --tag=storage --id=4.2 \
  --prompt="@agent-1-network: API signature: esp_err_t prism_fs_write(const char* path, const void* data, size_t len, bool atomic)"

# Unblock and continue
task-master set-status --tag=network --id=12.3 --status=in-progress
```

### Stale Task Recovery

If a task claim is >2 hours old, assume abandoned:
```bash
# Health monitor will auto-recover stale tasks
# Manual recovery if needed:
task-master set-status --tag=<tag> --id=<stale-task> --status=pending
```

**See [multi-agent.md](./multi-agent.md) for detailed coordination patterns.**

---

## 🔧 MCP Tool Usage Priority

### Essential Tools (Use Frequently)

**1. task-master-ai (Project Management)**
- `next_task` - Get next available task
- `get_task` - View task details
- `update_subtask` - Log implementation progress
- `research` - Research before implementing
- `set_task_status` - Update task status

**2. filesystem (Code Operations)**
- Read firmware source files
- Write implementation code
- Batch read multiple files
- Edit specific code sections

**3. context7 (Documentation)**
- `resolve-library-id` - Find ESP-IDF library ID
- `get-library-docs` - Fetch ESP-IDF v5.x, FreeRTOS docs
- Use for: API signatures, best practices, examples

**4. brave-search (Research)**
- Current best practices (post-training-cutoff)
- ESP32 community solutions
- Performance benchmarks
- Common pitfall warnings

**5. sequential-thinking (Complex Problems)**
- Architecture decisions
- Debugging complex issues
- Trade-off analysis
- Design pattern selection

**See [mcp-usage.md](./mcp-usage.md) for complete tool reference.**

---

## ✅ Quality Gates (MANDATORY)

### Per-Task Quality Checklist

**BEFORE marking task as `done`, verify:**

1. **Build Verification**
   ```bash
   cd firmware
   idf.py build
   # Must succeed with zero errors
   ```

2. **Memory Check**
   - Heap usage logged via `esp_get_free_heap_size()`
   - No allocations in hot paths (>60 FPS loop)
   - Fixed-size allocations preferred over malloc/free

3. **Code Quality**
   - Clear variable names (but concise: `i` for index is fine)
   - Error handling for ALL esp_err_t returns
   - Comments for non-obvious logic only

4. **Documentation**
   ```bash
   task-master update-subtask --id=<current> \
     --prompt="Implementation complete. Approach: [brief]. 
     Challenges: [any issues]. Memory impact: [heap delta]. 
     Build: verified."
   ```

5. **Test Strategy**
   - Unit test for isolated modules
   - Integration test for component interactions
   - Hardware test for RMT/WiFi/Flash operations

### Build Failure Protocol

**If build fails after your changes:**

1. **Immediate Rollback**
   ```bash
   git diff HEAD  # Review changes
   git checkout -- <problematic-files>  # Rollback
   ```

2. **Research the Error**
   ```bash
   task-master research \
     --query="ESP-IDF build error: <error-message>" \
     --save-to="<current-task>"
   ```

3. **Update Task**
   ```bash
   task-master update-subtask --id=<current> \
     --prompt="Build failure encountered: <error>. 
     Rolled back. Researching solution..."
   ```

4. **Fix and Verify**
   - Implement fix based on research
   - Verify build succeeds
   - Document what was fixed

**NEVER leave project in non-building state.**

---

## 📋 Development Workflow

### Standard Task Execution Loop

**Phase 1: Research (10-30 min)**
1. Get task via `task-master next --tag=<your-tag>`
2. Review task details via `task-master show --id=<task-id>`
3. **Research FIRST** (Context7 + Brave Search)
4. Document findings via `update-subtask`

**Phase 2: Planning (5-10 min)**
5. Identify files to modify
6. Plan implementation approach
7. Note memory/performance implications
8. Update task with plan

**Phase 3: Implementation (1-4 hours)**
9. Set status to `in-progress`
10. Write code following research findings
11. Use descriptive variable names
12. Handle ALL error cases
13. Log progress via `update-subtask` as you go

**Phase 4: Verification (15-30 min)**
14. Verify build: `cd firmware && idf.py build`
15. Check heap usage if applicable
16. Test on hardware if available
17. Document completion via `update-subtask`

**Phase 5: Completion (5 min)**
18. Final build verification
19. Set status to `done`
20. Move to next task

**See [workflow.md](./workflow.md) for detailed workflow patterns.**

---

## 🚨 Error Handling & Recovery

### Common Issues & Solutions

**Issue: Task Blocked on Dependency**
```bash
# Check dependency status
task-master show --id=<dependency-id>

# If dependency is pending, consider working on it first
# Or communicate with responsible agent via update-subtask
```

**Issue: Build Errors After Changes**
```bash
# Get full error output
cd firmware
idf.py build 2>&1 | tee build-error.log

# Research the specific error
task-master research \
  --query="ESP-IDF <specific-error-code>" \
  --save-file

# Fix based on research, verify, document
```

**Issue: Memory Budget Exceeded**
```bash
# Profile memory usage
idf.py size-components

# Research optimization strategies
task-master research \
  --query="ESP32 memory optimization techniques" \
  --save-to="<current-task>"

# Refactor using research findings
```

**Issue: Stale Documentation**
```bash
# Use Context7 for latest docs
# Context7 pulls real-time docs, not training data

# Or use Brave Search for very recent (2024-2025) info
```

### Escalation Path

**Level 1: Self-Research (10-30 min)**
- Use Context7, Brave Search, sequential-thinking
- Consult existing firmware code
- Check ESP-IDF examples

**Level 2: Cross-Agent Consultation (30-60 min)**
- Block task with specific question
- Tag responsible agent domain
- Wait for response via update-subtask

**Level 3: Human Escalation (>60 min blocked)**
- Document all research attempted
- Summarize the blocker clearly
- Provide context for human developer

**DO NOT spend >2 hours stuck without escalating.**

---

## 📚 Resource Quick Reference

### File Locations

```
Project Root: /Users/spectrasynq/Workspace_Management/Software/PRISM.k1

Key Files:
├── firmware/                     # ESP32 firmware codebase
│   ├── main/main.c               # Entry point
│   ├── components/               # Reusable modules
│   │   ├── core/                 # System init, memory, errors
│   │   ├── network/              # WebSocket, WiFi, protocols
│   │   ├── storage/              # LittleFS, patterns, cache
│   │   ├── playback/             # LED driver, effects
│   │   └── templates/            # Built-in patterns
│   ├── CMakeLists.txt            # Build configuration
│   ├── partitions.csv            # Flash layout
│   └── sdkconfig.defaults        # ESP-IDF config
│
├── .taskmaster/
│   ├── tasks/tasks.json          # Task database
│   ├── config.json               # AI model config
│   └── docs/
│       ├── README.md              # ⭐ SOURCE OF TRUTH INDEX
│       ├── CANON.md                              # ⭐ SINGLE SOURCE OF TRUTH (auto-generated)
│       ├── prism-firmware-prd.txt # Product requirements
│       └── research/              # Evidence & analysis
│
└── .agent/                       # THIS DIRECTORY
    ├── instructions.md           # YOU ARE HERE
    ├── workflow.md               # Workflow details
    ├── multi-agent.md            # Coordination
    ├── research-first.md         # Research methodology
    └── mcp-usage.md              # Tool reference
```

**⚠️ CRITICAL:** Always reference `.taskmaster/CANON.md` for technical decisions! (Auto-generated from approved ADRs)

### External Documentation

**ESP-IDF v5.x:**
- Use Context7: `/espressif/esp-idf/v5.3`
- Official: https://docs.espressif.com/projects/esp-idf/en/v5.3/

**FreeRTOS:**
- Use Context7: `/freertos/freertos-kernel`
- Official: https://www.freertos.org/

**LittleFS:**
- Use Context7: `/littlefs-project/littlefs`
- GitHub: https://github.com/littlefs-project/littlefs

**ESP32 Community:**
- ESP32 Forum: https://esp32.com/
- Brave Search: "esp32-s3 [your-topic] best practices 2024"

---

## 🎓 Best Practices

### Memory Management
- Use `heap_caps_malloc()` with specific capabilities
- Prefer stack allocation for small, short-lived data
- Use memory pools for fixed-size objects
- Monitor fragmentation: `heap_caps_get_largest_free_block()`
- Set alert threshold: <50KB free heap

### Real-Time Performance
- Use hardware timers for 60 FPS timing
- Priority: Playback task > Network task > Storage task
- Minimize ISR execution time (<10μs)
- Use DMA for LED output via RMT
- Avoid malloc/free in hot paths

### Error Handling
- Check ALL `esp_err_t` return values
- Use `ESP_ERROR_CHECK()` for critical operations
- Log errors with context: `ESP_LOGE(TAG, "msg: %s", esp_err_to_name(err))`
- Implement graceful degradation where possible
- Never `abort()` on recoverable errors

### Code Style
- Clear names: `heap_monitor_task` not `hmt`
- Concise in loops: `for (int i = 0; i < n; i++)` is fine
- Comments for "why" not "what"
- Group related code logically
- Keep functions <50 lines where possible

### Build System
- NEVER commit build artifacts
- Use `idf.py fullclean` for deep cleaning
- Verify build after EVERY change
- Test partition table changes carefully
- Document sdkconfig changes

---

## 🔍 Context Awareness

### What You Know

- **Training Data:** Up to January 2025
- **Project Files:** Full codebase access via filesystem MCP
- **Current Docs:** Real-time via Context7 MCP
- **Recent Info:** Post-training via Brave Search MCP

### What You Don't Know

- Changes made by other agents in last 5 minutes
- Real-time hardware behavior (need testing)
- User's specific API keys (don't ask, use placeholders)
- Project decisions made outside this codebase

### How to Fill Gaps

**For Technical Info:**
```bash
# Recent best practices
task-master research --query="<topic>" --save-file

# Official documentation
# Use Context7 via MCP
```

**For Project Context:**
```bash
# Recent changes
git log --oneline -10

# Current task state
task-master list --tag=<your-tag>

# Other agent activity
ls -la .taskmaster/.locks/
```

**For Codebase Context:**
```bash
# Find relevant files
rg "<search-term>" firmware/

# Understand file structure  
tree firmware/components/
```

---

## 🎯 Success Criteria

### Per-Task Success
- ✅ Build passes: `idf.py build` succeeds
- ✅ Memory safe: No heap fragmentation, <150KB usage
- ✅ Documented: Implementation logged via update-subtask
- ✅ Tested: Verification strategy executed

### Per-Component Success
- ✅ Unit tests pass for isolated modules
- ✅ Integration tests pass for component interactions
- ✅ Hardware tests pass on ESP32-S3 dev board
- ✅ Performance targets met (60 FPS, <100ms, etc.)

### Overall Project Success
- ✅ 15 templates working on first boot
- ✅ <100ms pattern switch verified
- ✅ 60 FPS LED output stable for 24h
- ✅ <150KB heap usage maintained
- ✅ Zero heap fragmentation after 24h test
- ✅ WebSocket reliable at 500KB/s
- ✅ 25+ patterns fit in 1.47MB storage
- ✅ OTA updates work without bricking

---

## 🆘 Need Help?

**Workflow Questions:** See [workflow.md](./workflow.md)  
**Multi-Agent Issues:** See [multi-agent.md](./multi-agent.md)  
**Research Guidance:** See [research-first.md](./research-first.md)  
**MCP Tool Help:** See [mcp-usage.md](./mcp-usage.md)  
**Taskmaster Commands:** See [taskmaster-reference.md](./taskmaster-reference.md)  
**Cursor Patterns:** See [cursor-workflow.md](./cursor-workflow.md)

**Still Stuck?** 
- Block task with detailed question
- Document all research attempted
- Tag relevant agent domain or escalate to human

---

**Remember:** 
- Research FIRST, code second
- Build verification is MANDATORY
- Memory safety is NON-NEGOTIABLE
- Document EVERYTHING via update-subtask

**Let's build production-grade firmware together.** 🚀

---

**Version:** 1.0  
**Last Updated:** 2025-10-15  
**Next Review:** After Phase 1 research sprint completion
