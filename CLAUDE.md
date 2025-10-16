# Agent Instructions

See `.agent/instructions.md` for complete agent configuration for PRISM.k1 firmware development.

## 🎯 Critical Documentation Hierarchy

### Primary Sources (START HERE)
1. **[Knowledge Fortress Entry Point](.taskmaster/README.md)** ⭐ - START HERE for all documentation
2. **[CANON.md](.taskmaster/CANON.md)** 📋 - SINGLE SOURCE OF TRUTH (auto-generated, immutable)
3. **[Master Instructions](.agent/instructions.md)** - Full agent guidance for PRISM project

### Knowledge Fortress System
- **[METHODOLOGY.md](.taskmaster/METHODOLOGY.md)** - Research-first process
- **[ADR_GUIDE.md](.taskmaster/ADR_GUIDE.md)** - How to write Architecture Decision Records
- **[VALIDATION_GUIDE.md](.taskmaster/VALIDATION_GUIDE.md)** - Validation procedures and checklists
- **[agent-rules.yml](.taskmaster/agent-rules.yml)** - Machine-readable rules for agent behavior

### Architecture Decision Records (ADRs)
- **[decisions/](.taskmaster/decisions/)** - Immutable ADRs (5 approved decisions)
- ADR-001: Partition Table Configuration
- ADR-002: WebSocket Buffer Size
- ADR-003: LED Configuration
- ADR-004: Pattern Size Limits
- ADR-005: Storage Mount Path

### Agent Resources
- **[Workflow](.agent/workflow.md)** - Development workflow & quality gates
- **[Multi-Agent](.agent/multi-agent.md)** - Multi-agent coordination patterns
- **[Research-First](.agent/research-first.md)** - Research-first methodology
- **[MCP Usage](.agent/mcp-usage.md)** - MCP tool usage guide
- **[Cursor Workflow](.agent/cursor-workflow.md)** - Cursor-specific development workflow
- **[Taskmaster Reference](.agent/taskmaster-reference.md)** - Complete Taskmaster tool reference
- **[Multi-Agent Quickstart](.agent/multi-agent-quickstart.md)** - 15-minute deployment guide

### Project Status
- **[MIGRATION.md](MIGRATION.md)** - Configuration migration history

## Project Overview

**PRISM.k1** - ESP32-S3 LED controller firmware
- Hardware: ESP32-S3, 512KB RAM, 8MB Flash
- Stack: ESP-IDF v5.x, FreeRTOS, LittleFS
- Goals: <100ms pattern switch, 60 FPS, <150KB heap
- Critical: Memory safety, zero fragmentation, field reliability

## Technical Specifications

**⚠️ ALWAYS CHECK:** `.taskmaster/CANON.md` for:
- Partition table configuration
- Filesystem mount paths (`/littlefs` NOT `/prism`)
- WebSocket protocol (Binary TLV, NOT JSON)
- Memory allocations and budgets
- Error codes and constants
- ALL technical specifications

**CANON.md is auto-generated from approved ADRs. Never edit manually.**

## Knowledge Fortress Process

### Making Technical Decisions
1. **Research First** - Conduct reproducible research
2. **Captain Review** - Submit research for validation
3. **Create ADR** - Document decision with evidence
4. **Captain Approval** - Get ADR approved (becomes immutable)
5. **Update CANON** - Run `./scripts/generate-canon.sh`
6. **Sync Code** - Run `./scripts/sync-code-to-canon.sh`
7. **Validate** - Run `./scripts/validate-canon.sh`

### When Encountering Conflicts
1. **STOP** - Do not implement either specification
2. **Document** - Record both conflicting specifications
3. **Research** - Investigate which is correct
4. **Resolve** - Create conflict resolution ADR
5. **Update CANON** - CANON becomes new truth

See `.taskmaster/examples/example-conflict-resolution.md` for complete example.

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
# Knowledge Fortress scripts
./taskmaster/scripts/generate-canon.sh     # Update CANON from ADRs
./taskmaster/scripts/create-adr.sh         # Create new ADR
./taskmaster/scripts/validate-canon.sh     # Validate code matches CANON
./taskmaster/scripts/sync-code-to-canon.sh # Generate code from CANON

# Task management
task-master next              # Get next task
task-master show <id>         # View task details
task-master set-status --id=<id> --status=done

# Multi-agent deployment
./taskmaster setup            # Initialize multi-agent
./taskmaster dashboard        # Real-time monitoring
./taskmaster run-agent <id> <tag> <max>  # Start agent
```

## ⚠️ CRITICAL WARNINGS

1. **NEVER edit CANON.md manually** - Always regenerate from ADRs
2. **NEVER skip Captain review** - All research and ADRs require approval
3. **NEVER implement conflicting specs** - Resolve conflicts first
4. **NEVER make decisions without evidence** - Research first
5. **ALWAYS check CANON.md** - Single source of truth for all specs

## Deprecated Documentation

Old documentation has been moved to `.deprecated/pre-fortress/` for review.

**DO NOT USE:**
- `.deprecated/pre-fortress/docs/` - Old documentation system
- `.deprecated/pre-fortress/research-needs-validation/` - Unvalidated research

**Use Knowledge Fortress system in `.taskmaster/` instead.**

For complete documentation, see `.taskmaster/README.md`

## Task Master AI Instructions
**Import Task Master's development workflow commands and guidelines, treat as if import is in the main CLAUDE.md file.**
@./.taskmaster/CLAUDE.md

## Beads Agent Memory (bd)

We use Beads (`bd`) as the agent-first execution queue and durable memory alongside Task Master planning.

- Database: `.beads/prism.db` (auto-exported to `.beads/issues.jsonl` and synced via git)
- Use `--json` for programmatic flows; agents should prefer Beads over ad-hoc markdown checklists.

Quick commands:

```bash
# Learn the workflow
bd quickstart

# Find unblocked work and inspect details
bd ready
bd show <id>

# Work lifecycle
bd update <id> --status in_progress
bd create "<title>" -t task -p 2 --deps discovered-from:<parent-id>
bd close <id> --reason "Implemented"
```

Notes:
- Dependencies: use `blocks`, `parent-child`, `related`, `discovered-from` as appropriate.
- Auto-sync: changes export to `.beads/issues.jsonl`; pulls auto-import into the DB.
- MCP: If using Claude Desktop/other MCP clients, use the `beads` MCP server (command: `beads-mcp`).
