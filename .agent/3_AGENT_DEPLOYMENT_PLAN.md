# 3-Agent Sequential Deployment Plan
**ADR-010 Implementation: 10 Tasks, 56 Subtasks**

## Overview

3 Codex agents working sequentially through balanced pipelines.
All agents work on `main` branch, committing after each subtask completion.

---

## 🔥 AGENT 1: Critical Path (Foundation → Integration → Advanced)

**Workload:** 4 tasks, 20 subtasks
**Complexity:** 24 points
**Pipeline:** 11 → 12 → 14 → 19

### Sequential Flow:

```
Task 11: Motion/Sync Enums (3 subtasks)
   ↓ Creates enums/structs everyone needs
Task 12: SYNC/OFFSET Temporal (5 subtasks)
   ↓ Uses enums from 11
Task 14: Temporal Integration (4 subtasks)
   ↓ Wires 12 into LED playback
Task 19: CUSTOM Mode + Web Editor (8 subtasks)
   ↓ Most complex, builds on everything
```

### Why This Pipeline?
- **Critical dependency chain** - each task builds on previous
- Task 11 unblocks everything else
- Task 19 is most complex, placed last when agent has context
- Natural progression: data → logic → integration → advanced

### Agent Brief Location:
`.agent/AGENT_1_FOUNDATION_ADVANCED.md`

---

## 🟦 AGENT 2: Parser → Shapes → Validation

**Workload:** 3 tasks, 17 subtasks
**Complexity:** 22 points
**Pipeline:** 13 → 15 → 16

### Sequential Flow:

```
Task 13: .prism v1.1 Parser (5 subtasks)
   ↓ Extends file format
Task 15: PROGRESSIVE Mode (6 subtasks)
   ↓ Uses v1.1 format for shape presets
Task 16: Hardware Validation (6 subtasks)
   ↓ Camera-based validation of PROGRESSIVE
```

### Why This Pipeline?
- **Parser → Implementation → Validation** flow
- Task 13 independent of Agent 1's early work
- PROGRESSIVE mode exercises v1.1 format
- Hardware validation tests real-world temporal sequencing

### Agent Brief Location:
`.agent/AGENT_2_PARSER_SHAPES.md`

---

## 🟨 AGENT 3: WAVE → Profile → Release

**Workload:** 3 tasks, 19 subtasks
**Complexity:** 24 points
**Pipeline:** 17 → 18 → 20

### Sequential Flow:

```
Task 17: WAVE Mode (6 subtasks)
   ↓ LUT-based sinusoidal patterns
Task 18: WAVE Profiling (5 subtasks)
   ↓ Optimize Task 17 performance
Task 20: Docs/Migration/Release (8 subtasks)
   ↓ Final polish, depends on all work
```

### Why This Pipeline?
- **Build → Optimize → Document** flow
- WAVE mode is independent implementation
- Profiling Task 18 optimizes Task 17
- Task 20 waits for other agents to finish bulk work

### Agent Brief Location:
`.agent/AGENT_3_WAVE_RELEASE.md`

---

## Coordination Protocol

### Git Workflow (All Agents on `main`):

```bash
# Before starting each subtask
git pull --rebase

# After completing each subtask
git add .
git commit -m "feat(task-X): complete subtask X.Y - description"
git push

# Update taskmaster
task-master set-status --id=X.Y --status=done
```

### Commit Message Template:

```
feat(task-11): implement subtask 11.1 - create prism_motion.h enums
feat(task-12): implement subtask 12.3 - add SYNC mode memcpy path
fix(task-15): correct triangle interpolation boundary condition
test(task-18): add esp_timer profiling validation harness
```

### Conflict Resolution:

If merge conflict occurs:
1. Agent pauses
2. You manually resolve conflict
3. Agent continues from next subtask

**Expected conflicts:** Minimal - agents work on different components

---

## Progress Monitoring

### You Run This:

```bash
# Real-time dashboard (refresh every 30s)
watch -n 30 'task-master list'

# Or manually check
task-master list | grep -E "Task (11|12|13|14|15|16|17|18|19|20)"
```

### Agent Status Markers:

```
○ Task 11 | pending    → Not started
▶ Task 11 | in-progress → Agent working
✓ Task 11 | done       → Completed
```

---

## Launch Sequence

### 1. Start All 3 Agents Simultaneously

**VS Code Window 1 - Agent 1:**
```
Open ChatGPT extension
Paste: "Read .agent/AGENT_1_FOUNDATION_ADVANCED.md and execute the entire
pipeline sequentially. Use task-master to track progress. Commit after each
subtask."
```

**VS Code Window 2 - Agent 2:**
```
Open ChatGPT extension
Paste: "Read .agent/AGENT_2_PARSER_SHAPES.md and execute the entire pipeline
sequentially. Use task-master to track progress. Commit after each subtask."
```

**VS Code Window 3 - Agent 3:**
```
Open ChatGPT extension
Paste: "Read .agent/AGENT_3_WAVE_RELEASE.md and execute the entire pipeline
sequentially. Use task-master to track progress. Commit after each subtask."
```

### 2. Monitor Progress

```bash
# Terminal 4 (your command center)
cd /Users/spectrasynq/Workspace_Management/Software/PRISM.k1
watch -n 30 'task-master list'
```

### 3. Handle Issues

- **Agent stuck?** → Check their chat, provide guidance
- **Merge conflict?** → Pause agent, resolve, resume
- **Build failure?** → Agent should debug, but you can assist

---

## Estimated Timeline

**Agent 1:** 4-6 hours (20 subtasks)
**Agent 2:** 3-5 hours (17 subtasks)
**Agent 3:** 4-6 hours (19 subtasks)

**Total wall time:** ~6 hours (agents work in parallel)
**Total work:** ~15 hours of implementation (compressed to 6 via parallelism)

---

## Success Criteria

### All Agents Complete When:

- ✅ All 56 subtasks marked `done` in taskmaster
- ✅ All 10 tasks marked `done`
- ✅ Firmware builds cleanly: `cd firmware && idf.py build`
- ✅ No merge conflicts remaining
- ✅ Git history shows sequential commits from all 3 agents

### Final Validation:

```bash
cd firmware
idf.py build
idf.py unity  # Run all tests
```

---

## Notes

- **Agent 1 is critical path** - prioritize unblocking them
- **Agent 3's Task 20** may wait for Agents 1 & 2 to finish
- **Agents work independently** - minimal cross-dependencies
- **All research is complete** - agents have full context
- **Briefs contain code snippets** - agents can copy-paste foundations

---

## Emergency Procedures

### Agent Diverges:
```bash
# Reset agent's work
git reset --hard origin/main
# Restart from last successful commit
```

### Total Chaos:
```bash
# Pause all agents
# Assess damage
# Cherry-pick successful commits
# Resume from stable state
```

---

**Ready to deploy? Check `.agent/AGENT_X_*.md` briefs before launching!**
