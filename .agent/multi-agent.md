# PRISM.k1 Multi-Agent Coordination

**Purpose:** Enable 3-5 agents to work concurrently without conflicts, with automatic problem resolution.

---

## 🎯 Multi-Agent Architecture

### Agent Roles & Specializations

| Agent ID | Tag Context | Domain | Responsibilities |
|----------|-------------|--------|------------------|
| **agent-1-network** | `network` | WebSocket, WiFi | Tasks 3-11, 15-17, 26-28 |
| **agent-2-storage** | `storage` | LittleFS, Patterns | Tasks 4, 7-8, 12-14, 29-30 |
| **agent-3-playback** | `playback` | LED Driver, Effects | Tasks 18-22, 34 |
| **agent-4-templates** | `templates` | Pattern Design | Tasks 46-53 |
| **agent-5-integration** | `integration` | Testing, Optimization | Tasks 31-35, cross-component |

### Work Stream Isolation

```
PRISM.k1/
├── Tag: network (Agent 1)
│   ├── WebSocket server implementation
│   ├── WiFi connection management
│   ├── Binary protocol parser
│   └── mDNS service discovery
│
├── Tag: storage (Agent 2)
│   ├── LittleFS filesystem operations
│   ├── Pattern file format handling
│   ├── Hot cache implementation
│   └── Index management
│
├── Tag: playback (Agent 3)
│   ├── RMT peripheral LED driver
│   ├── Animation engine & interpolation
│   ├── Effect rendering pipeline
│   └── Frame buffer management
│
├── Tag: templates (Agent 4)
│   ├── Pre-built pattern design
│   ├── Template metadata system
│   ├── Palette optimization
│   └── Effect algorithm development
│
└── Tag: integration (Agent 5)
    ├── Cross-component testing
    ├── Performance profiling
    ├── Memory leak detection
    └── Build optimization
```

**Key Principle:** Each agent owns a complete vertical slice. No shared files during active development.

---

## 🔒 Task Claiming Protocol

### Atomic Task Allocation

**DO NOT manually select tasks.** Use the atomic allocation system:

```bash
# Agent startup script
export AGENT_ID="agent-1-network"
export AGENT_TAG="network"

# Main work loop
while true; do
  # Atomic claim via lock file
  TASK_ID=$(task-master next --tag=$AGENT_TAG | grep -oP 'Task \K[\d.]+' | head -1)
  
  if [ -z "$TASK_ID" ]; then
    echo "No tasks available in $AGENT_TAG"
    break
  fi
  
  # Mark in-progress (atomic operation)
  task-master set-status --tag=$AGENT_TAG --id=$TASK_ID --status=in-progress
  
  # Create claim file for tracking
  echo "$AGENT_ID:$(date -u +%Y-%m-%dT%H:%M:%SZ):$$" > \
    ".taskmaster/.locks/task-${AGENT_TAG}-${TASK_ID}.claim"
  
  # Work on task following workflow.md
  echo "🤖 $AGENT_ID working on task $TASK_ID"
  
  # ... implementation ...
  
  # Release claim
  task-master set-status --tag=$AGENT_TAG --id=$TASK_ID --status=done
  rm ".taskmaster/.locks/task-${AGENT_TAG}-${TASK_ID}.claim"
done
```

### Lock File Structure

```bash
.taskmaster/.locks/
├── task-network-10.claim        # Agent 1 working on task 10
├── task-storage-12.claim        # Agent 2 working on task 12
├── task-playback-19.claim       # Agent 3 working on task 19
└── network.lock                 # Tag-level lock for atomic operations
```

**Claim File Format:**
```
agent-1-network:2025-10-15T14:32:15Z:12345
^              ^                   ^
Agent ID       Timestamp          Process ID
```

---

## 🔄 Cross-Agent Communication

### Interface Contracts

**When one agent needs information from another domain:**

#### Pattern 1: API Contract Request

**Agent 1 (network) needs API from Agent 2 (storage):**

```bash
# Agent 1: Block and request
task-master update-subtask --tag=network --id=11.3 \
  --prompt="🔴 BLOCKED: Need Storage API Contract

@agent-2-storage (task 4.2)

Required API:
- Function: Pattern write operation
- Signature: esp_err_t prism_storage_write(???)
- Parameters: Need to know: path format, data structure, flags
- Return: Error codes to handle
- Thread safety: Is it safe to call from network task?

Use Case: WebSocket handler will call this after receiving pattern upload.
Timeline: Blocking task 11.3 (WebSocket upload handler)
"

task-master set-status --tag=network --id=11.3 --status=blocked
```

**Agent 2 (storage) responds:**

```bash
# Agent 2: Provide API contract
task-master update-subtask --tag=storage --id=4.2 \
  --prompt="📤 API Contract Response for @agent-1-network

Function Signature:
\`\`\`c
esp_err_t prism_storage_write(
    const char *path,        // Format: \"/prism/patterns/<uuid>.prism\"
    const void *data,        // Binary pattern data
    size_t length,           // Data size in bytes
    bool atomic              // true = use .tmp + rename
);
\`\`\`

Return Values:
- ESP_OK: Success
- ESP_ERR_INVALID_ARG: NULL pointer or invalid path
- ESP_ERR_NO_MEM: Insufficient storage space
- ESP_FAIL: Write operation failed

Thread Safety: YES - Uses filesystem mutex internally
Max Size: 204800 bytes (200KB)
Performance: ~500KB/s write speed

Header File: \`firmware/components/storage/include/prism_storage.h\`
Implementation: Task 4.2 (status: in-progress)
ETA: 2 hours
"
```

**Agent 1: Unblock and continue:**

```bash
# Agent 1: Acknowledge and unblock
task-master update-subtask --tag=network --id=11.3 \
  --prompt="✅ UNBLOCKED: Storage API contract received from @agent-2-storage

Will implement WebSocket handler using:
- prism_storage_write() with atomic=true
- Error handling for ESP_ERR_NO_MEM
- UUID generation for path format

Proceeding with implementation."

task-master set-status --tag=network --id=11.3 --status=in-progress
```

#### Pattern 2: Data Format Specification

**Agent 3 (playback) needs pattern format from Agent 2 (storage):**

```bash
# Agent 3: Request spec
task-master update-subtask --tag=playback --id=19.2 \
  --prompt="🔴 BLOCKED: Need Pattern File Format Specification

@agent-2-storage (task 7)

Required Info:
- File format: Header structure? TLV? Binary layout?
- How to read timeline data?
- How to read palette data?
- Validation: CRC? Magic bytes?
- Endianness: Little or big?

Use Case: Timeline player needs to parse .prism files for playback.
"
```

#### Pattern 3: Shared Constants

**When constants need to be shared across domains:**

**Create shared header:**
```c
// firmware/components/core/include/prism_constants.h
#ifndef PRISM_CONSTANTS_H
#define PRISM_CONSTANTS_H

// Shared configuration
#define PRISM_MAX_PATTERN_SIZE      (200 * 1024)  // 200KB
#define PRISM_PATTERN_MAGIC         0x5052534D    // "PRSM"
#define PRISM_LED_COUNT             320
#define PRISM_FRAME_RATE            60            // Hz

// Storage paths
#define PRISM_STORAGE_BASE          "/prism"
#define PRISM_PATTERNS_DIR          "/prism/patterns"
#define PRISM_CACHE_DIR             "/prism/cache"

// Network configuration
#define PRISM_WEBSOCKET_PORT        8080
#define PRISM_WEBSOCKET_BUFFER      8192

#endif // PRISM_CONSTANTS_H
```

**Document in task:**
```bash
task-master update-subtask --tag=integration --id=35.1 \
  --prompt="📋 Shared Constants Defined

Created: firmware/components/core/include/prism_constants.h

All agents: Include this header for shared configuration.
No agent should define these constants locally.

Constants defined:
- PRISM_MAX_PATTERN_SIZE = 200KB
- PRISM_LED_COUNT = 320
- PRISM_FRAME_RATE = 60 Hz
- ... (see header for full list)

@agent-1-network @agent-2-storage @agent-3-playback @agent-4-templates
"
```

---

## 🕐 Stale Task Detection & Recovery

### Automatic Health Monitoring

**Health monitor runs every 5 minutes:**

```bash
#!/bin/bash
# .taskmaster/scripts/health-monitor.sh

while true; do
  echo "🏥 Running health check..."
  
  # Find stale claims (>2 hours old)
  find .taskmaster/.locks -name "task-*.claim" -mmin +120 | while read CLAIM; do
    TASK=$(basename "$CLAIM" | sed 's/task-//;s/.claim//')
    TAG=$(echo "$TASK" | cut -d- -f1)
    TASK_ID=$(echo "$TASK" | cut -d- -f2)
    
    echo "⚠️  Stale task detected: $TAG/$TASK_ID"
    
    # Auto-recovery: reset to pending
    task-master set-status --tag="$TAG" --id="$TASK_ID" --status=pending
    mv "$CLAIM" "$CLAIM.abandoned-$(date +%s)"
    
    echo "✅ Task $TAG/$TASK_ID reset to pending"
  done
  
  # Check for dependency violations
  task-master validate-dependencies 2>&1 | grep "ERROR" && \
    task-master fix-dependencies
  
  sleep 300  # 5 minutes
done
```

**Run as background service:**
```bash
nohup ./.taskmaster/scripts/health-monitor.sh > .taskmaster/logs/health.log 2>&1 &
echo $! > .taskmaster/.health-monitor.pid
```

### Manual Stale Task Recovery

**If an agent crashes or hangs:**

```bash
# List all active claims
ls -lh .taskmaster/.locks/task-*.claim

# Check claim age
find .taskmaster/.locks -name "task-*.claim" -mmin +60  # Over 1 hour

# Manually reset stale task
TAG="network"
TASK_ID="10"
task-master set-status --tag=$TAG --id=$TASK_ID --status=pending
rm .taskmaster/.locks/task-${TAG}-${TASK_ID}.claim

# Task is now available for re-claiming
```

---

## 📊 Real-Time Agent Dashboard

### Live Activity Monitor

```bash
#!/bin/bash
# .taskmaster/scripts/dashboard.sh

while true; do
  clear
  echo "╔════════════════════════════════════════════════════════════════╗"
  echo "║          PRISM.k1 Multi-Agent Dashboard                       ║"
  echo "╚════════════════════════════════════════════════════════════════╝"
  echo "$(date '+%Y-%m-%d %H:%M:%S')"
  echo ""
  
  # Active agents
  echo "🤖 Active Agents:"
  find .taskmaster/.locks -name "task-*.claim" 2>/dev/null | while read CLAIM; do
    if [ ! -f "$CLAIM" ]; then continue; fi
    AGENT=$(cut -d: -f1 "$CLAIM")
    TASK=$(basename "$CLAIM" | sed 's/task-//;s/.claim//')
    TIME=$(cut -d: -f2 "$CLAIM")
    printf "  %-20s | %-15s | %s\n" "$AGENT" "$TASK" "$TIME"
  done
  [ ! "$(ls -A .taskmaster/.locks/task-*.claim 2>/dev/null)" ] && echo "  (no active agents)"
  echo ""
  
  # Progress by tag
  echo "📊 Progress by Work Stream:"
  for TAG in network storage playback templates integration; do
    TOTAL=$(task-master list --tag="$TAG" 2>/dev/null | wc -l)
    DONE=$(task-master list --tag="$TAG" --status=done 2>/dev/null | wc -l)
    
    if [ $TOTAL -gt 0 ]; then
      PROGRESS=$((DONE * 100 / TOTAL))
      BARS=$((PROGRESS / 5))
      printf "  %-12s [%-20s] %3d%% (%d/%d)\n" \
        "$TAG" \
        "$(printf '█%.0s' $(seq 1 $BARS))$(printf '░%.0s' $(seq 1 $((20 - BARS))))" \
        "$PROGRESS" "$DONE" "$TOTAL"
    fi
  done
  echo ""
  
  # System health
  echo "💚 System Health:"
  STALE=$(find .taskmaster/.locks -name "task-*.claim" -mmin +120 2>/dev/null | wc -l)
  BLOCKED=$(task-master list --status=blocked 2>/dev/null | wc -l)
  echo "  Stale tasks: $STALE"
  echo "  Blocked tasks: $BLOCKED"
  echo ""
  
  # Recent completions
  echo "📝 Recent Completions (last 10 minutes):"
  find .taskmaster/.locks -name "*.claim.completed-*" -mmin -10 2>/dev/null | \
    tail -5 | while read COMPLETED; do
      echo "  ✅ $(basename "$COMPLETED" | sed 's/task-//;s/.claim.completed-.*$//')"
    done
  
  sleep 10
done
```

**Launch dashboard:**
```bash
# In tmux/screen session
tmux new-session -d -s taskmaster-dash '.taskmaster/scripts/dashboard.sh'
tmux attach -t taskmaster-dash
```

---

## 🎨 Agent Specialization Guidelines

### Network Agent (agent-1-network)

**Skills:**
- WebSocket protocol expertise
- WiFi connection management
- Binary protocol design
- Network security

**Focus Areas:**
- Low-latency communication
- Reliable upload handling
- Connection recovery
- Protocol efficiency

**Typical Task Flow:**
1. Research WebSocket patterns
2. Implement binary protocol parser
3. Add error recovery mechanisms
4. Test with large uploads
5. Verify latency <100ms

### Storage Agent (agent-2-storage)

**Skills:**
- Filesystem operations
- Data serialization
- Caching strategies
- Wear leveling awareness

**Focus Areas:**
- Atomic file operations
- Pattern format efficiency
- Hot cache performance
- Flash longevity

**Typical Task Flow:**
1. Research LittleFS best practices
2. Design atomic commit pattern
3. Implement cache eviction
4. Test under power-loss scenarios
5. Verify no flash corruption

### Playback Agent (agent-3-playback)

**Skills:**
- Real-time systems
- Hardware peripherals (RMT)
- Animation mathematics
- Performance optimization

**Focus Areas:**
- 60 FPS consistency
- LED timing accuracy
- Effect algorithms
- CPU efficiency

**Typical Task Flow:**
1. Research RMT peripheral usage
2. Implement double-buffering
3. Optimize frame generation
4. Test with oscilloscope
5. Verify zero flicker

### Templates Agent (agent-4-templates)

**Skills:**
- Creative pattern design
- Effect algorithm development
- Color theory
- User experience

**Focus Areas:**
- Visual quality
- Template variety
- Instant deployment
- Metadata accuracy

**Typical Task Flow:**
1. Research effect algorithms
2. Design pattern sequence
3. Optimize palette usage
4. Test visual appearance
5. Verify deploy time <2s

### Integration Agent (agent-5-integration)

**Skills:**
- System-level testing
- Performance profiling
- Memory analysis
- Cross-component debugging

**Focus Areas:**
- Build stability
- Integration testing
- Performance targets
- Quality metrics

**Typical Task Flow:**
1. Review component interfaces
2. Design integration tests
3. Run stress tests
4. Profile memory/CPU
5. Verify all targets met

---

## 🛡️ Conflict Prevention

### Shared File Rules

**Avoid editing the same file simultaneously:**

| File Type | Ownership | Rule |
|-----------|-----------|------|
| Component implementation | Owning agent | Exclusive write access |
| Shared headers | Integration agent | Coordinate via task updates |
| Build configs | Owning agent | Tag-specific CMakeLists.txt |
| Documentation | Any agent | Append-only, timestamp changes |

### Merge Conflict Resolution

**If Git merge conflicts occur:**

```bash
# 1. Identify conflicting files
git status

# 2. Review both changes
git diff --ours --theirs <file>

# 3. Coordinate via task updates
task-master update-subtask --tag=integration --id=<task> \
  --prompt="⚠️ Merge Conflict in <file>

Agent 1 changes: <description>
Agent 2 changes: <description>

@agent-1-<domain> @agent-2-<domain>
Please coordinate resolution approach."

# 4. Agree on resolution
# 5. Manually merge
# 6. Test thoroughly
# 7. Commit with detailed message
```

---

## 📈 Performance Metrics

### Agent Efficiency Tracking

**Measure per agent:**

```bash
# Tasks completed per day
find .taskmaster/.locks -name "*.claim.completed-*" -mtime -1 | \
  cut -d: -f1 | sort | uniq -c

# Average task duration
# (Track claim timestamp to completion timestamp)

# Blocked task rate
# (blocked tasks / total tasks)

# Rework rate
# (tasks returned to pending after being in-progress)
```

**Target Metrics:**
- Agent utilization: >80% (not idle)
- Task completion rate: 3-5 tasks/agent/day
- Blocked task rate: <5%
- Rework rate: <10%

---

## 🎯 Optimization Strategies

### Load Balancing

**If one tag has backlog:**

```bash
# Move tasks to underutilized tag
task-master use-tag network
task-master list --status=pending

# If network has 10 pending, storage has 2 pending
# Move some network tasks to storage (if cross-domain)
task-master move --from=15,16,17 --to=storage-tag
```

### Parallel Subtask Creation

**For very complex tasks:**

```bash
# Expand task into many small subtasks
task-master expand --id=18 --num=8 --research

# Now 8 subtasks available for work
# Multiple agents can work on subtasks simultaneously
```

---

## 🆘 Emergency Procedures

### All Agents Stuck

**If progress halts:**

```bash
# 1. Check dashboard for status
./taskmaster/scripts/dashboard.sh

# 2. Review blocked tasks
task-master list --status=blocked

# 3. Identify bottleneck
# - Dependency chain issue?
# - All waiting on same blocker?
# - Build broken?

# 4. Escalate to human if >4 hours blocked
```

### Build Completely Broken

**If main build fails:**

```bash
# 1. Stop all agents immediately
pkill -f "agent-"

# 2. Identify breaking change
git log --oneline -10
git diff HEAD~1

# 3. Rollback to last working commit
git revert HEAD

# 4. Verify build works
cd firmware && idf.py build

# 5. Restart agents
# 6. Fix issue properly before re-applying
```

---

## 🎓 Best Practices Summary

**DO:**
- ✅ Work in your assigned tag/domain
- ✅ Use atomic task claiming
- ✅ Communicate via update-subtask
- ✅ Document API contracts clearly
- ✅ Verify build before marking done
- ✅ Log all cross-agent dependencies

**DON'T:**
- ❌ Manually grab tasks without claiming
- ❌ Edit files outside your domain
- ❌ Leave tasks in-progress >4 hours
- ❌ Skip research phase
- ❌ Commit broken builds
- ❌ Make assumptions about other agent's work

---

**Remember:** Isolation + Communication = No Conflicts 🤝

---

**Next:** See [research-first.md](./research-first.md) for research methodology.

**Version:** 1.0  
**Last Updated:** 2025-10-15
