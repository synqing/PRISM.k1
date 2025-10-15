#!/bin/bash
# PRISM.k1 Multi-Agent Setup Verification Script
# Tests all components of the multi-agent configuration

set -e  # Exit on any error

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Test counters
PASSED=0
FAILED=0
WARNINGS=0

# Helper functions
print_header() {
    echo ""
    echo "═══════════════════════════════════════════════════════════════"
    echo "  $1"
    echo "═══════════════════════════════════════════════════════════════"
    echo ""
}

print_test() {
    echo -n "Testing: $1 ... "
}

pass() {
    echo -e "${GREEN}✓ PASS${NC}"
    ((PASSED++))
}

fail() {
    echo -e "${RED}✗ FAIL${NC}"
    echo -e "${RED}  Error: $1${NC}"
    ((FAILED++))
}

warn() {
    echo -e "${YELLOW}⚠ WARNING${NC}"
    echo -e "${YELLOW}  Warning: $1${NC}"
    ((WARNINGS++))
}

info() {
    echo -e "${BLUE}ℹ INFO:${NC} $1"
}

# Start tests
echo ""
echo "╔═══════════════════════════════════════════════════════════════╗"
echo "║     PRISM.k1 Multi-Agent Setup Verification Script           ║"
echo "╚═══════════════════════════════════════════════════════════════╝"
echo ""
echo "Testing Date: $(date)"
echo "Project Root: $(pwd)"
echo ""

# =============================================================================
# SECTION 1: Directory Structure
# =============================================================================

print_header "1. Directory Structure"

print_test "Project root exists"
if [ -d "/Users/spectrasynq/Workspace_Management/Software/PRISM.k1" ]; then
    pass
else
    fail "Project root not found"
fi

print_test ".agent/ directory exists"
if [ -d ".agent" ]; then
    pass
else
    fail ".agent/ directory missing"
fi

print_test ".taskmaster/ directory exists"
if [ -d ".taskmaster" ]; then
    pass
else
    fail ".taskmaster/ directory missing"
fi

print_test ".taskmaster/scripts/ directory exists"
if [ -d ".taskmaster/scripts" ]; then
    pass
else
    fail ".taskmaster/scripts/ directory missing"
fi

print_test ".taskmaster/locks/ directory (will be created on first run)"
if [ -d ".taskmaster/.locks" ]; then
    pass
    info "Locks directory already exists"
else
    warn "Will be created on first agent run"
fi

print_test ".taskmaster/logs/ directory (will be created on first run)"
if [ -d ".taskmaster/logs" ]; then
    pass
    info "Logs directory already exists"
else
    warn "Will be created on first agent run"
fi

print_test ".taskmaster/metrics/ directory (will be created on first run)"
if [ -d ".taskmaster/metrics" ]; then
    pass
    info "Metrics directory already exists"
else
    warn "Will be created on first agent run"
fi

# =============================================================================
# SECTION 2: Core Files
# =============================================================================

print_header "2. Core Configuration Files"

print_test "CLAUDE.md exists"
if [ -f "CLAUDE.md" ]; then
    pass
else
    fail "CLAUDE.md missing"
fi

print_test "AGENT.md exists"
if [ -f "AGENT.md" ]; then
    pass
else
    fail "AGENT.md missing"
fi

print_test ".agent/instructions.md exists"
if [ -f ".agent/instructions.md" ]; then
    pass
else
    fail ".agent/instructions.md missing"
fi

print_test ".agent/workflow.md exists"
if [ -f ".agent/workflow.md" ]; then
    pass
else
    fail ".agent/workflow.md missing"
fi

print_test ".agent/multi-agent.md exists"
if [ -f ".agent/multi-agent.md" ]; then
    pass
else
    fail ".agent/multi-agent.md missing"
fi

print_test ".agent/multi-agent-quickstart.md exists"
if [ -f ".agent/multi-agent-quickstart.md" ]; then
    pass
else
    fail ".agent/multi-agent-quickstart.md missing"
fi

print_test ".agent/research-first.md exists"
if [ -f ".agent/research-first.md" ]; then
    pass
else
    fail ".agent/research-first.md missing"
fi

print_test ".agent/mcp-usage.md exists"
if [ -f ".agent/mcp-usage.md" ]; then
    pass
else
    fail ".agent/mcp-usage.md missing"
fi

print_test ".agent/cursor-workflow.md exists"
if [ -f ".agent/cursor-workflow.md" ]; then
    pass
else
    fail ".agent/cursor-workflow.md missing"
fi

print_test ".agent/taskmaster-reference.md exists"
if [ -f ".agent/taskmaster-reference.md" ]; then
    pass
else
    fail ".agent/taskmaster-reference.md missing"
fi

print_test "MIGRATION.md exists"
if [ -f "MIGRATION.md" ]; then
    pass
else
    fail "MIGRATION.md missing"
fi

print_test "STATUS.md exists"
if [ -f "STATUS.md" ]; then
    pass
else
    fail "STATUS.md missing"
fi

# =============================================================================
# SECTION 3: Multi-Agent Scripts
# =============================================================================

print_header "3. Multi-Agent Scripts"

print_test ".taskmaster/scripts/multi-agent.sh exists"
if [ -f ".taskmaster/scripts/multi-agent.sh" ]; then
    pass
else
    fail "multi-agent.sh script missing"
fi

print_test "multi-agent.sh is executable"
if [ -x ".taskmaster/scripts/multi-agent.sh" ]; then
    pass
else
    fail "multi-agent.sh is not executable (run: chmod +x .taskmaster/scripts/multi-agent.sh)"
fi

print_test "taskmaster symlink exists"
if [ -L "taskmaster" ]; then
    pass
else
    warn "Convenience symlink missing (run: ln -sf .taskmaster/scripts/multi-agent.sh taskmaster)"
fi

print_test "taskmaster symlink points to correct location"
if [ -L "taskmaster" ]; then
    TARGET=$(readlink "taskmaster")
    if [[ "$TARGET" == *"multi-agent.sh"* ]]; then
        pass
    else
        fail "Symlink points to wrong location: $TARGET"
    fi
else
    warn "Skipped - symlink doesn't exist"
fi

print_test "multi-agent.sh script syntax is valid"
if bash -n ".taskmaster/scripts/multi-agent.sh" 2>/dev/null; then
    pass
else
    fail "Script has syntax errors"
fi

# =============================================================================
# SECTION 4: MCP Configuration
# =============================================================================

print_header "4. MCP Server Configuration"

print_test ".mcp.json exists"
if [ -f ".mcp.json" ]; then
    pass
else
    fail ".mcp.json missing"
fi

print_test ".cursor/mcp.json exists"
if [ -f ".cursor/mcp.json" ]; then
    pass
else
    warn ".cursor/mcp.json missing (optional for Cursor users)"
fi

print_test ".vscode/mcp.json exists"
if [ -f ".vscode/mcp.json" ]; then
    pass
else
    warn ".vscode/mcp.json missing (optional for VSCode users)"
fi

print_test ".vscode/settings.json exists"
if [ -f ".vscode/settings.json" ]; then
    pass
else
    warn ".vscode/settings.json missing (optional for Amp users)"
fi

print_test "opencode.json exists"
if [ -f "opencode.json" ]; then
    pass
else
    warn "opencode.json missing (optional for OpenCode users)"
fi

# Check MCP server definitions
print_test "MCP configuration contains task-master-ai"
if grep -q "task-master-ai" .mcp.json 2>/dev/null; then
    pass
else
    fail "task-master-ai not found in .mcp.json"
fi

print_test "MCP configuration contains filesystem"
if grep -q "filesystem" .mcp.json 2>/dev/null; then
    pass
else
    fail "filesystem not found in .mcp.json"
fi

print_test "MCP configuration contains context7"
if grep -q "context7" .mcp.json 2>/dev/null; then
    pass
else
    fail "context7 not found in .mcp.json"
fi

print_test "MCP configuration contains brave-search"
if grep -q "brave-search" .mcp.json 2>/dev/null; then
    pass
else
    fail "brave-search not found in .mcp.json"
fi

print_test "MCP configuration contains sequential-thinking"
if grep -q "sequential-thinking" .mcp.json 2>/dev/null; then
    pass
else
    fail "sequential-thinking not found in .mcp.json"
fi

print_test "MCP configuration contains memory"
if grep -q "memory" .mcp.json 2>/dev/null; then
    pass
else
    fail "memory not found in .mcp.json"
fi

print_test "MCP configuration contains git"
if grep -q "\"git\"" .mcp.json 2>/dev/null; then
    pass
else
    fail "git not found in .mcp.json"
fi

print_test "MCP configuration contains sqlite"
if grep -q "sqlite" .mcp.json 2>/dev/null; then
    pass
else
    fail "sqlite not found in .mcp.json"
fi

# =============================================================================
# SECTION 5: Taskmaster Configuration
# =============================================================================

print_header "5. Taskmaster Configuration"

print_test ".taskmaster/config.json exists"
if [ -f ".taskmaster/config.json" ]; then
    pass
else
    fail ".taskmaster/config.json missing (run: task-master models --setup)"
fi

print_test ".taskmaster/tasks/tasks.json exists"
if [ -f ".taskmaster/tasks/tasks.json" ]; then
    pass
else
    warn "No tasks.json yet (normal for new project)"
fi

print_test "Taskmaster CLI is accessible"
if command -v task-master &> /dev/null; then
    pass
    info "Version: $(task-master --version 2>/dev/null || echo 'unknown')"
else
    warn "task-master CLI not in PATH (install: npm install -g task-master-ai)"
fi

# Check for API keys (don't display them)
print_test "Environment variables or .env file exists"
if [ -f ".env" ]; then
    pass
    info ".env file found"
else
    warn "No .env file (API keys should be in MCP config or environment)"
fi

# =============================================================================
# SECTION 6: Firmware Structure
# =============================================================================

print_header "6. ESP-IDF Firmware Structure"

print_test "firmware/ directory exists"
if [ -d "firmware" ]; then
    pass
else
    warn "firmware/ directory missing (expected for ESP32 project)"
fi

print_test "firmware/CMakeLists.txt exists"
if [ -f "firmware/CMakeLists.txt" ]; then
    pass
else
    warn "firmware/CMakeLists.txt missing (build verification will be skipped)"
fi

print_test "firmware/components/ directory exists"
if [ -d "firmware/components" ]; then
    pass
else
    warn "firmware/components/ directory missing"
fi

print_test "ESP-IDF environment is available"
if command -v idf.py &> /dev/null; then
    pass
    info "ESP-IDF detected"
else
    warn "idf.py not in PATH (ESP-IDF environment may not be loaded)"
fi

# =============================================================================
# SECTION 7: Agent Capabilities Configuration
# =============================================================================

print_header "7. Agent Capabilities Configuration"

print_test ".taskmaster/agent-capabilities.json exists"
if [ -f ".taskmaster/agent-capabilities.json" ]; then
    pass
else
    warn "Agent capabilities config missing (will be created)"
fi

# =============================================================================
# SECTION 8: Git Configuration
# =============================================================================

print_header "8. Git Repository"

print_test "Git repository initialized"
if [ -d ".git" ]; then
    pass
else
    warn "Not a git repository (version control recommended)"
fi

print_test ".gitignore exists"
if [ -f ".gitignore" ]; then
    pass
else
    warn ".gitignore missing (recommended to ignore .taskmaster/.locks, logs, etc.)"
fi

# =============================================================================
# SECTION 9: Multi-Agent Script Functions
# =============================================================================

print_header "9. Multi-Agent Script Functions"

print_test "allocate_task function exists in script"
if grep -q "allocate_task()" .taskmaster/scripts/multi-agent.sh 2>/dev/null; then
    pass
else
    fail "allocate_task function missing"
fi

print_test "release_task function exists in script"
if grep -q "release_task()" .taskmaster/scripts/multi-agent.sh 2>/dev/null; then
    pass
else
    fail "release_task function missing"
fi

print_test "health_check function exists in script"
if grep -q "health_check()" .taskmaster/scripts/multi-agent.sh 2>/dev/null; then
    pass
else
    fail "health_check function missing"
fi

print_test "run_agent function exists in script"
if grep -q "run_agent()" .taskmaster/scripts/multi-agent.sh 2>/dev/null; then
    pass
else
    fail "run_agent function missing"
fi

print_test "execute_task function exists in script"
if grep -q "execute_task()" .taskmaster/scripts/multi-agent.sh 2>/dev/null; then
    pass
else
    fail "execute_task function missing"
fi

print_test "verify_task function exists in script"
if grep -q "verify_task()" .taskmaster/scripts/multi-agent.sh 2>/dev/null; then
    pass
else
    fail "verify_task function missing"
fi

print_test "show_dashboard function exists in script"
if grep -q "show_dashboard()" .taskmaster/scripts/multi-agent.sh 2>/dev/null; then
    pass
else
    fail "show_dashboard function missing"
fi

print_test "collect_metrics function exists in script"
if grep -q "collect_metrics()" .taskmaster/scripts/multi-agent.sh 2>/dev/null; then
    pass
else
    fail "collect_metrics function missing"
fi

print_test "setup_multi_agent function exists in script"
if grep -q "setup_multi_agent()" .taskmaster/scripts/multi-agent.sh 2>/dev/null; then
    pass
else
    fail "setup_multi_agent function missing"
fi

# =============================================================================
# SECTION 10: System Dependencies
# =============================================================================

print_header "10. System Dependencies"

print_test "bash is available"
if command -v bash &> /dev/null; then
    pass
    info "Version: $(bash --version | head -1)"
else
    fail "bash not found"
fi

print_test "flock utility is available"
if command -v flock &> /dev/null; then
    pass
else
    fail "flock not found (required for atomic locking)"
fi

print_test "tmux is available (optional)"
if command -v tmux &> /dev/null; then
    pass
    info "Version: $(tmux -V)"
else
    warn "tmux not found (recommended for multi-agent orchestration)"
fi

print_test "jq is available (optional)"
if command -v jq &> /dev/null; then
    pass
    info "Version: $(jq --version)"
else
    warn "jq not found (useful for JSON parsing)"
fi

print_test "grep is available"
if command -v grep &> /dev/null; then
    pass
else
    fail "grep not found"
fi

print_test "Node.js/npm is available (for taskmaster)"
if command -v node &> /dev/null; then
    pass
    info "Node: $(node --version), npm: $(npm --version)"
else
    warn "Node.js not found (required for task-master-ai)"
fi

# =============================================================================
# SECTION 11: Quick Functional Tests
# =============================================================================

print_header "11. Functional Tests"

print_test "Multi-agent script help command works"
if ./.taskmaster/scripts/multi-agent.sh help &>/dev/null; then
    pass
else
    fail "multi-agent.sh help command failed"
fi

print_test "Taskmaster health-check simulation (dry run)"
if [ -x ".taskmaster/scripts/multi-agent.sh" ]; then
    # Just test that the function exists without running it
    if grep -q "health_check()" .taskmaster/scripts/multi-agent.sh; then
        pass
    else
        fail "health_check function not found"
    fi
else
    warn "Skipped - script not executable"
fi

# =============================================================================
# SUMMARY
# =============================================================================

print_header "Test Summary"

TOTAL=$((PASSED + FAILED + WARNINGS))

echo ""
echo "Results:"
echo "--------"
echo -e "${GREEN}Passed:   $PASSED${NC}"
echo -e "${RED}Failed:   $FAILED${NC}"
echo -e "${YELLOW}Warnings: $WARNINGS${NC}"
echo "--------"
echo "Total:    $TOTAL"
echo ""

# Calculate percentage
if [ $TOTAL -gt 0 ]; then
    PERCENT=$((PASSED * 100 / TOTAL))
    echo "Success Rate: ${PERCENT}%"
    echo ""
fi

# Final verdict
if [ $FAILED -eq 0 ]; then
    echo "╔═══════════════════════════════════════════════════════════════╗"
    echo -e "║  ${GREEN}✓ ALL CRITICAL TESTS PASSED${NC}                                  ║"
    echo "╚═══════════════════════════════════════════════════════════════╝"
    echo ""
    if [ $WARNINGS -gt 0 ]; then
        echo -e "${YELLOW}Note: Some optional features are missing but system is operational.${NC}"
        echo ""
    fi
    echo "Next Steps:"
    echo "1. Restart your IDE to load MCP servers"
    echo "2. Run: ./taskmaster setup"
    echo "3. Launch agents: ./taskmaster run-agent <agent-id> <tag> <max-tasks>"
    echo "4. Monitor: ./taskmaster dashboard"
    echo ""
    exit 0
else
    echo "╔═══════════════════════════════════════════════════════════════╗"
    echo -e "║  ${RED}✗ SOME TESTS FAILED - REVIEW ERRORS ABOVE${NC}                   ║"
    echo "╚═══════════════════════════════════════════════════════════════╝"
    echo ""
    echo "Please fix the failed tests before deploying multi-agent system."
    echo ""
    exit 1
fi
