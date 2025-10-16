# Agent Instructions

See `.agent/instructions.md` for complete agent configuration for PRISM.k1 firmware development.

## 🎯 Critical Documentation Hierarchy

### Primary Sources (START HERE)
1. **[Technical Spec Index](.taskmaster/docs/README.md)** ⭐ - SOURCE OF TRUTH for all technical documentation
2. **[Authoritative Specification](.taskmaster/docs/PRISM_AUTHORITATIVE_SPECIFICATION.md)** 📋 - Final reconciled technical spec
3. **[Master Instructions](.agent/instructions.md)** - Full agent guidance for PRISM project

### Agent Resources
- **[Workflow](.agent/workflow.md)** - Development workflow & quality gates
- **[Multi-Agent](.agent/multi-agent.md)** - Multi-agent coordination patterns
- **[Research-First](.agent/research-first.md)** - Research-first methodology
- **[MCP Usage](.agent/mcp-usage.md)** - MCP tool usage guide
- **[Cursor Workflow](.agent/cursor-workflow.md)** - Cursor-specific development workflow
- **[Taskmaster Reference](.agent/taskmaster-reference.md)** - Complete Taskmaster tool reference
- **[Multi-Agent Quickstart](.agent/multi-agent-quickstart.md)** - 15-minute deployment guide

### Project Status
- **[STATUS.md](STATUS.md)** - Current project configuration status
- **[MIGRATION.md](MIGRATION.md)** - Configuration migration history

## Project Overview

**PRISM.k1** - ESP32-S3 LED controller firmware
- Hardware: ESP32-S3, 512KB RAM, 8MB Flash
- Stack: ESP-IDF v5.x, FreeRTOS, LittleFS
- Goals: <100ms pattern switch, 60 FPS, <150KB heap
- Critical: Memory safety, zero fragmentation, field reliability

## Technical Specifications

**⚠️ ALWAYS CHECK:** `.taskmaster/docs/PRISM_AUTHORITATIVE_SPECIFICATION.md` for:
- Partition table configuration
- Filesystem mount paths (`/littlefs` NOT `/prism`)
- WebSocket protocol (Binary TLV, NOT JSON)
- Memory allocations and budgets
- Error codes and constants

## Essential Tools

Available via MCP:
- **task-master-ai** - Task management & workflow
- **filesystem** - Project file operations
- **context7** - ESP-IDF & FreeRTOS documentation
- **brave-search** - Web research for latest best practices
- **sequential-thinking** - Complex problem solving
- **memory** - Agent state persistence
- **git** - Version control operations
- **sqlite** - Data operations

## Quick Command Reference

```bash
# Single agent workflow
task-master next              # Get next task
task-master show <id>         # View task details
task-master set-status --id=<id> --status=done

# Multi-agent deployment
./taskmaster setup            # Initialize multi-agent
./taskmaster dashboard        # Real-time monitoring
./taskmaster run-agent <id> <tag> <max>  # Start agent
```

For complete documentation, see the files linked above.
