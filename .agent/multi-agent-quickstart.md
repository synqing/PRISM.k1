# Multi-Agent Taskmaster: Quick Start Guide

## 🚀 Deploy in 15 Minutes

This guide will have you running **5 concurrent agents** on your PRISM.k1 project in under 15 minutes.

---

## Prerequisites

- Taskmaster already initialized in project
- ESP-IDF development environment setup
- `tmux` or `screen` for terminal multiplexing (optional but recommended)
- Claude Code or Cursor with MCP configured

---

## Step 1: Deploy Scripts (2 minutes)

```bash
cd /Users/spectrasynq/Workspace_Management/Software/PRISM.k1

# Create scripts directory
mkdir -p .taskmaster/scripts

# Save the multi-agent script (from previous artifact)
cat > .taskmaster/scripts/multi-agent.sh << 'SCRIPT_EOF'
# [Paste the entire multi-agent.sh script here]
SCRIPT_EOF

chmod +x .taskmaster/scripts/multi-agent.sh

# Create convenience launcher
ln -s .taskmaster/scripts/multi-agent.sh ./taskmaster
```

---

## Step 2: Initialize Work Streams (3 minutes)

```bash
# Run automated setup
./taskmaster setup

# This creates 5 tagged contexts:
# - network (WebSocket, WiFi)
# - storage (LittleFS, patterns)
# - playback (LED driver, effects)
# - templates (Pattern templates)
# - integration (Testing, optimization)
```

**Manual Task Pruning (Optional but Recommended):**

```bash
# Network tag - keep only network-related tasks
./taskmaster use-tag network
task-master remove-task --id=4,7,8,19,20,21,... --yes

# Storage tag - keep only storage tasks
./taskmaster use-tag storage
task-master remove-task --id=9,10,11,15,16,... --yes

# Repeat for other tags...
```

---

## Step 3: Launch Monitoring (1 minute)

### Health Monitor (Background Process)

```bash
# Start continuous health monitoring
nohup bash -c '
  while true; do
    ./taskmaster health-check
    sleep 300  # Every 5 minutes
  done
' > .taskmaster/logs/health.log 2>&1 &

echo $! > .taskmaster/.health-monitor.pid
```

### Dashboard (Tmux Window)

```bash
# Create dedicated dashboard session
tmux new-session -d -s taskmaster-dash './taskmaster dashboard'

# Attach to view (Ctrl+B, D to detach)
tmux attach -t taskmaster-dash
```

---

## Step 4: Launch Agents (5 minutes)

### Option A: Tmux-Based Deployment (Recommended)

```bash
# Create agent session with 5 windows
tmux new-session -d -s agents

# Launch each agent in separate window
tmux send-keys -t agents:0 './taskmaster run-agent agent-1-network network 50' C-m
tmux split-window -t agents:0 -h './taskmaster run-agent agent-2-storage storage 50' C-m
tmux select-layout -t agents:0 tiled

tmux split-window -t agents:0 -h './taskmaster run-agent agent-3-playback playback 30' C-m
tmux split-window -t agents:0 -h './taskmaster run-agent agent-4-templates templates 40' C-m
tmux split-window -t agents:0 -h './taskmaster run-agent agent-5-integration integration 30' C-m

tmux select-layout -t agents:0 tiled

# View all agents
tmux attach -t agents
```

### Option B: Individual Terminals

Open 5 terminal windows and run:

**Terminal 1 (Network Agent):**
```bash
./taskmaster run-agent agent-1-network network 50
```

**Terminal 2 (Storage Agent):**
```bash
./taskmaster run-agent agent-2-storage storage 50
```

**Terminal 3 (Playback Agent):**
```bash
./taskmaster run-agent agent-3-playback playback 30
```

**Terminal 4 (Templates Agent):**
```bash
./taskmaster run-agent agent-4-templates templates 40
```

**Terminal 5 (Integration Agent):**
```bash
./taskmaster run-agent agent-5-integration integration 30
```

---

## Step 5: Monitor Progress (Ongoing)

### Real-Time Dashboard

```bash
# Attach to dashboard
tmux attach -t taskmaster-dash

# Detach with Ctrl+B, D
```

**Dashboard shows:**
- Active agents and current tasks
- Progress bars by work stream
- System health metrics
- Recent completions

### Manual Checks

```bash
# Check overall progress
task-master list --tag=master --status=done | wc -l

# Check agent activity
ls -lh .taskmaster/.locks/task-*.claim

# View recent logs
tail -f .taskmaster/logs/health.log

# Collect metrics
./taskmaster metrics
```

---

## Integration with Claude Code

### Modify execute_task() Function

Edit `.taskmaster/scripts/multi-agent.sh` and update the `execute_task()` function:

```bash
execute_task() {
    local TAG=$1
    local TASK_ID=$2
    local AGENT_ID=$3
    
    echo "🔨 Executing task $TASK_ID..."
    
    # Get task details
    TASK_DETAILS=$(task-master show --tag="$TAG" --id="$TASK_ID")
    
    # Research phase
    task-master research \
        --task-ids="$TASK_ID" \
        --tag="$TAG" \
        --query="Implementation guide and best practices for this task" \
        --save-to="$TASK_ID"
    
    # Implementation via Claude Code
    cd "$PROJECT_ROOT"
    
    claude-code <<EOF
I'm agent ${AGENT_ID} working on task ${TASK_ID} from tag ${TAG}.

Task Details:
${TASK_DETAILS}

Please implement this task following these steps:
1. Review task requirements and dependencies
2. Check if related files exist in firmware/components/
3. Implement the required functionality
4. Verify build compiles: cd firmware && idf.py build
5. Update task with implementation notes

When done, confirm implementation is complete.
EOF
    
    # Verify implementation
    if verify_task "$TAG" "$TASK_ID"; then
        task-master update-subtask \
            --tag="$TAG" \
            --id="$TASK_ID" \
            --prompt="Implementation completed by $AGENT_ID. Verified successfully."
        return 0
    else
        return 1
    fi
}
```

---

## Troubleshooting

### Agents Not Starting

```bash
# Check task availability
task-master list --tag=network

# Check for lock conflicts
ls -lh .taskmaster/.locks/

# Reset stale claims
find .taskmaster/.locks -name "task-*.claim" -delete
./taskmaster health-check
```

### Build Failures

```bash
# Check build logs
cat .taskmaster/logs/build-*.log

# Manual build test
cd firmware
idf.py fullclean
idf.py build
```

### Agents Stuck

```bash
# Check for stale claims
find .taskmaster/.locks -name "task-*.claim" -mmin +60

# Force health check
./taskmaster health-check

# Kill stuck agent
ps aux | grep "run-agent"
kill <PID>
```

### No Tasks Appearing

```bash
# Check task distribution
for TAG in network storage playback templates integration; do
    echo "=== $TAG ==="
    task-master list --tag=$TAG | head -10
done

# Re-parse PRD if needed
task-master parse-prd .taskmaster/docs/prism-firmware-prd.txt --append
```

---

## Optimization Tips

### 1. Task Granularity

If agents are completing tasks too quickly:
```bash
# Expand complex tasks into subtasks
task-master expand --all --tag=network --research
```

If tasks are too granular:
```bash
# Merge related subtasks
task-master move --from=5.1,5.2,5.3 --to=5
```

### 2. Agent Load Balancing

Monitor agent throughput:
```bash
# Count completions per agent
find .taskmaster/.locks -name "*.claim.completed-*" | \
  cut -d: -f1 | sort | uniq -c
```

Adjust agent assignment if imbalanced:
```bash
# Move tasks from overloaded tag to underutilized one
task-master use-tag network
task-master move --from=15,16,17 --to=storage
```

### 3. Priority Management

Boost critical path tasks:
```bash
# Find critical tasks
task-master complexity-report --tag=master

# Prioritize in agent execution
# (Modify allocate_task() to consider priority)
```

---

## Shutdown Procedure

### Graceful Shutdown

```bash
# Stop health monitor
kill $(cat .taskmaster/.health-monitor.pid)

# Kill agent tmux sessions
tmux kill-session -t agents
tmux kill-session -t taskmaster-dash

# Archive active claims
find .taskmaster/.locks -name "task-*.claim" -exec sh -c '
    mv "$1" "$1.shutdown-$(date +%s)"
' _ {} \;

# Generate final metrics
./taskmaster metrics
```

### Emergency Shutdown

```bash
# Kill all agents
pkill -f "run-agent"

# Clear all locks
rm -rf .taskmaster/.locks/*

# Reset in-progress tasks to pending
for TAG in network storage playback templates integration; do
    task-master list --tag=$TAG --status=in-progress | \
      grep -oP 'Task \K[\d.]+' | while read TASK; do
        task-master set-status --tag=$TAG --id=$TASK --status=pending
      done
done
```

---

## Expected Timeline

| Phase | Duration | Outcome |
|-------|----------|---------|
| Setup | 15 min | 5 agents operational |
| Ramp Up | 30 min | Agents claim first tasks |
| Steady State | 2-4 days | Parallel execution |
| Integration | 1 day | Merge work streams |
| **Total** | **3-5 days** | 53 tasks complete |

**vs. Single Agent:** 10-15 days → **3x faster!**

---

## Success Metrics

### Velocity
- **Target:** 12-20 tasks/day (vs. 3-5 single-agent)
- **Monitor:** `./taskmaster metrics`

### Quality
- **Build Success Rate:** >95%
- **Monitor:** Check `.taskmaster/logs/build-*.log`

### Efficiency
- **Agent Utilization:** >80%
- **Monitor:** Dashboard active agents vs. idle time

### Reliability
- **Stale Tasks:** <2 per day
- **Monitor:** `./taskmaster health-check`

---

## Next Steps

Once running smoothly:

1. **Tune Agent Capabilities** - Adjust task matching based on performance
2. **Add Custom Verifiers** - Implement component-specific tests
3. **Enable Auto-Recovery** - Let agents retry failed tasks with research
4. **Scale Up** - Add more agents as needed (6-10 total)
5. **Cross-Project** - Reuse patterns for other projects

---

## Quick Reference Commands

```bash
# Start everything
./taskmaster setup && \
  nohup ./taskmaster health-check-loop & \
  tmux new-session -d -s agents './taskmaster launch-agents' && \
  tmux new-session -d -s dash './taskmaster dashboard'

# Monitor
tmux attach -t dash

# Check status
task-master list --tag=master --status=done | wc -l

# Shutdown
./taskmaster shutdown-all

# Emergency stop
pkill -f taskmaster && rm -rf .taskmaster/.locks/*
```

---

## Support

If issues arise:

1. Check `.taskmaster/logs/health.log`
2. Run `./taskmaster health-check`
3. Review agent logs in tmux windows
4. Consult taskmaster documentation: `task-master help`

**Happy parallel development!** 🚀
