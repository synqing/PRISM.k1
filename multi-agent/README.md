# Multi-Agent Execution System
## Standalone Integration with Knowledge Fortress

**Version:** 1.0.0  
**Status:** DEPLOYED  
**Last Updated:** 2025-10-16

---

## Purpose

Coordinate multiple AI agents to implement PRISM K1 firmware features while
strictly following specifications from Knowledge Fortress (.taskmaster/).

## Architecture

```
┌─────────────────────────────────────┐
│  KNOWLEDGE FORTRESS (.taskmaster/)  │
│  • CANON.md (single source of truth)│
│  • ADRs (immutable decisions)       │
│  • agent-rules.yml (governance)     │
└──────────────┬──────────────────────┘
               │ READ ONLY
               ▼
┌─────────────────────────────────────┐
│  MULTI-AGENT SYSTEM (.multi-agent/) │
│  • Task execution & coordination    │
│  • Agent communication              │
│  • Build pipeline automation        │
└──────────────┬──────────────────────┘
               │ IMPLEMENT
               ▼
┌─────────────────────────────────────┐
│  FIRMWARE CODE (firmware/)          │
│  • Must match CANON (validated)     │
└─────────────────────────────────────┘
```

## Key Principle: Separation of Concerns

- **Knowledge Fortress** = WHAT to build (decisions, specifications)
- **Multi-Agent System** = HOW to build it (execution, coordination)
- **Firmware Code** = IMPLEMENTATION (validated against CANON)

## Integration Rules

### Multi-Agent READS (read-only):
- `.taskmaster/CANON.md` - Current specifications
- `.taskmaster/decisions/*.md` - Decision context
- `.taskmaster/agent-rules.yml` - Governance rules

### Multi-Agent WRITES:
- `firmware/**/*` - Code implementations
- `.multi-agent/logs/*` - Execution logs
- `.multi-agent/metrics/*` - Performance data

### Multi-Agent NEVER Modifies:
- `.taskmaster/decisions/*` - ADRs (immutable, Captain-owned)
- `.taskmaster/CANON.md` - Auto-generated, script-owned
- `.taskmaster/agent-rules.yml` - Governance rules

---

## Quick Start

### 1. Verify Prerequisites

```bash
# Check Knowledge Fortress is set up
ls -la .taskmaster/CANON.md
ls -la .taskmaster/agent-rules.yml

# Check multi-agent installation
ls -la .multi-agent/config/agents.yml
```

### 2. Sync with Fortress

```bash
# Initial sync
.multi-agent/scripts/sync-with-fortress.sh
```

### 3. Check Agent Configuration

```bash
# View configured agents
cat .multi-agent/config/agents.yml

# Check routing rules
cat .multi-agent/config/routing.yml
```

### 4. Test Task Execution

```bash
# Execute a task (replace 15 with your task ID)
.multi-agent/scripts/agent-executor.sh 15 network claude
```

---

## Core Components

### Configuration Files

#### `config/agents.yml`
Defines agent capabilities, skills, and constraints:
- agent-1-network: WebSocket, WiFi specialist
- agent-2-storage: LittleFS, caching expert
- agent-3-playback: LED driver specialist (critical path)
- agent-4-integration: Testing, debugging lead
- agent-5-templates: Pattern designer

#### `config/routing.yml`
Task assignment strategy:
- Skill-based matching
- Load balancing
- Priority boosting
- CANON synchronization rules

#### `config/pipeline.yml`
Build and test automation:
- Pre-integration checks
- Full build validation
- Post-integration metrics

### Scripts

#### Core Scripts

**`sync-with-fortress.sh`** - Synchronize with Knowledge Fortress
```bash
.multi-agent/scripts/sync-with-fortress.sh
```
- Checks CANON.md for updates
- Notifies agents on changes
- Maintains cache hash

**`agent-executor.sh`** - Execute tasks with AI agents
```bash
.multi-agent/scripts/agent-executor.sh <task-id> [tag] [executor]
```
- Integrates Claude Code or Cursor
- Reads CANON for specifications
- Validates against CANON after implementation

**`conflict-guard.sh`** - Prevent file-level conflicts
```bash
# Lock files before modifying
.multi-agent/scripts/conflict-guard.sh check agent-1 firmware/main/main.c

# Release locks after commit
.multi-agent/scripts/conflict-guard.sh release agent-1
```

**`message-bus.sh`** - Inter-agent communication
```bash
# Post message to agent
.multi-agent/scripts/message-bus.sh post agent-1 agent-2 help "Need review on websocket code"

# Check messages
.multi-agent/scripts/message-bus.sh check agent-1
```

**`collect-metrics.sh`** - Performance analytics
```bash
.multi-agent/scripts/collect-metrics.sh
```

**`smart-router.js`** - Intelligent task assignment
```bash
# Get task recommendations
node .multi-agent/scripts/smart-router.js recommend 10

# Check agent load
node .multi-agent/scripts/smart-router.js status
```

#### Hooks

**`hooks/pre-claim.sh`** - Pre-task validation
- Syncs with Knowledge Fortress
- Validates code matches CANON
- Checks file locks

---

## Workflow

### Standard Task Execution Flow

1. **Agent claims task** from taskmaster queue
2. **Pre-claim hook runs**
   - Sync with Knowledge Fortress
   - Validate code matches CANON
   - Check for conflicts
3. **Agent reads CANON** for specifications
4. **Agent implements** feature in firmware/
5. **Post-implementation validation**
   - Run `.taskmaster/scripts/validate-canon.sh`
   - Check build passes
6. **Agent commits** code (if validation passes)
7. **Metrics collected** for future ADRs
8. **Task marked complete**

### Example Execution

```bash
# 1. Sync fortress
.multi-agent/scripts/sync-with-fortress.sh

# 2. Check what tasks are available
task-master next --tag=network

# 3. Get smart routing recommendation
node .multi-agent/scripts/smart-router.js recommend 5

# 4. Execute task
.multi-agent/scripts/agent-executor.sh 15 network claude

# 5. Verify implementation
cd firmware
../.taskmaster/scripts/validate-canon.sh

# 6. Collect metrics
cd ..
.multi-agent/scripts/collect-metrics.sh
```

---

## Monitoring & Debugging

### View Active Operations

```bash
# Active task claims
find .multi-agent/.locks -name "*.claim"

# Recent completions
find .multi-agent/.locks -name "*.completed-*" -mtime -1

# Agent messages
find .multi-agent/.messages -name "*.json" -mtime -1
```

### Check Fortress Sync Status

```bash
# View current CANON hash
cat .multi-agent/.canon-hash

# Force re-sync
rm .multi-agent/.canon-hash
.multi-agent/scripts/sync-with-fortress.sh
```

### View Metrics

```bash
# Today's metrics
cat .multi-agent/metrics/$(date +%Y%m%d).json

# Last 7 days
ls -lt .multi-agent/metrics/ | head -8
```

### Logs

```bash
# View execution logs
ls -lt .multi-agent/logs/

# View specific agent log
cat .multi-agent/logs/agent-network-15.log
```

---

## Troubleshooting

### Problem: Code doesn't match CANON

```bash
# 1. Sync with fortress
.multi-agent/scripts/sync-with-fortress.sh

# 2. Check what's wrong
cd firmware
../.taskmaster/scripts/validate-canon.sh

# 3. Fix violations in code or create new ADR
```

### Problem: Agent can't claim task

```bash
# Run pre-claim validation manually
.multi-agent/scripts/hooks/pre-claim.sh <task-id> <agent-id>

# Check for specific issues:
# - Is CANON out of date?
# - Are files locked by another agent?
# - Does code validation fail?
```

### Problem: CANON changed during implementation

```bash
# This is CRITICAL - stop immediately
# 1. Sync fortress
.multi-agent/scripts/sync-with-fortress.sh

# 2. Re-read specifications from CANON
cat .taskmaster/CANON.md

# 3. Adjust implementation to match new specs
# 4. Validate before continuing
```

### Problem: File locked by another agent

```bash
# Check who owns the lock
cat .multi-agent/.file-locks/firmware_main_main.c.lock

# If agent is done, release manually
rm .multi-agent/.file-locks/firmware_main_main.c.lock
```

---

## Security & Safety

### File Permissions

Knowledge Fortress files should be read-only for agents:
```bash
# Make ADRs read-only
chmod 444 .taskmaster/decisions/*.md

# Make CANON read-only
chmod 444 .taskmaster/CANON.md
```

### Git Hooks

Pre-commit and pre-push hooks enforce:
- No manual CANON edits
- No direct ADR modifications
- Code matches CANON

### Validation

Every operation validates against CANON:
- Before claiming task (pre-claim hook)
- After implementing (validate-canon.sh)
- Before pushing (pre-push hook)

---

## Agent Capabilities

### Agent 1: Network Specialist
**Skills:** websocket, wifi, protocols, mdns  
**Focus:** Network communication layer  
**Max Tasks:** 2 concurrent

### Agent 2: Storage Specialist
**Skills:** filesystem, littlefs, caching, compression  
**Focus:** Pattern storage and file management  
**Max Tasks:** 2 concurrent

### Agent 3: Playback Specialist
**Skills:** led-driver, effects, animation, rmt-peripheral  
**Focus:** LED control (critical path)  
**Max Tasks:** 1 concurrent (precision required)

### Agent 4: Integration Lead
**Skills:** testing, debugging, optimization, profiling  
**Focus:** System integration and quality  
**Max Tasks:** 3 concurrent

### Agent 5: Template Designer
**Skills:** creative, patterns, effects-design, metadata  
**Focus:** Pattern library development  
**Max Tasks:** 2 concurrent

---

## Configuration Reference

### Routing Strategy

Agents are assigned tasks based on:
1. **Skill match** (10 points per matching skill)
2. **Priority boost** (5 points for high-priority tasks)
3. **Load balancing** (-2 points per active task)
4. **Critical path** (+15 points for critical agents on high-priority)
5. **Research capability** (+8 points if task needs research)

### Limits

- **Max load imbalance:** 1.5x average
- **Stale task timeout:** 2 hours
- **Max retries:** 3 attempts

---

## Integration with Taskmaster

Multi-agent reads from Taskmaster's task queue (`.taskmaster/tasks/tasks.json`)
but operates independently for execution.

**Task Flow:**
```
Taskmaster → Multi-Agent Router → Agent Executor → Firmware Code
                ↓                        ↓
           Smart Assignment        CANON Validation
```

---

## Metrics & Analytics

Daily metrics include:
- Tasks completed
- Tasks active
- CANON violations
- Build failures
- Agent utilization

Use metrics for:
- Performance ADRs
- System optimization
- Capacity planning

---

## Support

### For Questions About:

- **Specifications:** Check `.taskmaster/CANON.md`
- **Decisions:** See `.taskmaster/decisions/ADR-*.md`
- **Governance:** Read `.taskmaster/agent-rules.yml`
- **Execution Issues:** Check `.multi-agent/logs/`
- **Agent Behavior:** Review `.multi-agent/config/agents.yml`

### Emergency Procedures

**If agents corrupt Knowledge Fortress:**
1. Stop all agents immediately
2. Restore from git history
3. Review `.multi-agent/logs/` for cause
4. Fix permissions
5. Resume operations

**If CANON drift detected:**
1. Stop claiming new tasks
2. Run `.taskmaster/scripts/generate-canon.sh`
3. Validate all pending changes
4. Resume after validation passes

---

## Version History

| Version | Date | Changes | Author |
|---------|------|---------|--------|
| 1.0.0 | 2025-10-16 | Initial deployment | Claude (Technical Agent) |

---

**System Status:** ✅ OPERATIONAL  
**Fortress Integration:** ✅ ACTIVE  
**Agents Configured:** 5  
**Last Sync:** Run `sync-with-fortress.sh` to check

**The multi-agent system is ready for collaborative development!**
