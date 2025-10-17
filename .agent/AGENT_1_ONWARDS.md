# AGENT 1 - ONWARDS INSTRUCTIONS

**Agent ID:** AGENT-1-FIRMWARE
**Current Status:** Tasks 55-58 COMPLETE ✅
**Updated:** 2025-10-17

---

## 🎉 MISSION COMPLETE - TASKS 55-58

Excellent work! You've successfully implemented:
- ✅ Task 55: DELETE command (LIST was reassigned to 57)
- ✅ Task 56: STATUS/HELLO command
- ✅ Task 57: LIST command for pattern enumeration
- ✅ Task 58: mDNS responder (`prism-k1.local`)

**Impact:** Agent 2 (Studio) is now UNBLOCKED for:
- Task 42: Device Discovery (needs mDNS + STATUS)
- Task 50: Upload/Sync (needs LIST + DELETE)

---

## 📋 NEXT MISSION: CORE FIRMWARE FEATURES

You have **3 high-value firmware tasks** remaining. Captain recommends completing these in priority order:

### Priority Order:

#### 1. Task 7: RAM Hot Cache with LRU Eviction (HIGH PRIORITY)
**Why First:** Critical for performance. Pattern switching must be <100ms.

**File:** `firmware/components/storage/pattern_cache.c` (new)

**Requirements:**
- Implement RAM cache for frequently-used patterns
- LRU eviction policy when cache is full
- Cache size: 256KB (2 full patterns or 4-8 smaller ones)
- Hit rate target: >80% for typical usage
- Miss penalty: <50ms to load from LittleFS

**Dependencies:**
- ✅ Task 5 (storage) - DONE
- ✅ Task 6 (parser) - DONE

**Complexity:** ● 8 (High)

**Start Command:**
```bash
task-master set-status --id=7 --status=in-progress
task-master show 7
```

---

#### 2. Task 9: Effect Engine with Parameter Interpolation (MEDIUM PRIORITY)
**Why Second:** Enables rich pattern authoring. Needed for Studio integration.

**File:** `firmware/components/playback/effect_engine.c` (new)

**Requirements:**
- Parameter interpolation (smooth transitions)
- Effect chaining/compositing
- Real-time parameter updates via WebSocket
- 60 FPS target with effects enabled
- Memory budget: <50KB heap

**Dependencies:**
- ✅ Task 6 (parser) - DONE
- ⏳ Task 7 (cache) - YOUR NEXT TASK
- ✅ Task 8 (LED driver) - DONE

**Complexity:** ● 9 (Very High)

**Defer Until:** Task 7 complete

---

#### 3. Task 10: Template System with 15 Canvas-Ready Presets (LOWER PRIORITY)
**Why Third:** User-facing feature, depends on effect engine.

**Requirements:**
- 15 pre-built pattern templates
- Canvas metadata for Studio
- Example patterns demonstrating all temporal sequencing modes
- Documentation for each template

**Dependencies:**
- ✅ Task 5 (storage) - DONE
- ✅ Task 6 (parser) - DONE
- ⏳ Task 7 (cache) - YOUR NEXT TASK
- ⏳ Task 9 (effect engine) - AFTER TASK 7

**Complexity:** ● 7 (High)

**Defer Until:** Tasks 7 and 9 complete

---

## 🔧 TASK 54: HARDWARE BENCHMARK (DEFERRED)

**Status:** Currently marked "in-progress" but DEFERRED per Captain's orders

**Why Deferred:**
- Non-blocking for v1.1 release
- Requires dedicated hardware fixture
- Opportunistic execution when fixture available

**Action Required:** Mark as "deferred" for now:
```bash
task-master set-status --id=54 --status=deferred
task-master update-task --id=54 --prompt="DEFERRED: Non-blocking for release. Will execute when hardware fixture available after Tasks 7, 9, 10 complete. Agent 1 to coordinate timing."
```

**Future Execution:**
- After Tasks 7, 9, 10 are complete
- When ESP32-S3 benchmark fixture is free
- Coordinate with Captain for timing

---

## 🎯 RECOMMENDED WORKFLOW

### Week 1: Task 7 (RAM Cache)
```bash
# Day 1-2: Design cache architecture
task-master set-status --id=7 --status=in-progress
# Read existing storage layer
cat firmware/components/storage/pattern_storage_crud.c
# Design LRU data structures and eviction policy

# Day 3-4: Implement cache with unit tests
# Create pattern_cache.c and pattern_cache.h
# Implement load/evict/hit-rate tracking

# Day 5: Integration testing
# Test with real patterns on hardware
# Measure hit rates and miss penalties
# Optimize if needed

# Mark complete
task-master set-status --id=7 --status=done
```

### Week 2: Task 9 (Effect Engine)
```bash
# Start after Task 7 complete
task-master set-status --id=9 --status=in-progress
# Follow similar implementation pattern
```

### Week 3: Task 10 (Templates)
```bash
# Start after Task 9 complete
task-master set-status --id=10 --status=in-progress
```

---

## 📚 KEY REFERENCE DOCUMENTATION

### CANON and ADRs:
- `.taskmaster/CANON.md` - Single source of truth
- `.taskmaster/decisions/ADR-001.md` - Partition table
- `.taskmaster/decisions/ADR-002.md` - WebSocket buffer (4096 bytes)
- `.taskmaster/decisions/ADR-007.md` - Partition alignment
- `.taskmaster/decisions/ADR-010.md` - LGP Motion Architecture (temporal sequencing)

### Existing Code to Study:
- `firmware/components/storage/pattern_storage_crud.c` - Storage layer (for Task 7)
- `firmware/components/playback/decode_core.c` - Decode pipeline (for Task 9)
- `firmware/components/core/prism_decode_hooks.c` - Temporal hooks (for Task 9)

### ESP-IDF Documentation:
- Memory allocation: https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/system/mem_alloc.html
- FreeRTOS tasks: https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/system/freertos.html

---

## 💬 COORDINATION WITH OTHER AGENTS

### Agent 2 (Studio) - UNBLOCKED ✅
Your work (Tasks 55-58) has unblocked:
- Task 42: Device Discovery
- Task 50: Upload/Sync

**No further coordination needed** - Agent 2 can proceed independently.

### Agent 3 (Release) - INDEPENDENT
Agent 3 is working on documentation, tutorials, and soak testing. No dependencies on your firmware work.

---

## 🚨 CRITICAL REMINDERS

1. **Always check CANON.md first** - It's the single source of truth
2. **Memory budget is strict:**
   - Task 7 (cache): 256KB RAM
   - Task 9 (effects): <50KB heap
   - Total system: <150KB heap usage
3. **Performance targets:**
   - Pattern switch: <100ms
   - Frame rate: 60 FPS sustained
   - Cache hit rate: >80%
4. **Test on hardware** - Simulator behavior differs from ESP32-S3
5. **Update task status** after each completion

---

## ✅ SUCCESS CRITERIA

### Task 7 (Cache):
- [ ] Cache implements LRU eviction
- [ ] Hit rate >80% with 5 patterns
- [ ] Miss penalty <50ms
- [ ] No memory leaks (soak test 1 hour)
- [ ] Unit tests pass
- [ ] Integration tests pass on hardware

### Task 9 (Effects):
- [ ] Parameter interpolation smooth (no visible steps)
- [ ] Effect chaining works (3+ effects)
- [ ] Real-time updates via WebSocket
- [ ] 60 FPS sustained with effects
- [ ] Heap usage <50KB
- [ ] Unit tests pass

### Task 10 (Templates):
- [ ] 15 templates created
- [ ] All temporal modes represented (SYNC, OFFSET, PROGRESSIVE, WAVE, CUSTOM)
- [ ] Canvas metadata included
- [ ] Templates tested on hardware
- [ ] Documentation complete

---

## 🎖️ FINAL NOTES

**Excellent work on Tasks 55-58!** Your firmware commands are rock-solid and have unblocked the entire Studio integration workflow.

**Next up:** Task 7 (RAM Cache) is your highest priority. This is a critical performance component that will make pattern switching feel instant.

**Questions?** Check CANON.md first, then ask Captain.

**Good hunting, Agent 1!** 🫡

---

**PM:** Captain
**Last Updated:** 2025-10-17
**Next Review:** After Task 7 completion
