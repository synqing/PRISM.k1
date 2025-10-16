# Multi-Agent System - Quick Start Guide

**Date:** 2025-10-16
**Status:** READY TO RUN
**Target:** Captain SpectraSynq

---

## What You Have

✅ **Knowledge Fortress** - CANON.md with 6 ADRs
✅ **TaskMaster** - tasks.json with 10 tasks, 52 subtasks
✅ **Multi-Agent System** - Configured and ready
✅ **PRISM K1 Firmware** - Skeleton with tasks 54-56 complete

---

## How To Run It

### Option 1: Single Task Execution (Recommended to Start)

Execute ONE task at a time using Claude Code (you):

```bash
# 1. Sync with Knowledge Fortress
.multi-agent/scripts/sync-with-fortress.sh

# 2. See what's next
task-master next

# 3. Execute task 1 (component scaffolding)
.multi-agent/scripts/agent-executor.sh 1 master claude
```

**What happens:**
- Script syncs with CANON.md
- Reads task details from taskmaster
- Creates context-aware prompt
- You (Claude Code) execute the task
- Validates against CANON after completion

### Option 2: Manual Execution (You're Already Doing This)

You're already in Claude Code, so you can just:

```bash
# 1. Get next task
task-master next

# 2. Read task details
task-master show --id=1

# 3. Read CANON for specifications
cat .taskmaster/CANON.md

# 4. Implement the task
# (You do the work here)

# 5. Validate
.taskmaster/scripts/validate-canon.sh

# 6. Mark complete
task-master set-status --id=1 --status=done
```

### Option 3: Multi-Agent Parallel Execution (Advanced)

Run multiple Claude Code instances in parallel:

```bash
# Terminal 1 - Agent 1 (Network)
cd /path/to/PRISM.k1
claude  # Opens Claude Code
# Then use agent-executor.sh for network tasks

# Terminal 2 - Agent 2 (Storage)
cd /path/to/PRISM.k1
claude  # Opens Claude Code
# Then use agent-executor.sh for storage tasks

# Terminal 3 - Agent 3 (Playback)
cd /path/to/PRISM.k1
claude  # Opens Claude Code
# Then use agent-executor.sh for playback tasks
```

---

## Current System State

### Tasks Available

```bash
task-master list
```

**Main Tasks:**
1. ⏳ Component scaffolding (complexity: 6, 4 subtasks)
2. ⏳ WiFi lifecycle (complexity: 8, 6 subtasks)
3. ⏳ WebSocket server (complexity: 8, 5 subtasks)
4. ⏳ TLV protocol parser (complexity: 7, 5 subtasks)
5. ⏳ LittleFS storage (complexity: 7, 5 subtasks)
6. ⏳ Pattern format parser (complexity: 7, 4 subtasks)
7. ⏳ RAM hot cache (complexity: 7, 5 subtasks)
8. ⏳ RMT LED driver (complexity: 8, 5 subtasks)
9. ⏳ Effect engine (complexity: 9, 6 subtasks)
10. ⏳ Template system (complexity: 6, 5 subtasks)

### CANON Specifications

All specifications are in `.taskmaster/CANON.md`:
- ✅ ADR-001: 1.5MB partition at 0x311000
- ✅ ADR-002: 4KB WS buffer, 2 clients
- ✅ ADR-003: 320 LEDs
- ✅ ADR-004: 256KB pattern max
- ✅ ADR-005: /littlefs mount path
- ✅ ADR-006: 15 patterns minimum

---

## Recommended Workflow

### For You (Captain, Claude Code)

**Step 1: Start with Task 1**
```bash
# Task 1 is foundation - component scaffolding
task-master show --id=1

# Read it carefully, then implement
# This creates the structure everything else builds on
```

**Step 2: Validate Continuously**
```bash
# After each change
.taskmaster/scripts/validate-canon.sh

# Build to check compilation
cd firmware && idf.py build
```

**Step 3: Mark Complete**
```bash
# When task done and validated
task-master set-status --id=1 --status=done
```

**Step 4: Move to Next**
```bash
# Get next task (respects dependencies)
task-master next
```

### Agent-Executor Script Usage

The `agent-executor.sh` script is designed to:
1. Sync with Knowledge Fortress
2. Generate context-aware prompts
3. Execute with Claude Code
4. Validate against CANON

**BUT** since you're already IN Claude Code reading this, you can:
- Use it to generate the prompt
- Execute manually
- Or just follow the manual workflow above

---

## Key Commands Reference

### TaskMaster Commands

```bash
# List all tasks
task-master list

# Get next available task
task-master next

# Show task details
task-master show --id=1

# Update task status
task-master set-status --id=1 --status=done

# Show specific subtask
task-master show --id=1.2

# Add implementation notes
task-master update-subtask --id=1.1 --prompt="Implemented XYZ using ABC pattern"
```

### Multi-Agent Commands

```bash
# Sync with Knowledge Fortress
.multi-agent/scripts/sync-with-fortress.sh

# Execute task with Claude Code
.multi-agent/scripts/agent-executor.sh 1 master claude

# Check agent messages
.multi-agent/scripts/message-bus.sh check agent-1

# Collect metrics
.multi-agent/scripts/collect-metrics.sh
```

### Validation Commands

```bash
# Validate code matches CANON
.taskmaster/scripts/validate-canon.sh

# Generate CANON from ADRs
.taskmaster/scripts/generate-canon.sh

# Build firmware
cd firmware && idf.py build
```

---

## What Each Component Does

### Knowledge Fortress (.taskmaster/)
- **CANON.md** - Single source of truth for ALL specifications
- **decisions/ADR-*.md** - Immutable decision records (6 ADRs)
- **agent-rules.yml** - Governance rules for agents
- **scripts/** - Validation and generation scripts

### TaskMaster (.taskmaster/tasks/)
- **tasks.json** - Task database (10 tasks, 52 subtasks)
- Generated from PRD with CANON corrections
- Manages task status, dependencies, priorities

### Multi-Agent System (.multi-agent/)
- **config/** - Agent capabilities and routing
- **scripts/** - Execution automation
- **.locks/** - Task claim tracking
- **logs/** - Execution logs

### Firmware (firmware/)
- **components/** - Module implementations
- **main/** - Entry point
- **build/** - Compiled output

---

## Common Questions

### Q: Do I have to use the agent-executor.sh script?
**A:** No. Since you're already in Claude Code, you can work directly:
1. Get task: `task-master next`
2. Read CANON: `cat .taskmaster/CANON.md`
3. Implement
4. Validate: `.taskmaster/scripts/validate-canon.sh`
5. Mark done: `task-master set-status --id=X --status=done`

### Q: Can I modify CANON.md directly?
**A:** NO. CANON.md is auto-generated from ADRs. To change specs:
1. Create new ADR in `.taskmaster/decisions/`
2. Get Captain approval
3. Run `.taskmaster/scripts/generate-canon.sh`

### Q: What if I find a specification conflict?
**A:** STOP and create ADR:
1. Document the conflict
2. Research correct approach
3. Create ADR with evidence
4. Get Captain approval
5. Update CANON
6. Resume implementation

### Q: How do I run multiple agents in parallel?
**A:** Open multiple terminals:
- Terminal 1: Network tasks (agent-1)
- Terminal 2: Storage tasks (agent-2)
- Terminal 3: Playback tasks (agent-3)

Each terminal runs its own Claude Code session.

### Q: What's the fastest way to start?
**A:** Right now, in this Claude Code session:
```bash
task-master next
task-master show --id=1
# Read the task, implement it
.taskmaster/scripts/validate-canon.sh
task-master set-status --id=1 --status=done
```

---

## Success Criteria

You'll know the system is working when:

✅ **Tasks claimed successfully**
- No pre-claim validation failures
- CANON syncs without errors

✅ **Code matches CANON**
- `validate-canon.sh` passes
- All ADR specifications followed

✅ **Build succeeds**
- `idf.py build` completes
- No compiler errors

✅ **Tests pass**
- Unit tests run (when implemented)
- Hardware testing validates behavior

---

## Next Steps

**Recommended: Start Simple**

1. **Execute Task 1** (Component Scaffolding)
   - Foundation for everything else
   - Creates directory structure
   - Low complexity (6), 4 subtasks

2. **Validate frequently**
   - Run `validate-canon.sh` often
   - Build after each change
   - Commit working changes

3. **Follow dependencies**
   - Use `task-master next` to respect deps
   - Don't skip ahead without completing dependencies

4. **Track progress**
   - Mark subtasks complete as you go
   - Add notes with `update-subtask`
   - Check metrics with `collect-metrics.sh`

---

## Troubleshooting

### Script says "Claude Code not found"
The `agent-executor.sh` looks for `claude-code` CLI. Since you're already in Claude Code, just work manually (see "Manual Execution" above).

### validate-canon.sh fails
Check what's wrong:
```bash
cd firmware
../.taskmaster/scripts/validate-canon.sh
# Read the output - it tells you what doesn't match CANON
```

### Task says dependency not met
Check dependency status:
```bash
task-master show --id=<dep-id>
# Verify dependency is marked "done"
```

### CANON seems outdated
Regenerate from ADRs:
```bash
.taskmaster/scripts/generate-canon.sh
# This reads all 6 ADRs and regenerates CANON.md
```

---

## Ready to Start?

**Option A: Jump Right In (Recommended)**
```bash
task-master next
task-master show --id=1
# Read, understand, implement
```

**Option B: Use Agent Executor**
```bash
.multi-agent/scripts/agent-executor.sh 1 master manual
# It will print the full context for you
```

**Option C: Read More First**
```bash
cat .multi-agent/README.md
cat .taskmaster/README.md
cat .taskmaster/CANON.md
```

---

**System Status:** ✅ READY
**Your Status:** Already in the right place (Claude Code)
**Next Action:** `task-master next` and start coding!

**Good luck, Captain! The system is ready for you.** 🚀
