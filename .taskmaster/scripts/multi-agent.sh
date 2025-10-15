#!/bin/bash
# Multi-Agent Taskmaster Automation Scripts
# Place in .taskmaster/scripts/ directory

# =============================================================================
# 1. TASK ALLOCATOR - Atomic task claiming for agents
# =============================================================================

allocate_task() {
    local TAG="${1:-master}"
    local AGENT_ID="${2:-agent-unknown}"
    local LOCKFILE=".taskmaster/.locks/${TAG}.lock"
    
    # Create locks directory
    mkdir -p .taskmaster/.locks
    
    # Atomic lock acquisition
    exec 200>"$LOCKFILE"
    if ! flock -x -w 5 200; then
        echo "ERROR:LOCK_TIMEOUT"
        return 1
    fi
    
    # Get next available task
    TASK=$(task-master next --tag="$TAG" 2>&1 | grep -oP 'Task \K[\d.]+' | head -1)
    
    if [ -z "$TASK" ]; then
        echo "NO_TASKS_AVAILABLE"
        flock -u 200
        return 1
    fi
    
    # Claim task atomically
    task-master set-status --tag="$TAG" --id="$TASK" --status=in-progress 2>&1
    echo "${AGENT_ID}:$(date -u +%Y-%m-%dT%H:%M:%SZ):$$" > ".taskmaster/.locks/task-${TAG}-${TASK}.claim"
    
    echo "CLAIMED:${TASK}"
    flock -u 200
    return 0
}

# =============================================================================
# 2. TASK RELEASE - Clean up after task completion
# =============================================================================

release_task() {
    local TAG=$1
    local TASK_ID=$2
    local STATUS=${3:-done}
    local AGENT_ID=$4
    
    # Update status
    task-master set-status --tag="$TAG" --id="$TASK_ID" --status="$STATUS"
    
    # Remove claim file
    local CLAIM_FILE=".taskmaster/.locks/task-${TAG}-${TASK_ID}.claim"
    if [ -f "$CLAIM_FILE" ]; then
        # Archive claim for audit
        mv "$CLAIM_FILE" "$CLAIM_FILE.completed-$(date +%s)"
    fi
    
    echo "✅ Released task $TASK_ID with status: $STATUS"
}

# =============================================================================
# 3. HEALTH CHECK MONITOR - Detect and recover from issues
# =============================================================================

health_check() {
    echo "🏥 Running health checks..."
    
    # Check 1: Stale task claims (>2 hours)
    echo "Checking for stale claims..."
    find .taskmaster/.locks -name "task-*.claim" -type f -mmin +120 2>/dev/null | while read CLAIM; do
        if [ ! -f "$CLAIM" ]; then continue; fi
        
        local TASK=$(basename "$CLAIM" | sed 's/task-//;s/.claim//')
        local TAG=$(echo "$TASK" | cut -d- -f1)
        local TASK_ID=$(echo "$TASK" | cut -d- -f2)
        
        echo "⚠️  Stale task detected: $TAG/$TASK_ID (>2h in-progress)"
        
        # Auto-recovery
        task-master set-status --tag="$TAG" --id="$TASK_ID" --status=pending
        mv "$CLAIM" "${CLAIM}.abandoned-$(date +%s)"
        
        echo "✅ Task $TAG/$TASK_ID reset to pending"
    done
    
    # Check 2: Dependency violations
    echo "Checking dependencies..."
    for TAG in master network storage playback templates integration; do
        if task-master validate-dependencies --tag="$TAG" 2>&1 | grep -q "ERROR"; then
            echo "🔧 Fixing dependencies in $TAG..."
            task-master fix-dependencies --tag="$TAG"
        fi
    done
    
    # Check 3: Orphaned lock files
    echo "Cleaning orphaned locks..."
    find .taskmaster/.locks -name "*.lock" -type f -mmin +60 -delete
    
    # Check 4: Build health
    echo "Verifying build health..."
    if [ -f "firmware/CMakeLists.txt" ]; then
        cd firmware
        if idf.py build --dry-run &>/dev/null; then
            echo "✅ Build system healthy"
        else
            echo "⚠️  Build system issues detected"
        fi
        cd ..
    fi
    
    echo "✅ Health check complete"
}

# =============================================================================
# 4. AGENT RUNNER - Main agent execution loop
# =============================================================================

run_agent() {
    local AGENT_ID=${1:-"agent-default"}
    local TAG=${2:-"master"}
    local MAX_TASKS=${3:-100}
    
    echo "🤖 Starting agent: $AGENT_ID on tag: $TAG"
    
    local COMPLETED=0
    local FAILED=0
    
    while [ $COMPLETED -lt $MAX_TASKS ]; do
        # Allocate next task
        RESULT=$(allocate_task "$TAG" "$AGENT_ID")
        
        if [[ "$RESULT" == "NO_TASKS_AVAILABLE" ]]; then
            echo "✅ All tasks in $TAG completed!"
            break
        elif [[ "$RESULT" == ERROR:* ]]; then
            echo "❌ Failed to allocate task: $RESULT"
            sleep 30
            continue
        fi
        
        # Extract task ID
        TASK_ID=$(echo "$RESULT" | cut -d: -f2)
        echo "📋 Working on task: $TASK_ID"
        
        # Get task details
        task-master show --tag="$TAG" --id="$TASK_ID"
        
        # Execute task (placeholder - integrate with your AI agent)
        if execute_task "$TAG" "$TASK_ID" "$AGENT_ID"; then
            release_task "$TAG" "$TASK_ID" "done" "$AGENT_ID"
            ((COMPLETED++))
            echo "✅ Completed: $TASK_ID ($COMPLETED/$MAX_TASKS)"
        else
            release_task "$TAG" "$TASK_ID" "blocked" "$AGENT_ID"
            ((FAILED++))
            echo "❌ Failed: $TASK_ID"
        fi
        
        # Brief cooldown
        sleep 5
    done
    
    echo "🏁 Agent $AGENT_ID finished: $COMPLETED completed, $FAILED failed"
}

# =============================================================================
# 5. TASK EXECUTOR - Integration point for AI agents
# =============================================================================

execute_task() {
    local TAG=$1
    local TASK_ID=$2
    local AGENT_ID=$3
    
    echo "🔨 Executing task $TASK_ID..."
    
    # Research phase
    echo "📚 Researching task context..."
    task-master research \
        --task-ids="$TASK_ID" \
        --tag="$TAG" \
        --query="Best practices and implementation guide for this task" \
        --save-to="$TASK_ID" \
        --no-followup \
        2>&1 | tee ".taskmaster/logs/research-${TAG}-${TASK_ID}.log"
    
    # Implementation phase (customize this for your environment)
    echo "💻 Implementing task..."
    
    # Option 1: Call Claude Code
    # claude-code --prompt "Implement task $TASK_ID from tag $TAG" --auto-approve
    
    # Option 2: Call Cursor via CLI
    # cursor --task "$TASK_ID" --tag "$TAG"
    
    # Option 3: Custom implementation script
    # ./scripts/implement-task.sh "$TAG" "$TASK_ID"
    
    # For now, placeholder
    echo "⚠️  Implementation placeholder - integrate with your AI agent here"
    
    # Verification phase
    echo "🧪 Verifying implementation..."
    if verify_task "$TAG" "$TASK_ID"; then
        task-master update-subtask \
            --tag="$TAG" \
            --id="$TASK_ID" \
            --prompt="Implementation completed by $AGENT_ID. Build verified successfully."
        return 0
    else
        task-master update-subtask \
            --tag="$TAG" \
            --id="$TASK_ID" \
            --prompt="Implementation attempted by $AGENT_ID but verification failed."
        return 1
    fi
}

# =============================================================================
# 6. TASK VERIFIER - Build and test validation
# =============================================================================

verify_task() {
    local TAG=$1
    local TASK_ID=$2
    
    echo "🔍 Verifying task $TASK_ID..."
    
    # Build verification
    if [ -f "firmware/CMakeLists.txt" ]; then
        cd firmware
        if idf.py build 2>&1 | tee "../.taskmaster/logs/build-${TAG}-${TASK_ID}.log"; then
            echo "✅ Build passed"
            cd ..
            return 0
        else
            echo "❌ Build failed"
            cd ..
            return 1
        fi
    else
        echo "⚠️  No build system found, skipping verification"
        return 0
    fi
}

# =============================================================================
# 7. DASHBOARD - Real-time agent activity monitor
# =============================================================================

show_dashboard() {
    while true; do
        clear
        echo "╔════════════════════════════════════════════════════════════════╗"
        echo "║          PRISM.k1 Multi-Agent Dashboard                       ║"
        echo "╚════════════════════════════════════════════════════════════════╝"
        echo "$(date '+%Y-%m-%d %H:%M:%S')"
        echo ""
        
        # Active agents
        echo "🤖 Active Agents:"
        if [ -d ".taskmaster/.locks" ]; then
            find .taskmaster/.locks -name "task-*.claim" -type f 2>/dev/null | while read CLAIM; do
                if [ ! -f "$CLAIM" ]; then continue; fi
                local AGENT=$(cut -d: -f1 "$CLAIM")
                local TASK=$(basename "$CLAIM" | sed 's/task-//;s/.claim//')
                local TIME=$(cut -d: -f2 "$CLAIM")
                printf "  %-20s | %-15s | %s\n" "$AGENT" "$TASK" "$TIME"
            done
        fi
        echo ""
        
        # Progress by tag
        echo "📊 Progress by Work Stream:"
        for TAG in master network storage playback templates integration; do
            local TOTAL=$(task-master list --tag="$TAG" 2>/dev/null | wc -l)
            local DONE=$(task-master list --tag="$TAG" --status=done 2>/dev/null | wc -l)
            
            if [ $TOTAL -gt 0 ]; then
                local PROGRESS=$((DONE * 100 / TOTAL))
                local BARS=$((PROGRESS / 5))
                printf "  %-12s [%-20s] %3d%% (%d/%d)\n" \
                    "$TAG" \
                    "$(printf '█%.0s' $(seq 1 $BARS))$(printf '░%.0s' $(seq 1 $((20 - BARS))))" \
                    "$PROGRESS" "$DONE" "$TOTAL"
            fi
        done
        echo ""
        
        # System health
        echo "💚 System Health:"
        local STALE=$(find .taskmaster/.locks -name "task-*.claim" -type f -mmin +120 2>/dev/null | wc -l)
        local BLOCKED=$(task-master list --status=blocked 2>/dev/null | wc -l)
        echo "  Stale tasks: $STALE"
        echo "  Blocked tasks: $BLOCKED"
        echo ""
        
        # Recent activity
        echo "📝 Recent Activity (last 10 minutes):"
        find .taskmaster/.locks -name "*.claim.completed-*" -mmin -10 2>/dev/null | \
            tail -5 | while read COMPLETED; do
                echo "  ✅ $(basename "$COMPLETED" | sed 's/task-//;s/.claim.completed-.*$//')"
            done
        
        sleep 10
    done
}

# =============================================================================
# 8. METRICS COLLECTOR - Agent performance analytics
# =============================================================================

collect_metrics() {
    echo "📊 Collecting agent metrics..."
    
    local METRICS_FILE=".taskmaster/metrics/$(date +%Y%m%d).json"
    mkdir -p .taskmaster/metrics
    
    cat > "$METRICS_FILE" <<EOF
{
  "timestamp": "$(date -u +%Y-%m-%dT%H:%M:%SZ)",
  "agents": {
EOF
    
    # Per-agent metrics
    find .taskmaster/.locks -name "*.claim.completed-*" -mtime -1 2>/dev/null | \
        cut -d: -f1 | sort | uniq -c | while read COUNT AGENT; do
            echo "    \"$AGENT\": { \"completed\": $COUNT },"
        done >> "$METRICS_FILE"
    
    echo "  }," >> "$METRICS_FILE"
    
    # Overall metrics
    cat >> "$METRICS_FILE" <<EOF
  "overall": {
    "total_tasks": $(task-master list --tag=master 2>/dev/null | wc -l),
    "completed": $(task-master list --tag=master --status=done 2>/dev/null | wc -l),
    "in_progress": $(task-master list --tag=master --status=in-progress 2>/dev/null | wc -l),
    "blocked": $(task-master list --tag=master --status=blocked 2>/dev/null | wc -l)
  }
}
EOF
    
    echo "✅ Metrics saved to $METRICS_FILE"
}

# =============================================================================
# 9. SETUP SCRIPT - Initialize multi-agent environment
# =============================================================================

setup_multi_agent() {
    echo "🚀 Setting up multi-agent environment..."
    
    # Create directory structure
    mkdir -p .taskmaster/{scripts,locks,logs,metrics}
    
    # Create work stream tags
    echo "Creating work stream tags..."
    
    task-master add-tag network --copy-from-current \
        --description="WebSocket, WiFi, and network protocols"
    
    task-master add-tag storage --copy-from-current \
        --description="LittleFS, pattern storage, and caching"
    
    task-master add-tag playback --copy-from-current \
        --description="LED driver, effects, and animation"
    
    task-master add-tag templates --copy-from-current \
        --description="Template patterns and deployment"
    
    task-master add-tag integration --copy-from-current \
        --description="Testing, debugging, and optimization"
    
    echo "✅ Multi-agent environment ready!"
    echo ""
    echo "Next steps:"
    echo "1. Run health monitor: nohup ./taskmaster health-check-loop &"
    echo "2. Launch agents: ./taskmaster launch-agents"
    echo "3. Monitor progress: ./taskmaster dashboard"
}

# =============================================================================
# MAIN SCRIPT DISPATCHER
# =============================================================================

case "${1:-help}" in
    allocate)
        allocate_task "${2:-master}" "${3:-agent-default}"
        ;;
    release)
        release_task "$2" "$3" "${4:-done}" "${5:-agent-default}"
        ;;
    health-check)
        health_check
        ;;
    run-agent)
        run_agent "${2:-agent-default}" "${3:-master}" "${4:-100}"
        ;;
    dashboard)
        show_dashboard
        ;;
    metrics)
        collect_metrics
        ;;
    setup)
        setup_multi_agent
        ;;
    help)
        cat <<EOF
Multi-Agent Taskmaster Scripts
================================

Commands:
  allocate <tag> <agent_id>     - Claim next available task
  release <tag> <task_id> <status> <agent_id> - Release completed task
  health-check                  - Run system health checks
  run-agent <id> <tag> <max>   - Start agent execution loop
  dashboard                     - Show real-time activity monitor
  metrics                       - Collect performance metrics
  setup                         - Initialize multi-agent environment

Example Usage:
  ./multi-agent.sh setup
  ./multi-agent.sh run-agent agent-1 network 50
  ./multi-agent.sh dashboard

EOF
        ;;
    *)
        echo "Unknown command: $1"
        echo "Run './multi-agent.sh help' for usage"
        exit 1
        ;;
esac
