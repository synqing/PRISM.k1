#!/bin/bash
# Multi-Agent System - Installation Verification

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

echo -e "${BLUE}"
echo "╔══════════════════════════════════════════════════════════════╗"
echo "║    Multi-Agent System - Installation Verification           ║"
echo "╚══════════════════════════════════════════════════════════════╝"
echo -e "${NC}"
echo ""

PROJECT_ROOT="/Users/spectrasynq/Workspace_Management/Software/PRISM.k1"
cd "$PROJECT_ROOT"

PASS=0
FAIL=0

check() {
    local NAME=$1
    local TEST=$2
    
    if eval "$TEST" > /dev/null 2>&1; then
        echo -e "${GREEN}✓${NC} $NAME"
        ((PASS++))
    else
        echo -e "${RED}✗${NC} $NAME"
        ((FAIL++))
    fi
}

echo "Directory Structure:"
check "Main directory exists" "[ -d .multi-agent ]"
check "Config directory exists" "[ -d .multi-agent/config ]"
check "Scripts directory exists" "[ -d .multi-agent/scripts ]"
check "Hooks directory exists" "[ -d .multi-agent/scripts/hooks ]"
check "Locks directory exists" "[ -d .multi-agent/.locks ]"
check "Messages directory exists" "[ -d .multi-agent/.messages ]"
check "File-locks directory exists" "[ -d .multi-agent/.file-locks ]"
check "Logs directory exists" "[ -d .multi-agent/logs ]"
check "Metrics directory exists" "[ -d .multi-agent/metrics ]"
echo ""

echo "Configuration Files:"
check "agents.yml exists" "[ -f .multi-agent/config/agents.yml ]"
check "routing.yml exists" "[ -f .multi-agent/config/routing.yml ]"
check "pipeline.yml exists" "[ -f .multi-agent/config/pipeline.yml ]"
echo ""

echo "Core Scripts:"
check "sync-with-fortress.sh exists" "[ -f .multi-agent/scripts/sync-with-fortress.sh ]"
check "agent-executor.sh exists" "[ -f .multi-agent/scripts/agent-executor.sh ]"
check "conflict-guard.sh exists" "[ -f .multi-agent/scripts/conflict-guard.sh ]"
check "message-bus.sh exists" "[ -f .multi-agent/scripts/message-bus.sh ]"
check "collect-metrics.sh exists" "[ -f .multi-agent/scripts/collect-metrics.sh ]"
check "smart-router.js exists" "[ -f .multi-agent/scripts/smart-router.js ]"
echo ""

echo "Hooks:"
check "pre-claim.sh exists" "[ -f .multi-agent/scripts/hooks/pre-claim.sh ]"
echo ""

echo "Documentation:"
check "README.md exists" "[ -f .multi-agent/README.md ]"
check ".canon-hash exists" "[ -f .multi-agent/.canon-hash ]"
echo ""

echo "Knowledge Fortress Integration:"
check "CANON.md exists" "[ -f .taskmaster/CANON.md ]"
check "agent-rules.yml exists" "[ -f .taskmaster/agent-rules.yml ]"
check "tasks.json exists" "[ -f .taskmaster/tasks/tasks.json ]"
echo ""

echo "Script Permissions:"
check "sync-with-fortress.sh executable" "[ -x .multi-agent/scripts/sync-with-fortress.sh ]"
check "agent-executor.sh executable" "[ -x .multi-agent/scripts/agent-executor.sh ]"
check "conflict-guard.sh executable" "[ -x .multi-agent/scripts/conflict-guard.sh ]"
check "message-bus.sh executable" "[ -x .multi-agent/scripts/message-bus.sh ]"
check "collect-metrics.sh executable" "[ -x .multi-agent/scripts/collect-metrics.sh ]"
check "smart-router.js executable" "[ -x .multi-agent/scripts/smart-router.js ]"
check "pre-claim.sh executable" "[ -x .multi-agent/scripts/hooks/pre-claim.sh ]"
echo ""

echo "Functional Tests:"

# Test fortress sync
if .multi-agent/scripts/sync-with-fortress.sh > /dev/null 2>&1; then
    echo -e "${GREEN}✓${NC} Fortress sync works"
    ((PASS++))
else
    echo -e "${RED}✗${NC} Fortress sync failed"
    ((FAIL++))
fi

# Test message bus
if .multi-agent/scripts/message-bus.sh post system test test "Installation test" > /dev/null 2>&1; then
    echo -e "${GREEN}✓${NC} Message bus works"
    ((PASS++))
    rm -f .multi-agent/.messages/*-test.json 2>/dev/null
else
    echo -e "${RED}✗${NC} Message bus failed"
    ((FAIL++))
fi

# Test conflict guard
if .multi-agent/scripts/conflict-guard.sh check test-agent test-file.c > /dev/null 2>&1; then
    echo -e "${GREEN}✓${NC} Conflict guard works"
    ((PASS++))
    .multi-agent/scripts/conflict-guard.sh release test-agent > /dev/null 2>&1
else
    echo -e "${RED}✗${NC} Conflict guard failed"
    ((FAIL++))
fi

# Test metrics collection
if .multi-agent/scripts/collect-metrics.sh > /dev/null 2>&1; then
    echo -e "${GREEN}✓${NC} Metrics collection works"
    ((PASS++))
else
    echo -e "${RED}✗${NC} Metrics collection failed"
    ((FAIL++))
fi

# Test smart router (requires node)
if command -v node &> /dev/null; then
    if node .multi-agent/scripts/smart-router.js status > /dev/null 2>&1; then
        echo -e "${GREEN}✓${NC} Smart router works"
        ((PASS++))
    else
        echo -e "${RED}✗${NC} Smart router failed"
        ((FAIL++))
    fi
else
    echo -e "${YELLOW}⚠${NC} Node.js not found, skipping smart router test"
fi

echo ""
echo "═══════════════════════════════════════════════════════════════"
echo -e "${GREEN}Passed: $PASS${NC} | ${RED}Failed: $FAIL${NC}"
echo "═══════════════════════════════════════════════════════════════"
echo ""

if [ $FAIL -eq 0 ]; then
    echo -e "${GREEN}✅ All checks passed! Multi-Agent System is ready.${NC}"
    echo ""
    echo "Next steps:"
    echo "1. Read: cat .multi-agent/README.md"
    echo "2. Test task execution: .multi-agent/scripts/agent-executor.sh --help"
    echo "3. Check agent status: node .multi-agent/scripts/smart-router.js status"
    echo ""
    exit 0
else
    echo -e "${RED}❌ Some checks failed. Please review errors above.${NC}"
    echo ""
    exit 1
fi
