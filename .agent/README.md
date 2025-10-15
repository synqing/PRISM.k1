# PRISM.k1 Agent Configuration

**Mission:** Build production-ready ESP32-S3 firmware for PRISM LED controller with multi-agent collaborative workflow.

## 📚 Documentation Structure

This directory contains the complete agent configuration and workflow documentation:

### Core Documents

- **[instructions.md](./instructions.md)** - Master agent instructions (auto-loaded by Claude/Cursor)
- **[workflow.md](./workflow.md)** - Development workflow and quality gates
- **[multi-agent.md](./multi-agent.md)** - Multi-agent coordination patterns
- **[research-first.md](./research-first.md)** - Research-first methodology (MANDATORY)
- **[mcp-usage.md](./mcp-usage.md)** - MCP tool reference and usage guide

### Integration References

- **[cursor-workflow.md](./cursor-workflow.md)** - Cursor-specific workflow patterns
- **[taskmaster-reference.md](./taskmaster-reference.md)** - Complete taskmaster command reference

### Multi-Agent Deployment

- **[multi-agent-quickstart.md](./multi-agent-quickstart.md)** - 15-minute deployment guide for 5 concurrent agents
- **Multi-Agent Scripts:** `.taskmaster/scripts/multi-agent.sh` - Complete automation suite

## 🎯 Quick Start

### For New Agents

1. Read [instructions.md](./instructions.md) - Your mission brief
2. Review [research-first.md](./research-first.md) - MANDATORY before any coding
3. Check [multi-agent.md](./multi-agent.md) - Coordination protocols
4. Reference [mcp-usage.md](./mcp-usage.md) - Available tools

### For Multi-Agent Deployment

**Deploy 5 concurrent agents in 15 minutes:**

```bash
# 1. Setup work streams
cd /Users/spectrasynq/Workspace_Management/Software/PRISM.k1
./.taskmaster/scripts/multi-agent.sh setup

# 2. Launch agents
./.taskmaster/scripts/multi-agent.sh run-agent agent-1-network network 50 &
./.taskmaster/scripts/multi-agent.sh run-agent agent-2-storage storage 50 &
./.taskmaster/scripts/multi-agent.sh run-agent agent-3-playback playback 30 &
./.taskmaster/scripts/multi-agent.sh run-agent agent-4-templates templates 40 &
./.taskmaster/scripts/multi-agent.sh run-agent agent-5-integration integration 30 &

# 3. Monitor progress
./.taskmaster/scripts/multi-agent.sh dashboard
```

**See [multi-agent-quickstart.md](./multi-agent-quickstart.md) for complete guide.**

### For Existing Workflow

- Follow [workflow.md](./workflow.md) for task execution
- Use [taskmaster-reference.md](./taskmaster-reference.md) for commands
- Consult [cursor-workflow.md](./cursor-workflow.md) for Cursor-specific patterns

## 🚨 Critical Constraints

**Hardware:**
- ESP32-S3: 512KB RAM, 8MB Flash
- Memory budget: <150KB heap at runtime
- Performance target: 60 FPS LED output, <100ms pattern switch

**Development Principles:**
1. **Research First** - ALWAYS research before implementing
2. **Build Verification** - EVERY task must verify `idf.py build`
3. **Memory Safety** - Zero fragmentation tolerance
4. **Documentation** - Log everything via update-subtask

## 🔧 MCP Tools Available

| Tool | Purpose | Priority |
|------|---------|----------|
| task-master-ai | Task management | ⭐⭐⭐⭐⭐ |
| filesystem | Code operations | ⭐⭐⭐⭐⭐ |
| context7 | ESP-IDF/FreeRTOS docs | ⭐⭐⭐⭐⭐ |
| brave-search | Research queries | ⭐⭐⭐⭐⭐ |
| sequential-thinking | Architecture decisions | ⭐⭐⭐⭐ |
| memory | Agent state tracking | ⭐⭐⭐⭐ |
| git | Version control | ⭐⭐⭐ |

## 📊 Project Status

- **Tasks:** 53 main tasks (expandable to ~80-100 subtasks)
- **Tags:** master, network, storage, playback, templates, integration
- **Phase:** Research Sprint (Days 1-3) → Expansion (Day 4) → Implementation (Week 1+)

## 🤝 Multi-Agent Workflow

### Velocity Improvements

**Single Agent:** 3-5 tasks/day → 10-15 days total  
**5 Concurrent Agents:** 12-20 tasks/day → 3-5 days total  
**Speedup:** **3x faster!**

### Agent Specialization

Agents work in isolated tagged contexts:
- **Agent 1 (Network):** WebSocket, WiFi, protocols
- **Agent 2 (Storage):** LittleFS, patterns, caching
- **Agent 3 (Playback):** LED driver, effects, animation
- **Agent 4 (Templates):** Pattern templates, deployment
- **Agent 5 (Integration):** Testing, debugging, optimization

### Features

✅ Atomic task claiming (no conflicts)  
✅ Automatic health monitoring  
✅ Real-time dashboard  
✅ Build verification per task  
✅ Research-first development  
✅ Stale task auto-recovery  

See [multi-agent.md](./multi-agent.md) for coordination details and [multi-agent-quickstart.md](./multi-agent-quickstart.md) for deployment.

## 📖 External Resources

- **Project Root:** `/Users/spectrasynq/Workspace_Management/Software/PRISM.k1`
- **Firmware:** `./firmware/`
- **Taskmaster:** `./.taskmaster/`
- **PRD:** `./.taskmaster/docs/prism-firmware-prd.txt`
- **Multi-Agent Scripts:** `./.taskmaster/scripts/multi-agent.sh`

## 🆘 Troubleshooting

**Agent Conflicts?** → Check [multi-agent.md](./multi-agent.md) for task claiming  
**Build Failures?** → Review [workflow.md](./workflow.md) quality gates  
**Stuck on Task?** → Follow [research-first.md](./research-first.md)  
**MCP Issues?** → Consult [mcp-usage.md](./mcp-usage.md)  
**Multi-Agent Setup?** → See [multi-agent-quickstart.md](./multi-agent-quickstart.md)

## 🚀 Quick Commands

```bash
# Single agent workflow
task-master next
task-master show <id>
task-master set-status --id=<id> --status=done

# Multi-agent workflow  
./.taskmaster/scripts/multi-agent.sh setup
./.taskmaster/scripts/multi-agent.sh run-agent <agent-id> <tag> <max-tasks>
./.taskmaster/scripts/multi-agent.sh dashboard
./.taskmaster/scripts/multi-agent.sh health-check

# Research and documentation
task-master research --task-ids=<id> --query="..." --save-to=<id>
task-master update-subtask --id=<id> --prompt="findings..."
```

---

**Version:** 1.1 (Multi-Agent Support)  
**Last Updated:** 2025-10-15  
**Maintained By:** PRISM.k1 Development Team
