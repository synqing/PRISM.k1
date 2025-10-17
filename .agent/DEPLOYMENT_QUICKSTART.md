# 🚀 AGENT DEPLOYMENT QUICKSTART

**PM:** Captain
**Date:** 2025-10-16
**Status:** Ready for deployment

---

## 📋 AGENT ASSIGNMENTS

### Agent 1 - Firmware Commands
**Mission:** Implement firmware protocol commands for Studio integration
**File:** `.agent/AGENT_1_FIRMWARE_COMMANDS.md`
**Duration:** 2 days (9 hours)
**Tasks:** 55, 56, 57, 58
**Status:** 🟢 READY (No blockers)

**Start Command:**
```bash
cd /Users/spectrasynq/Workspace_Management/Software/PRISM.k1
cat .agent/AGENT_1_FIRMWARE_COMMANDS.md
task-master set-status --id=55 --status=in-progress
```

---

### Agent 2 - Studio Foundation
**Mission:** Build PRISM Studio desktop application foundation
**File:** `.agent/AGENT_2_STUDIO_FOUNDATION.md`
**Duration:** 1 week
**Tasks:** 41 (+ subtasks)
**Status:** 🟢 READY (No blockers)

**Start Command:**
```bash
cd /Users/spectrasynq/Workspace_Management/Software/PRISM.k1
cat .agent/AGENT_2_STUDIO_FOUNDATION.md
task-master set-status --id=41 --status=in-progress
```

---

### Agent 3 - Release v1.1
**Mission:** Ship PRISM K1 firmware v1.1 release
**File:** `.agent/AGENT_3_RELEASE_V1.1.md`
**Duration:** 1 week
**Tasks:** 20
**Status:** 🟢 READY (All dependencies DONE)

**Start Command:**
```bash
cd /Users/spectrasynq/Workspace_Management/Software/PRISM.k1
cat .agent/AGENT_3_RELEASE_V1.1.md
task-master set-status --id=20 --status=in-progress
```

---

## ⚡ QUICK START STEPS

### For Each Agent:

#### 1. Read Your Mission Brief
```bash
# Agent 1:
cat .agent/AGENT_1_FIRMWARE_COMMANDS.md

# Agent 2:
cat .agent/AGENT_2_STUDIO_FOUNDATION.md

# Agent 3:
cat .agent/AGENT_3_RELEASE_V1.1.md
```

#### 2. Run Pre-Flight Checklist
Each mission brief has a "PRE-FLIGHT CHECKLIST" section - verify all items before starting.

#### 3. Start Your First Task
```bash
# Example for Agent 1:
task-master set-status --id=55 --status=in-progress
task-master show 55  # Read detailed task info
```

#### 4. Follow Your Mission Brief
Each brief has:
- Detailed task breakdowns
- Code examples
- Test strategies
- Reference documentation
- Success criteria

#### 5. Update Task Status as You Progress
```bash
# Mark task complete:
task-master set-status --id=55 --status=done

# Add implementation notes:
task-master update-subtask --id=55.1 --prompt="Implemented DELETE handler with TLV response builder. Tests passing."
```

---

## 🎯 MISSION OBJECTIVES SUMMARY

### Agent 1 (Firmware - 9 hours):
- ✅ Task 55: DELETE command (1h)
- ✅ Task 56: STATUS/HELLO command (3h)
- ✅ Task 57: LIST command (2h)
- ✅ Task 58: mDNS responder (3h)

**Output:** Firmware commands ready for Studio integration

---

### Agent 2 (Studio - 1 week):
- ✅ Task 41: Tauri + React foundation
  - 41.1: Scaffold workspace
  - 41.2: Configure linting/TypeScript
  - 41.3: Security hardening
  - 41.4: Test harnesses (Vitest, Playwright, nextest)
  - 41.5: CI/CD matrix
  - 41.6: Developer docs
  - 41.7: Husky pre-commit

**Output:** Studio foundation ready for feature development (Tasks 42-50)

---

### Agent 3 (Release - 1 week):
- ✅ Task 20: v1.1 Release
  - Tutorial videos (5× 10-15 min)
  - 24h soak test (3 devices)
  - Preset library (≥20 patterns)
  - Release notes + documentation
  - GitHub release publication

**Output:** v1.1 publicly released with full documentation

---

## 📊 DEPENDENCIES

### Agent 1 → Agent 2:
- Agent 2's Tasks 42 (Device Discovery) and 50 (Upload/Sync) **BLOCKED** until Agent 1 completes Tasks 55-58
- Agent 2 can start Task 41 immediately (no dependencies)
- Agent 2 can work on Tasks 43-49 while waiting for Agent 1

### Agent 3 → Independent:
- Task 20 dependencies (23, 26, 27, 28) are ALL DONE ✅
- Agent 3 can start immediately

**Recommended:** Start all 3 agents in parallel. Agent 2 has plenty of work that doesn't depend on Agent 1.

---

## 🚨 CRITICAL SUCCESS FACTORS

### Agent 1:
- **Quality:** All handlers must validate inputs and handle errors gracefully
- **Testing:** Unit tests MUST pass before marking tasks done
- **Integration:** Test with actual WebSocket client (wscat or similar)

### Agent 2:
- **Security:** Follow Tauri 2 security best practices (CSP, no unsafe IPC)
- **Testing:** Comprehensive test coverage (unit, component, E2E)
- **Documentation:** README must be accurate and complete

### Agent 3:
- **Validation:** 24h soak test is non-negotiable
- **Quality:** Video tutorials must be clear and professional
- **Presets:** All patterns must be tested on hardware

---

## 📞 COMMUNICATION PROTOCOL

### Task Status Updates:
Update after completing each task/subtask:
```bash
task-master set-status --id=XX --status=done
```

### Blockers:
If blocked, update task immediately:
```bash
task-master update-task --id=XX --prompt="BLOCKED: [reason]. Need [resolution]."
```
Then notify Captain.

### Handoffs:
When completing work that unblocks another agent:
```bash
task-master update-task --id=42 --prompt="Agent 1 COMPLETE: Tasks 55-58 done. Device Discovery (Task 42) ready for implementation."
```

### Questions:
1. Check your mission brief first
2. Check CANON.md (`.taskmaster/CANON.md`)
3. Check relevant ADRs (`.taskmaster/decisions/`)
4. If still unclear, ask Captain

---

## 🛠️ TOOLS & RESOURCES

### TaskMaster Commands:
```bash
task-master next                    # Get next task
task-master show <id>              # View task details
task-master set-status --id=<id> --status=in-progress
task-master set-status --id=<id> --status=done
task-master expand --id=<id>       # Break into subtasks (if needed)
task-master list                   # View all tasks
```

### Key Files:
- **CANON:** `.taskmaster/CANON.md` (single source of truth)
- **ADRs:** `.taskmaster/decisions/*.md` (architecture decisions)
- **Tasks:** `.taskmaster/tasks/*.md` (individual task files)
- **Your Brief:** `.agent/AGENT_X_*.md`

### Development:
```bash
# Firmware (Agent 1):
cd firmware
idf.py build
idf.py flash monitor

# Studio (Agent 2):
cd studio
pnpm tauri dev
pnpm test

# Release (Agent 3):
cd tools
python -m tools.prism_packaging --help
```

---

## ✅ COMPLETION CRITERIA

### Agent 1:
- [ ] All 4 tasks (55-58) marked DONE in TaskMaster
- [ ] Firmware builds without errors
- [ ] Unit tests pass
- [ ] Manual WebSocket tests succeed
- [ ] mDNS resolves (`ping prism-k1.local`)
- [ ] Handoff note posted for Agent 2

### Agent 2:
- [ ] Task 41 marked DONE with all 7 subtasks complete
- [ ] `studio/` directory scaffolded
- [ ] Dev server launches successfully
- [ ] All lint/format/type checks pass
- [ ] Tests pass (unit, E2E, Rust)
- [ ] Production build succeeds
- [ ] CI pipeline green on GitHub Actions
- [ ] README is comprehensive

### Agent 3:
- [ ] Task 20 marked DONE
- [ ] 5 tutorial videos uploaded to YouTube
- [ ] 24h soak test completed with report
- [ ] 20+ presets created and validated
- [ ] Release notes written
- [ ] GitHub release published
- [ ] README updated with v1.1 info

---

## 🎉 POST-COMPLETION

When your mission is complete:

1. **Mark all tasks DONE:**
```bash
task-master set-status --id=<final-task-id> --status=done
```

2. **Write completion summary:**
```bash
task-master update-task --id=<main-task> --prompt="MISSION COMPLETE: [summary of deliverables]. Handoff: [notes for next agents]."
```

3. **Notify Captain:**
Post summary in team channel or via this system.

4. **Celebrate! 🎊**

---

## 💪 YOU GOT THIS!

Three elite agents, three clear missions, one incredible outcome: PRISM K1 v1.1 with Studio integration.

**Work with precision. Communicate clearly. Ship excellence.**

🫡 **Captain out.**
