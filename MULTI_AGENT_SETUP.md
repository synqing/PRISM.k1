# PRISM K1 - Multi-Agent Setup Guide

**Status:** ✅ READY TO RUN
**Date:** 2025-10-16

---

## 🎯 What Is This?

A system to run **multiple Claude Code instances in parallel**, each working on different PRISM K1 firmware tasks simultaneously.

**Goal:** 3-5x faster development by parallelizing independent tasks.

---

## ✅ Fixed Issues

- ✅ Removed `.` from `.multi-agent/` folder (now `multi-agent/`)
- ✅ Fixed `return` statement bug in `sync-with-fortress.sh` (changed to `exit`)
- ✅ Updated all script references to use `multi-agent/` instead of `.multi-agent/`
- ✅ Created launcher script to spawn multiple Claude Code instances

---

## 🚀 How To Run Multi-Agent System

### Option 1: Automated Launcher (EASIEST)

Run the launcher script - it will spawn multiple terminal windows:

```bash
./multi-agent-launch.sh
```

**What it does:**
1. Checks Claude Code CLI is installed
2. Syncs with Knowledge Fortress
3. Asks how many agents you want (1-5)
4. Spawns that many terminal windows
5. Each window runs Claude Code on a different task

**Requirements:**
- Claude Code CLI installed (`claude` command available)
- task-master CLI installed (`task-master` command available)
- macOS (uses Terminal.app) or Linux (uses gnome-terminal)

---

### Option 2: Manual Spawning

Open separate terminals manually and run Claude Code in each:

**Terminal 1 - Integration Agent (Task 1):**
```bash
cd /Users/spectrasynq/Workspace_Management/Software/PRISM.k1
task-master show --id=1
claude
```

**Terminal 2 - Network Agent (Task 2):**
```bash
cd /Users/spectrasynq/Workspace_Management/Software/PRISM.k1
task-master show --id=2
claude
```

**Terminal 3 - Storage Agent (Task 5):**
```bash
cd /Users/spectrasynq/Workspace_Management/Software/PRISM.k1
task-master show --id=5
claude
```

**Terminal 4 - Playback Agent (Task 8):**
```bash
cd /Users/spectrasynq/Workspace_Management/Software/PRISM.k1
task-master show --id=8
claude
```

Each Claude Code instance will work independently on its assigned task.

---

### Option 3: Using This Current Session (Single Agent)

Since I'm already running in this Claude Code session, just continue here:

```bash
task-master next
task-master show --id=1
# I'll implement task 1 right now
```

---

## 📋 Task Assignment Strategy

**Parallelizable Tasks (can run simultaneously):**

| Agent | Tag | Tasks | Why Parallel-Safe |
|-------|-----|-------|-------------------|
| Integration | master | 1 | Foundation - creates structure for others |
| Network | network | 2, 3, 4 | Independent module (no dependencies on storage/playback) |
| Storage | storage | 5, 6, 7 | Independent module (no dependencies on network/playback) |
| Playback | playback | 8, 9 | Independent module (needs task 1 complete first) |
| Templates | templates | 10 | Independent (builds on storage when ready) |

**Dependencies:**
- Task 1 MUST complete first (creates component structure)
- After Task 1, Tasks 2-10 can run in parallel

---

## 🔧 System Components

### Scripts Fixed

1. **`multi-agent/scripts/sync-with-fortress.sh`**
   - Fixed `return` → `exit` bug
   - Updated paths from `.multi-agent/` to `multi-agent/`
   - ✅ Now works correctly

2. **`multi-agent/scripts/agent-executor.sh`**
   - Updated paths from `.multi-agent/` to `multi-agent/`
   - ✅ Now works correctly

3. **All other scripts** in `multi-agent/scripts/`
   - Batch updated all references
   - ✅ All paths corrected

### New Scripts Created

4. **`multi-agent-launch.sh`** (NEW)
   - Spawns multiple Claude Code instances
   - macOS/Linux compatible
   - Interactive agent selection
   - ✅ Ready to use

---

## 🎮 How Agents Coordinate

### Task Claiming
Each agent claims tasks through task-master:
```bash
task-master set-status --id=X --status=in-progress
```

### CANON Sync
All agents read the same CANON.md:
```bash
multi-agent/scripts/sync-with-fortress.sh
```

### Validation
Each agent validates against CANON before completing:
```bash
.taskmaster/scripts/validate-canon.sh
```

### No Conflicts
- Task dependencies prevent conflicts
- Each agent works on different components
- CANON ensures all agents follow same specs
- Git handles merge conflicts (if any)

---

## 📊 Monitoring Progress

### Check Task Status
```bash
task-master list
```

### Check Agent Logs
```bash
ls -lh multi-agent/logs/
tail -f multi-agent/logs/agent-network-2.log
```

### Check Build Status
```bash
cd firmware && idf.py build
```

---

## ⚠️ Important Notes

### I Cannot Duplicate Myself
**Claude Code instances are NOT self-replicating.**

Each Claude Code instance is a separate terminal session that YOU must spawn. I (this current instance) cannot create new instances of myself.

The `multi-agent-launch.sh` script spawns terminal windows for you, but you still need to:
1. Have Claude Code CLI installed
2. Run the launcher from YOUR terminal
3. The launcher opens NEW terminals for you

### Each Agent is Independent
- Each Claude Code instance has its own conversation history
- They don't share context or memory
- Coordination happens through task-master and CANON.md
- This is INTENTIONAL - allows true parallel execution

### Recommended Workflow

**Day 1: Single Agent (Task 1)**
```bash
# Complete Task 1 first (foundation)
task-master show --id=1
# Implement task 1
task-master set-status --id=1 --status=done
```

**Day 2+: Multi-Agent (Tasks 2-10)**
```bash
# Once Task 1 is done, spawn multiple agents
./multi-agent-launch.sh
# Choose 3-5 agents
# Each works on different tasks in parallel
```

---

## 🐛 Troubleshooting

### "Claude Code not found"
Install Claude Code CLI:
```bash
# Check installation instructions from Anthropic
claude --version
```

### "task-master not found"
Install task-master:
```bash
npm install -g task-master-ai
```

### "Sync with fortress failed"
Check CANON.md exists:
```bash
cat .taskmaster/CANON.md
```

If missing, regenerate:
```bash
.taskmaster/scripts/generate-canon.sh
```

### Scripts show "permission denied"
Make scripts executable:
```bash
chmod +x multi-agent/scripts/*.sh
chmod +x multi-agent-launch.sh
```

### Agents stepping on each other
Check task status before claiming:
```bash
task-master list
# Ensure no other agent has claimed the task
```

---

## 🎯 Quick Start Commands

### Start Single Agent (This Session)
```bash
task-master next
# I'll implement it right now
```

### Start Multi-Agent (New Terminals)
```bash
./multi-agent-launch.sh
# Answer prompts, wait for terminals to spawn
```

### Monitor Progress
```bash
watch -n 5 task-master list
```

### Validate Everything
```bash
.taskmaster/scripts/validate-canon.sh
cd firmware && idf.py build
```

---

## ✅ System Status

```
✅ Scripts fixed (return → exit bug)
✅ Paths updated (.multi-agent/ → multi-agent/)
✅ Launcher created (multi-agent-launch.sh)
✅ CANON synced and validated
✅ TaskMaster ready (10 tasks, 52 subtasks)
✅ Knowledge Fortress operational (6 ADRs)

🚀 READY TO RUN MULTI-AGENT SYSTEM
```

---

**Captain,**

The multi-agent system is now fully operational. All bugs fixed. Launcher ready.

**To spawn multiple agents:** `./multi-agent-launch.sh`

**To continue in this session:** `task-master next` (I'll do it right now)

Choose your path and let's get this show on the fucking road. 🚀

---

*Document: MULTI_AGENT_SETUP.md*
*Date: 2025-10-16*
*Status: Ready for execution*
