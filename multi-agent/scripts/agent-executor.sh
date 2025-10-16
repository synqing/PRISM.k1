#!/bin/bash
# Agent Executor - Binds AI agents to Taskmaster tasks
# Integrates with Knowledge Fortress for specifications

PROJECT_ROOT="/Users/spectrasynq/Workspace_Management/Software/PRISM.k1"
cd "$PROJECT_ROOT"

execute_with_claude_code() {
    local TASK_ID=$1
    local TAG=$2
    
    echo "🤖 Executing task $TASK_ID (tag: $TAG) with Claude Code..."
    
    # PRE-EXECUTION: Sync with Knowledge Fortress
    echo "📚 Syncing with Knowledge Fortress..."
    if ! multi-agent/scripts/sync-with-fortress.sh; then
        echo "❌ Failed to sync with fortress"
        return 1
    fi
    
    # Read CANON specifications
    if [ ! -f ".taskmaster/CANON.md" ]; then
        echo "❌ CANON.md not found"
        return 1
    fi
    
    CANON=$(cat .taskmaster/CANON.md)
    
    # Read agent rules
    AGENT_RULES=""
    if [ -f ".taskmaster/agent-rules.yml" ]; then
        AGENT_RULES=$(grep -A 30 "agent_behavior:" .taskmaster/agent-rules.yml 2>/dev/null || echo "")
    fi
    
    # Get task details from taskmaster
    TASK_INFO=$(task-master show --tag="$TAG" --id="$TASK_ID" 2>/dev/null || echo "Task ID: $TASK_ID")
    
    # Build context-aware prompt
    PROMPT=$(cat <<PROMPT
You are implementing task $TASK_ID for PRISM K1 firmware (tag: $TAG).

CRITICAL: You MUST follow specifications in CANON.md

## Current Specifications (from CANON.md)
$CANON

## Task Details
$TASK_INFO

## Implementation Rules (from agent-rules.yml)
$AGENT_RULES

## Instructions
1. Read the task description carefully
2. Check CANON.md for relevant specifications
3. Implement EXACTLY as specified in CANON (no deviations)
4. Add ADR references in code comments (e.g., /* ADR-002 */)
5. Follow existing code patterns in the firmware
6. Test your changes if possible
7. Validate against CANON before finishing

## ESP32-S3 Constraints
- 512KB SRAM, ~200KB available for heap
- 8MB Flash
- Real-time LED updates required (60 FPS)
- Memory fragmentation is a critical concern

Start implementation now. Be methodical and thorough.
PROMPT
)
    
    echo "📝 Generated implementation prompt"
    
    # Check if Claude Code is available
    if ! command -v claude-code &> /dev/null; then
        echo "⚠️  Claude Code not found. Using manual execution mode."
        echo ""
        echo "PROMPT FOR MANUAL EXECUTION:"
        echo "=============================="
        echo "$PROMPT"
        echo "=============================="
        echo ""
        echo "Please execute this task manually and then run:"
        echo "  .taskmaster/scripts/validate-canon.sh"
        return 0
    fi
    
    # Execute with Claude Code
    echo "🚀 Launching Claude Code..."
    claude-code --prompt "$PROMPT" \
                --working-dir "$(pwd)/firmware" \
                --auto-approve-safe \
                --log "multi-agent/logs/agent-${TAG}-${TASK_ID}.log" \
                --context ".taskmaster/CANON.md" \
                --context "firmware/**/*.{c,h}"
    
    local EXIT_CODE=$?
    
    # POST-EXECUTION: Validate against CANON
    echo "🔍 Validating implementation against CANON..."
    
    if [ -f ".taskmaster/scripts/validate-canon.sh" ]; then
        if .taskmaster/scripts/validate-canon.sh; then
            echo "✅ Implementation matches CANON"
            return 0
        else
            echo "❌ Implementation violates CANON"
            echo "   Review changes and fix violations before committing"
            return 1
        fi
    else
        echo "⚠️  CANON validation script not found, skipping validation"
        return $EXIT_CODE
    fi
}

# Alternative executor for Cursor or other IDEs
execute_with_cursor() {
    local TASK_ID=$1
    local TAG=$2
    
    echo "🤖 Executing task $TASK_ID with Cursor..."
    
    # Sync fortress
    multi-agent/scripts/sync-with-fortress.sh
    
    # Check if cursor CLI is available
    if ! command -v cursor &> /dev/null; then
        echo "❌ Cursor CLI not found"
        return 1
    fi
    
    # Launch Cursor with task context
    cursor --task "$TASK_ID" --tag "$TAG" --context ".taskmaster/CANON.md"
    
    return $?
}

# Main execution
if [ $# -lt 1 ]; then
    echo "Usage: agent-executor.sh <task-id> [tag] [executor]"
    echo ""
    echo "Arguments:"
    echo "  task-id    Task ID from taskmaster (e.g., 15, 15.2)"
    echo "  tag        Task tag/context (default: master)"
    echo "  executor   Execution method: claude|cursor|manual (default: claude)"
    echo ""
    echo "Examples:"
    echo "  ./agent-executor.sh 15 network claude"
    echo "  ./agent-executor.sh 12.3 storage"
    exit 1
fi

TASK_ID=$1
TAG=${2:-master}
EXECUTOR=${3:-claude}

case "$EXECUTOR" in
    claude)
        execute_with_claude_code "$TASK_ID" "$TAG"
        ;;
    cursor)
        execute_with_cursor "$TASK_ID" "$TAG"
        ;;
    manual)
        echo "Manual execution mode"
        echo "Task: $TASK_ID (tag: $TAG)"
        echo "Please implement this task manually"
        ;;
    *)
        echo "Unknown executor: $EXECUTOR"
        exit 1
        ;;
esac
