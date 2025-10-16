#!/bin/bash
# Multi-Agent Standalone System - Quick Setup
# Run this AFTER Knowledge Fortress Phase 1 is complete

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

echo -e "${BLUE}"
echo "╔══════════════════════════════════════════════════════════════╗"
echo "║    PRISM K1 Multi-Agent System - Standalone Setup           ║"
echo "╚══════════════════════════════════════════════════════════════╝"
echo -e "${NC}"

PROJECT_ROOT="/Users/spectrasynq/Workspace_Management/Software/PRISM.k1"
cd "$PROJECT_ROOT"

# ============================================================================
# Phase 0: Prerequisites Check
# ============================================================================

echo -e "${YELLOW}[0/4] Checking prerequisites...${NC}"

# Check if Knowledge Fortress exists
if [ ! -f ".taskmaster/CANON.md" ]; then
    echo -e "${RED}❌ Knowledge Fortress not set up${NC}"
    echo "   Please complete Knowledge Fortress Phase 1 first:"
    echo "   See: .taskmaster/continuity/QUICK_START.md"
    exit 1
fi

if [ ! -f ".taskmaster/agent-rules.yml" ]; then
    echo -e "${RED}❌ agent-rules.yml not found${NC}"
    echo "   Knowledge Fortress is incomplete"
    exit 1
fi

echo -e "${GREEN}✓ Knowledge Fortress found${NC}"
echo -e "${GREEN}✓ Prerequisites satisfied${NC}"
echo ""

# ============================================================================
# Phase 1: Directory Structure
# ============================================================================

echo -e "${YELLOW}[1/4] Creating directory structure...${NC}"

mkdir -p .multi-agent/{config,scripts,.locks,.messages,.file-locks,logs,metrics,web}
mkdir -p .multi-agent/scripts/hooks

echo -e "${GREEN}✓ Directory structure created${NC}"
echo ""

# ============================================================================
# Phase 2: Configuration Files
# ============================================================================

echo -e "${YELLOW}[2/4] Installing configuration files...${NC}"

# 2.1 Agent Capabilities
cat > .multi-agent/config/agents.yml << 'EOF'
# Multi-Agent System - Agent Capabilities
# Integrates with Knowledge Fortress (.taskmaster/)

version: "1.0.0"

agents:
  agent-1-network:
    name: "Network Specialist"
    skills: [websocket, wifi, protocols, mdns]
    max_concurrent_tasks: 2
    priority_boost: [high]
    reads_from:
      - .taskmaster/CANON.md
      - .taskmaster/decisions/ADR-*.md
    writes_to:
      - firmware/components/network/**
    validation_required: true
    
  agent-2-storage:
    name: "Storage Specialist"
    skills: [filesystem, littlefs, caching, compression]
    max_concurrent_tasks: 2
    priority_boost: [high, medium]
    reads_from:
      - .taskmaster/CANON.md
      - .taskmaster/decisions/ADR-*.md
    writes_to:
      - firmware/components/storage/**
    validation_required: true
    
  agent-3-playback:
    name: "Playback Specialist"
    skills: [led-driver, effects, animation, rmt-peripheral]
    max_concurrent_tasks: 1
    critical_path: true
    reads_from:
      - .taskmaster/CANON.md
      - .taskmaster/decisions/ADR-*.md
    writes_to:
      - firmware/components/playback/**
    validation_required: true
    
  agent-4-integration:
    name: "Integration Lead"
    skills: [testing, debugging, optimization, profiling]
    max_concurrent_tasks: 3
    blocker_resolver: true
    reads_from:
      - .taskmaster/CANON.md
      - .taskmaster/decisions/ADR-*.md
      - .multi-agent/metrics/**
    writes_to:
      - firmware/**
      - .taskmaster/research/[PROPOSED]/**
    validation_required: true

# Integration Rules
integration:
  canon_check: before-claim      # When to verify CANON sync
  validation: after-implement     # When to run validate-canon.sh
  max_retries: 3                  # Max retries on validation failure
  
  fortress_sync:
    auto: true                    # Auto-sync with Knowledge Fortress
    interval: 300                 # Check every 5 minutes
    on_canon_change: notify-all   # Notify all agents on CANON update
EOF

echo -e "${GREEN}✓ agents.yml created${NC}"

# 2.2 Routing Configuration
cat > .multi-agent/config/routing.yml << 'EOF'
# Multi-Agent System - Task Routing Configuration

strategy: skill-based-with-load-balancing

scoring:
  skill_match: 10       # Points per matching skill
  priority_boost: 5     # Points for priority match
  load_penalty: 2       # Penalty per active task
  critical_bonus: 15    # Bonus for critical path agents
  research_bonus: 8     # Bonus if task needs research

limits:
  max_load_imbalance: 1.5  # Rebalance if load > 1.5x average
  max_stale_time: 7200     # Auto-reset tasks older than 2 hours

fortress_integration:
  read_canon_on_claim: true         # Read CANON before claiming
  validate_before_push: true        # Validate against CANON before push
  cite_adrs_in_code: true          # Require ADR references in comments
EOF

echo -e "${GREEN}✓ routing.yml created${NC}"

# 2.3 Pipeline Configuration
cat > .multi-agent/config/pipeline.yml << 'EOF'
# Multi-Agent System - Build & Test Pipeline

stages:
  pre_integration:
    trigger: task-completed
    checks:
      - name: compile-check
        command: cd firmware && idf.py build --dry-run
        required: true
      - name: canon-validation
        command: .taskmaster/scripts/validate-canon.sh
        required: true
      - name: memory-estimate
        command: .multi-agent/scripts/estimate-memory.sh
        required: false
    
  integration:
    trigger: branch-merge
    checks:
      - name: full-build
        command: cd firmware && idf.py fullclean && idf.py build
        required: true
      - name: unit-tests
        command: cd firmware && idf.py test
        required: false
    
  post_integration:
    trigger: tests-passed
    actions:
      - name: update-docs
        command: .multi-agent/scripts/generate-docs.sh
      - name: collect-metrics
        command: .multi-agent/scripts/collect-metrics.sh

auto_rollback:
  enabled: true
  on_failure: [build-failure, validation-failure]
  preserve_logs: true
  notify: captain
EOF

echo -e "${GREEN}✓ pipeline.yml created${NC}"
echo ""

# ============================================================================
# Phase 3: Core Scripts
# ============================================================================

echo -e "${YELLOW}[3/4] Installing core scripts...${NC}"

# 3.1 Fortress Sync Script
cat > .multi-agent/scripts/sync-with-fortress.sh << 'EOF'
#!/bin/bash
# Sync Multi-Agent with Knowledge Fortress

set -e

PROJECT_ROOT="/Users/spectrasynq/Workspace_Management/Software/PRISM.k1"
cd "$PROJECT_ROOT"

# Check CANON freshness
if [ ! -f ".taskmaster/CANON.md" ]; then
    echo "❌ CANON.md not found. Knowledge Fortress incomplete."
    exit 1
fi

CANON_HASH=$(shasum -a 256 .taskmaster/CANON.md | cut -d' ' -f1)
CACHED_HASH=$(cat .multi-agent/.canon-hash 2>/dev/null || echo "none")

if [ "$CANON_HASH" != "$CACHED_HASH" ]; then
    echo "📜 CANON updated (hash: ${CANON_HASH:0:8}...)"
    
    # Invalidate any active claims (agents must re-read CANON)
    if [ -d ".multi-agent/.locks" ]; then
        ACTIVE_CLAIMS=$(find .multi-agent/.locks -name "*.claim" 2>/dev/null | wc -l)
        if [ $ACTIVE_CLAIMS -gt 0 ]; then
            echo "⚠️  $ACTIVE_CLAIMS active tasks - agents will be notified"
        fi
    fi
    
    # Broadcast update to all agents
    if [ -x ".multi-agent/scripts/message-bus.sh" ]; then
        .multi-agent/scripts/message-bus.sh post \
            "system" "all" "canon-updated" \
            "CANON.md changed. Re-read specifications before claiming new tasks."
    fi
    
    # Update cached hash
    echo "$CANON_HASH" > .multi-agent/.canon-hash
    
    echo "✅ Fortress sync complete"
    return 0
else
    echo "✓ CANON up to date"
    return 0
fi
EOF

chmod +x .multi-agent/scripts/sync-with-fortress.sh
echo -e "${GREEN}✓ sync-with-fortress.sh installed${NC}"

# 3.2 Pre-Claim Hook
cat > .multi-agent/scripts/hooks/pre-claim.sh << 'EOF'
#!/bin/bash
# Pre-Claim Hook - Run before agent claims task

set -e

TASK_ID=$1
AGENT_ID=$2

echo "🔍 Pre-claim validation for task $TASK_ID by $AGENT_ID..."

# 1. Sync with Knowledge Fortress
cd ../..
if ! .multi-agent/scripts/sync-with-fortress.sh; then
    echo "❌ Failed to sync with Knowledge Fortress"
    exit 1
fi

# 2. Validate current code matches CANON (if validate script exists)
if [ -x ".taskmaster/scripts/validate-canon.sh" ]; then
    cd firmware
    if ! ../.taskmaster/scripts/validate-canon.sh 2>&1 | tail -10; then
        echo "❌ Code does not match CANON. Fix drift before claiming tasks."
        cd ..
        exit 1
    fi
    cd ..
fi

# 3. Check for file locks on files this task will modify
# (This would parse task description for file paths)

echo "✅ Pre-claim validation passed"
EOF

chmod +x .multi-agent/scripts/hooks/pre-claim.sh
echo -e "${GREEN}✓ pre-claim.sh hook installed${NC}"

# 3.3 Conflict Guard
cat > .multi-agent/scripts/conflict-guard.sh << 'EOF'
#!/bin/bash
# Conflict Guard - Prevent file-level conflicts

check_file_locks() {
    local AGENT_ID=$1
    shift
    local FILES=("$@")
    
    for FILE in "${FILES[@]}"; do
        local LOCK_FILE=".multi-agent/.file-locks/$(echo $FILE | tr '/' '_').lock"
        
        if [ -f "$LOCK_FILE" ]; then
            local OWNER=$(cat "$LOCK_FILE" | cut -d: -f1)
            if [ "$OWNER" != "$AGENT_ID" ]; then
                echo "CONFLICT: File $FILE locked by $OWNER"
                return 1
            fi
        fi
    done
    
    # Claim files
    mkdir -p .multi-agent/.file-locks
    for FILE in "${FILES[@]}"; do
        local LOCK_FILE=".multi-agent/.file-locks/$(echo $FILE | tr '/' '_').lock"
        echo "$AGENT_ID:$(date -u +%Y-%m-%dT%H:%M:%SZ)" > "$LOCK_FILE"
    done
    
    echo "✅ Files locked successfully"
    return 0
}

release_file_locks() {
    local AGENT_ID=$1
    
    find .multi-agent/.file-locks -type f 2>/dev/null | while read LOCK; do
        if grep -q "^${AGENT_ID}:" "$LOCK" 2>/dev/null; then
            rm "$LOCK"
        fi
    done
    
    echo "✅ File locks released"
}

# CLI
case "$1" in
    check) shift; check_file_locks "$@" ;;
    release) release_file_locks "$2" ;;
    *) echo "Usage: conflict-guard.sh check|release <agent-id> [files...]" ;;
esac
EOF

chmod +x .multi-agent/scripts/conflict-guard.sh
echo -e "${GREEN}✓ conflict-guard.sh installed${NC}"

# 3.4 Message Bus
cat > .multi-agent/scripts/message-bus.sh << 'EOF'
#!/bin/bash
# Message Bus - Inter-agent communication

post_message() {
    local FROM=$1
    local TO=$2
    local TYPE=$3
    local MESSAGE=$4
    
    mkdir -p .multi-agent/.messages
    local MSG_FILE=".multi-agent/.messages/$(date +%s)-${FROM}-${TO}.json"
    
    cat > "$MSG_FILE" <<ENDMSG
{
    "from": "$FROM",
    "to": "$TO",
    "type": "$TYPE",
    "message": "$MESSAGE",
    "timestamp": "$(date -u +%Y-%m-%dT%H:%M:%SZ)",
    "read": false
}
ENDMSG
    
    echo "✉️  Message sent: $FROM → $TO"
}

check_messages() {
    local AGENT_ID=$1
    
    find .multi-agent/.messages -name "*-${AGENT_ID}.json" -o -name "*-all.json" 2>/dev/null | \
        while read MSG; do
            if [ -f "$MSG" ] && command -v jq >/dev/null 2>&1; then
                if [ "$(jq -r .read "$MSG" 2>/dev/null)" = "false" ]; then
                    echo "📬 New message:"
                    jq . "$MSG"
                    
                    # Mark as read
                    local TMP=$(mktemp)
                    jq '.read = true' "$MSG" > "$TMP" && mv "$TMP" "$MSG"
                fi
            fi
        done
}

# CLI
case "$1" in
    post) post_message "$2" "$3" "$4" "$5" ;;
    check) check_messages "$2" ;;
    *) echo "Usage: message-bus.sh post|check <args>" ;;
esac
EOF

chmod +x .multi-agent/scripts/message-bus.sh
echo -e "${GREEN}✓ message-bus.sh installed${NC}"

# 3.5 Metrics Collection
cat > .multi-agent/scripts/collect-metrics.sh << 'EOF'
#!/bin/bash
# Collect Multi-Agent Metrics

mkdir -p .multi-agent/metrics

TODAY=$(date +%Y%m%d)
METRICS_FILE=".multi-agent/metrics/${TODAY}.json"

# Count completed tasks
COMPLETED=$(find .multi-agent/.locks -name "*.completed-*" -mtime -1 2>/dev/null | wc -l | tr -d ' ')

# Calculate average time (simplified)
AVG_TIME="TODO"

# Count violations
VIOLATIONS=$(grep -r "VIOLATION" .multi-agent/logs/*.log 2>/dev/null | wc -l | tr -d ' ')

cat > "$METRICS_FILE" <<ENDMETRICS
{
  "date": "$(date -u +%Y-%m-%dT%H:%M:%SZ)",
  "tasks_completed": $COMPLETED,
  "average_completion_time": "$AVG_TIME",
  "canon_violations": $VIOLATIONS,
  "build_failures": 0,
  "agents_active": $(find .multi-agent/.locks -name "*.claim" 2>/dev/null | wc -l | tr -d ' ')
}
ENDMETRICS

echo "📊 Metrics collected: $METRICS_FILE"
EOF

chmod +x .multi-agent/scripts/collect-metrics.sh
echo -e "${GREEN}✓ collect-metrics.sh installed${NC}"

echo ""

# ============================================================================
# Phase 4: Documentation & Finalization
# ============================================================================

echo -e "${YELLOW}[4/4] Creating documentation...${NC}"

cat > .multi-agent/README.md << 'EOF'
# Multi-Agent Execution System
## Standalone Integration with Knowledge Fortress

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

## Integration Rules

**Multi-agent READS (read-only):**
- `.taskmaster/CANON.md` - Current specifications
- `.taskmaster/decisions/*.md` - Decision context
- `.taskmaster/agent-rules.yml` - Governance rules

**Multi-agent WRITES:**
- `firmware/**/*` - Code implementations
- `.multi-agent/logs/*` - Execution logs
- `.multi-agent/metrics/*` - Performance data

**Multi-agent NEVER modifies:**
- `.taskmaster/decisions/*` - ADRs (immutable, Captain-owned)
- `.taskmaster/CANON.md` - Auto-generated, script-owned
- `.taskmaster/agent-rules.yml` - Governance rules

## Quick Start

```bash
# 1. Verify Knowledge Fortress is set up
ls -la .taskmaster/CANON.md

# 2. Sync with fortress
.multi-agent/scripts/sync-with-fortress.sh

# 3. Check agent configuration
cat .multi-agent/config/agents.yml
```

## Key Commands

```bash
# Sync with Knowledge Fortress
.multi-agent/scripts/sync-with-fortress.sh

# Check for messages
.multi-agent/scripts/message-bus.sh check agent-1-network

# Lock files before modifying
.multi-agent/scripts/conflict-guard.sh check agent-1 \
    firmware/main/main.c firmware/components/network/websocket.c

# Release locks
.multi-agent/scripts/conflict-guard.sh release agent-1

# Collect metrics
.multi-agent/scripts/collect-metrics.sh
```

## Workflow

1. **Agent claims task** from taskmaster
2. **Pre-claim hook** runs (sync fortress, validate code)
3. **Agent reads CANON** for specifications
4. **Agent implements** feature in firmware/
5. **Post-implementation** validation against CANON
6. **Agent commits** code (if validation passes)
7. **Metrics collected** for future ADRs

## Validation

Every agent operation validates against CANON:

```bash
# Before claiming task
.multi-agent/scripts/hooks/pre-claim.sh <task-id> <agent-id>

# After implementing
.taskmaster/scripts/validate-canon.sh

# Before pushing
.githooks/pre-push
```

## Monitoring

```bash
# View active agents
find .multi-agent/.locks -name "*.claim"

# Check fortress sync status
cat .multi-agent/.canon-hash

# View recent metrics
cat .multi-agent/metrics/$(date +%Y%m%d).json

# Check agent messages
.multi-agent/scripts/message-bus.sh check all
```

## Troubleshooting

**Problem: Code doesn't match CANON**
```bash
# Sync and validate
.multi-agent/scripts/sync-with-fortress.sh
cd firmware && ../.taskmaster/scripts/validate-canon.sh
```

**Problem: Agent can't claim task**
```bash
# Check pre-claim validation
.multi-agent/scripts/hooks/pre-claim.sh <task-id> <agent-id>
```

**Problem: CANON changed during implementation**
```bash
# Re-sync immediately
.multi-agent/scripts/sync-with-fortress.sh
# Re-read specifications before continuing
```

## Security

- Knowledge Fortress files are READ-ONLY for agents
- File permissions enforce separation
- Git hooks prevent accidental modifications
- All changes validated against CANON

## Support

For questions about:
- **Specifications:** Check `.taskmaster/CANON.md`
- **Decisions:** See `.taskmaster/decisions/ADR-*.md`
- **Governance:** Read `.taskmaster/agent-rules.yml`
- **Execution issues:** Check `.multi-agent/logs/`

---

**System Status:** READY FOR USE  
**Version:** 1.0.0  
**Last Updated:** 2025-10-16
EOF

echo -e "${GREEN}✓ README.md created${NC}"

# Create empty .canon-hash
touch .multi-agent/.canon-hash

echo ""
echo -e "${BLUE}╔══════════════════════════════════════════════════════════════╗${NC}"
echo -e "${BLUE}║              Setup Complete! ✅                              ║${NC}"
echo -e "${BLUE}╚══════════════════════════════════════════════════════════════╝${NC}"
echo ""
echo -e "${GREEN}Multi-Agent System installed successfully!${NC}"
echo ""
echo "📁 Structure created:"
echo "   .multi-agent/"
echo "   ├── config/          (agent capabilities, routing rules)"
echo "   ├── scripts/         (core automation)"
echo "   ├── .locks/          (task claims)"
echo "   └── logs/            (execution logs)"
echo ""
echo "📋 Next steps:"
echo ""
echo "1. Verify fortress integration:"
echo "   .multi-agent/scripts/sync-with-fortress.sh"
echo ""
echo "2. Test pre-claim validation:"
echo "   .multi-agent/scripts/hooks/pre-claim.sh test-task agent-1"
echo ""
echo "3. Read the documentation:"
echo "   cat .multi-agent/README.md"
echo ""
echo "4. Review configuration:"
echo "   cat .multi-agent/config/agents.yml"
echo ""
echo "🎯 The multi-agent system is now ready to coordinate agent execution"
echo "   while respecting Knowledge Fortress governance!"
echo ""
