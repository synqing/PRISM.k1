# PRISM.k1 MCP Tool Usage Reference

**Purpose:** Complete guide to all Model Context Protocol (MCP) tools available for agents.

---

## 🔧 Available MCP Tools

### Priority Matrix

| Priority | Tool | Use Case | Frequency |
|----------|------|----------|-----------|
| ⭐⭐⭐⭐⭐ | task-master-ai | Task management | Every task |
| ⭐⭐⭐⭐⭐ | filesystem | Code operations | Every task |
| ⭐⭐⭐⭐⭐ | context7 | ESP-IDF/FreeRTOS docs | Research phase |
| ⭐⭐⭐⭐⭐ | brave-search | Web research | Research phase |
| ⭐⭐⭐⭐ | sequential-thinking | Complex decisions | When stuck |
| ⭐⭐⭐⭐ | memory | Agent state | Cross-session |
| ⭐⭐⭐ | git | Version control | Per completion |
| ⭐⭐ | sqlite | Metrics tracking | Optional |

---

## 1️⃣ task-master-ai (Project Management)

### Purpose
Manage tasks, track progress, coordinate agents, conduct research.

### Core Operations

#### Task Discovery
```javascript
// Get next available task
next_task({ tag: "network" })
// Returns: Task ID and summary

// View task details
get_task({ 
  id: "10",
  tag: "network"
})
// Returns: Full task spec with dependencies, details, test strategy

// List all tasks
get_tasks({ 
  tag: "network",
  status: "pending",
  withSubtasks: true
})
```

#### Task Status Management
```javascript
// Mark task in-progress
set_task_status({
  tag: "network",
  id: "10",
  status: "in-progress"
})

// Mark complete
set_task_status({
  tag: "network",
  id: "10",
  status: "done"
})

// Block if waiting
set_task_status({
  tag: "network",
  id: "10",
  status: "blocked"
})
```

#### Progress Logging
```javascript
// Log implementation progress
update_subtask({
  tag: "network",
  id: "10.2",
  prompt: "Implemented WebSocket binary frame parser. 
  
  Approach: TLV encoding with CRC32 validation
  Memory: 8KB buffer + 256B overhead
  Build: ✅ Passing
  
  Next: Add error recovery for corrupted frames"
})

// Update main task
update_task({
  id: "10",
  tag: "network",
  prompt: "WebSocket server implementation complete.
  
  Features: Binary protocol, TLV encoding, 8KB buffer
  Performance: 500KB/s sustained
  Tests: 12 unit tests passing"
})
```

#### Research (CRITICAL)
```javascript
// Research before implementing
research({
  taskIds: "10",
  tag: "network",
  query: "ESP-IDF 5.x WebSocket binary protocol best practices memory-efficient implementation",
  saveTo: "10",
  detail: "high"
})

// Research with file context
research({
  taskIds: "10,11",
  filePaths: "firmware/components/network/include/websocket.h",
  query: "WebSocket buffer overflow prevention patterns",
  saveFile: true  // Saves to .taskmaster/docs/research/
})
```

#### Task Creation & Expansion
```javascript
// Add new task
add_task({
  tag: "network",
  prompt: "Implement WebSocket ping/pong for connection keepalive",
  priority: "medium",
  research: true
})

// Expand complex task into subtasks
expand_task({
  id: "18",
  tag: "playback",
  num: 5,  // Suggest 5 subtasks
  research: true,  // Use research for informed breakdown
  prompt: "Focus on separation of concerns: timer, queue, generation, output"
})

// Expand all eligible tasks
expand_all({
  tag: "playback",
  research: true
})
```

#### Analysis
```javascript
// Analyze task complexity
analyze_project_complexity({
  tag: "network",
  threshold: 7,  // Recommend expansion if >7
  research: true
})

// View complexity report
complexity_report({ tag: "network" })
```

### Usage Patterns

**Start of Session:**
```javascript
// 1. Get next task
const task = next_task({ tag: "network" });

// 2. Review details
get_task({ id: task.id, tag: "network" });

// 3. Research first
research({
  taskIds: task.id,
  query: "<specific-topic>",
  saveTo: task.id
});

// 4. Log plan
update_subtask({
  id: task.id,
  prompt: "Research complete. Plan: ..."
});

// 5. Set in-progress
set_task_status({ id: task.id, status: "in-progress" });
```

**During Implementation:**
```javascript
// Log progress every 30-60 minutes
update_subtask({
  id: task.id,
  prompt: "Progress: Completed X, Y. Working on Z."
});
```

**Task Completion:**
```javascript
// 1. Document completion
update_subtask({
  id: task.id,
  prompt: "Implementation complete. Build: ✅. Tests: ✅."
});

// 2. Mark done
set_task_status({ id: task.id, status: "done" });

// 3. Get next
next_task({ tag: "network" });
```

---

## 2️⃣ filesystem (File Operations)

### Purpose
Read/write/edit project files directly.

### Core Operations

#### Reading Files
```javascript
// Read single file
const content = await read_file({ 
  path: "/Users/spectrasynq/Workspace_Management/Software/PRISM.k1/firmware/main/main.c"
});

// Read multiple files at once (EFFICIENT)
const files = await read_multiple_files({
  paths: [
    "/Users/spectrasynq/.../firmware/components/network/include/websocket.h",
    "/Users/spectrasynq/.../firmware/components/network/src/websocket.c",
    "/Users/spectrasynq/.../firmware/components/network/CMakeLists.txt"
  ]
});
```

#### Writing Files
```javascript
// Create new file
await write_file({
  path: "/Users/spectrasynq/.../firmware/components/network/src/websocket.c",
  content: `
#include "websocket.h"
#include "esp_log.h"

static const char *TAG = "websocket";

esp_err_t websocket_init(void) {
    ESP_LOGI(TAG, "Initializing WebSocket server");
    // ... implementation ...
    return ESP_OK;
}
  `
});

// Overwrite existing file
await write_file({
  path: "/Users/spectrasynq/.../firmware/main/main.c",
  content: updatedMainContent
});
```

#### Editing Files (Line-Based)
```javascript
// Make targeted edits
await edit_file({
  path: "/Users/spectrasynq/.../firmware/components/network/src/websocket.c",
  edits: [
    {
      oldText: "esp_err_t websocket_init(void) {\n    ESP_LOGI(TAG, \"Initializing\");",
      newText: "esp_err_t websocket_init(const websocket_config_t *config) {\n    ESP_LOGI(TAG, \"Initializing with config\");"
    }
  ]
});

// Dry run to preview changes
await edit_file({
  path: "...",
  edits: [...],
  dryRun: true  // Returns git-style diff
});
```

#### Directory Operations
```javascript
// Create directory
await create_directory({
  path: "/Users/spectrasynq/.../firmware/components/templates/include"
});

// List directory contents
const contents = await list_directory({
  path: "/Users/spectrasynq/.../firmware/components"
});
// Returns: [FILE] websocket.c, [DIR] network, ...

// Get directory tree
const tree = await directory_tree({
  path: "/Users/spectrasynq/.../firmware/components/network"
});
// Returns: JSON tree structure

// Search for files
const results = await search_files({
  path: "/Users/spectrasynq/.../firmware",
  pattern: "websocket",
  excludePatterns: ["build/", "*.o"]
});
```

#### File Metadata
```javascript
// Get file info
const info = await get_file_info({
  path: "/Users/spectrasynq/.../firmware/main/main.c"
});
// Returns: size, created, modified, permissions, type

// Move/rename file
await move_file({
  source: "/Users/spectrasynq/.../firmware/components/network/old_name.c",
  destination: "/Users/spectrasynq/.../firmware/components/network/websocket.c"
});
```

### Usage Patterns

**Implementing a Task:**
```javascript
// 1. Read existing related code
const existingCode = await read_multiple_files({
  paths: [
    "firmware/components/network/include/prism_network.h",
    "firmware/main/main.c"
  ]
});

// 2. Create new component
await create_directory({ path: "firmware/components/network/src" });

await write_file({
  path: "firmware/components/network/src/websocket.c",
  content: implementationCode
});

// 3. Update CMakeLists.txt
await edit_file({
  path: "firmware/components/network/CMakeLists.txt",
  edits: [{
    oldText: "SRCS \"\"",
    newText: "SRCS \"src/websocket.c\""
  }]
});

// 4. Update main.c to call new code
await edit_file({
  path: "firmware/main/main.c",
  edits: [{
    oldText: "// TODO: Initialize components",
    newText: "#include \"websocket.h\"\n// ...\nESP_ERROR_CHECK(websocket_init());"
  }]
});
```

**Allowed Directories:**
```javascript
// Check what directories are accessible
const allowed = await list_allowed_directories();
// Should include: /Users/spectrasynq/Workspace_Management/Software/PRISM.k1
```

---

## 3️⃣ context7 (Documentation)

### Purpose
Fetch up-to-date, version-specific documentation for libraries directly from source.

### Core Operations

#### Resolve Library
```javascript
// Find Context7 ID for a library
const libs = await resolve_library_id({
  libraryName: "esp-idf"
});
// Returns: { libraryId: "/espressif/esp-idf", ... }

// For specific version
const libs = await resolve_library_id({
  libraryName: "esp-idf v5.3"
});
// Returns: { libraryId: "/espressif/esp-idf/v5.3", ... }
```

#### Get Documentation
```javascript
// Fetch documentation
const docs = await get_library_docs({
  context7CompatibleLibraryID: "/espressif/esp-idf/v5.3",
  topic: "websocket",  // Focus on specific topic
  tokens: 5000  // Max tokens to retrieve
});

// More specific topics
const docs = await get_library_docs({
  context7CompatibleLibraryID: "/espressif/esp-idf/v5.3",
  topic: "esp_http_server websocket binary",
  tokens: 8000
});

// FreeRTOS docs
const freertos = await get_library_docs({
  context7CompatibleLibraryID: "/freertos/freertos-kernel",
  topic: "task priorities",
  tokens: 3000
});

// LittleFS docs
const littlefs = await get_library_docs({
  context7CompatibleLibraryID: "/littlefs-project/littlefs",
  topic: "atomic operations",
  tokens: 3000
});
```

### Usage Patterns

**Research Phase:**
```javascript
// 1. Resolve library
const espIdf = await resolve_library_id({ libraryName: "esp-idf v5.3" });

// 2. Get relevant docs
const webSocketDocs = await get_library_docs({
  context7CompatibleLibraryID: espIdf.libraryId,
  topic: "esp_http_server websocket implementation",
  tokens: 5000
});

// 3. Get related docs
const httpServerDocs = await get_library_docs({
  context7CompatibleLibraryID: espIdf.libraryId,
  topic: "httpd_config chunked transfer",
  tokens: 3000
});

// 4. Document findings
await update_subtask({
  id: currentTask,
  prompt: `Research via Context7:

ESP-IDF v5.3 WebSocket Patterns:
- Use httpd_ws_frame_t for frame handling
- Buffer size: 8KB recommended
- Use httpd_ws_recv_frame() for receiving
- Binary frames: HTTPD_WS_TYPE_BINARY

API Reference: ${webSocketDocs.summary}
`
});
```

**Common Libraries for PRISM.k1:**
- `/espressif/esp-idf/v5.3` - ESP-IDF framework
- `/freertos/freertos-kernel` - FreeRTOS RTOS
- `/littlefs-project/littlefs` - LittleFS filesystem

**Note:** Context7 pulls LIVE documentation, not training data. Always current.

---

## 4️⃣ brave-search (Web Research)

### Purpose
Search the web for current best practices, recent developments, and community solutions.

### Core Operations

#### Search Query
```javascript
// Basic search
const results = await web_search({
  query: "ESP32-S3 memory fragmentation prevention 2024"
});

// More specific
const results = await web_search({
  query: "ESP-IDF 5.x WebSocket binary protocol best practices"
});

// Recent tutorials
const results = await web_search({
  query: "RMT peripheral WS2812B 60 FPS tutorial 2025"
});
```

#### Fetch Full Page
```javascript
// Get complete article content
const page = await web_fetch({
  url: "https://docs.espressif.com/projects/esp-idf/en/v5.3/esp32s3/api-reference/peripherals/rmt.html"
});
```

### Usage Patterns

**Research Best Practices:**
```javascript
// 1. Search for current approaches
const results = await web_search({
  query: "ESP32 LittleFS atomic file write 2024"
});

// 2. Read top articles
const article1 = await web_fetch({ url: results[0].url });
const article2 = await web_fetch({ url: results[1].url });

// 3. Synthesize findings
await update_subtask({
  id: currentTask,
  prompt: `Research via Brave Search:

Pattern: .tmp + fsync + rename for atomic writes
Source: ${results[0].title}

Confirmed by: ${results[1].title}

Implementation approach validated.
`
});
```

**Troubleshooting:**
```javascript
// Search for specific errors
const results = await web_search({
  query: "ESP-IDF ESP_ERR_NO_MEM httpd_start solution"
});

// Find community solutions
const results = await web_search({
  query: "ESP32 forum WS2812B timing issues RMT"
});
```

**Rate Limits:**
- FREE tier: 2,000 queries/month
- Beyond: $3 per 1,000 queries

**Best Practices:**
- Use specific queries (include year: 2024-2025)
- Combine library name + version (ESP-IDF 5.x)
- Include problem domain (embedded, real-time, etc.)

---

## 5️⃣ sequential-thinking (Complex Reasoning)

### Purpose
Structured problem-solving for architecture decisions and debugging.

### Core Operations

```javascript
// Invoke sequential thinking for complex problem
const result = await sequential_thinking({
  problem: "Design memory allocation strategy for PRISM.k1:
  
Context:
- ESP32-S3: 512KB total RAM, ~300KB usable
- Need: 60 FPS LED output (no jitter)
- Constraint: Avoid fragmentation (will brick device)
- Requirements: WebSocket uploads, pattern storage, hot cache

Question: Should we use:
A) malloc/free with pools
B) heap_caps_malloc with fixed allocations
C) Pre-allocated static buffers
D) Combination approach

Analyze trade-offs."
});
```

### Usage Patterns

**Architecture Decisions:**
```javascript
// Use for complex design choices
const analysis = await sequential_thinking({
  problem: "Choose pattern file format:

Options:
1. Fixed-size records (fast, inflexible)
2. TLV encoding (flexible, slightly slower)
3. JSON (easy, large size)
4. Protobuf (efficient, complex)

Constraints:
- Must fit 25-35 patterns in 1.47MB
- Parse at 60 FPS for playback
- WebSocket upload over WiFi

Recommendation with rationale?"
});
```

**Debugging:**
```javascript
// Analyze complex bugs
const solution = await sequential_thinking({
  problem: "Memory corruption after 24h runtime:

Symptoms:
- Heap fragmentation increases over time
- Crash at ~23 hours uptime
- Free heap: 150KB → 45KB → crash

Suspected causes:
1. Memory leak in WebSocket handler
2. Fragmentation from pattern cache
3. LittleFS metadata accumulation

Systematic debugging approach?"
});
```

---

## 6️⃣ memory (Agent State)

### Purpose
Persist agent state across sessions, track context.

### Core Operations

```javascript
// Store agent state
await memory_store({
  key: "agent-1-network-state",
  value: JSON.stringify({
    currentTask: "10",
    lastCompleted: "9",
    blockers: [],
    context: "Implementing WebSocket binary protocol"
  })
});

// Retrieve state
const state = await memory_retrieve({
  key: "agent-1-network-state"
});

// Store research findings
await memory_store({
  key: "websocket-research-findings",
  value: JSON.stringify({
    bufferSize: "8KB",
    pattern: "chunked-transfer",
    performance: "500KB/s",
    source: "ESP-IDF 5.3 docs + community"
  })
});
```

### Usage Patterns

**Session Continuity:**
```javascript
// On agent startup
const lastState = await memory_retrieve({
  key: "agent-state"
});

if (lastState) {
  console.log("Resuming from:", lastState.currentTask);
}

// During work
await memory_store({
  key: "agent-state",
  value: { currentTask, progress: "70%" }
});

// On shutdown
await memory_store({
  key: "agent-state",
  value: { status: "shutdown", reason: "clean" }
});
```

---

## 7️⃣ git (Version Control)

### Purpose
Git operations for version control.

### Core Operations

```javascript
// Check status
const status = await git_status();

// Stage files
await git_add({ files: ["firmware/components/network/src/websocket.c"] });

// Commit
await git_commit({
  message: "feat: implement WebSocket binary protocol (task 10)\n\n- TLV encoding\n- 8KB buffer\n- CRC32 validation"
});

// View log
const log = await git_log({ count: 10 });

// Create branch
await git_branch({ name: "feature/websocket" });
```

### Usage Patterns

**Per-Task Completion:**
```javascript
// After completing task
await git_add({ files: ["firmware/components/..."] });
await git_commit({
  message: `feat: implement ${taskTitle} (task ${taskId})\n\nDetails: ${summary}`
});
```

---

## 8️⃣ sqlite (Metrics Tracking)

### Purpose
Local database for metrics and analytics.

### Core Operations

```javascript
// Create metrics table
await sqlite_execute({
  query: `
CREATE TABLE IF NOT EXISTS task_metrics (
  id INTEGER PRIMARY KEY,
  task_id TEXT,
  agent_id TEXT,
  start_time INTEGER,
  end_time INTEGER,
  duration INTEGER,
  status TEXT
);
  `
});

// Insert metric
await sqlite_execute({
  query: "INSERT INTO task_metrics VALUES (?, ?, ?, ?, ?, ?, ?)",
  params: [null, "10", "agent-1", startTime, endTime, duration, "done"]
});

// Query metrics
const results = await sqlite_query({
  query: "SELECT * FROM task_metrics WHERE agent_id = ? ORDER BY end_time DESC",
  params: ["agent-1"]
});
```

### Usage Patterns

**Performance Tracking:**
```javascript
// Track task duration
const start = Date.now();
// ... work on task ...
const end = Date.now();

await sqlite_execute({
  query: "INSERT INTO task_metrics (task_id, agent_id, duration) VALUES (?, ?, ?)",
  params: [taskId, agentId, end - start]
});
```

---

## 🎯 Tool Selection Decision Tree

```
Need to manage tasks? → task-master-ai
Need to read/write files? → filesystem
Need library documentation? → context7
Need recent best practices? → brave-search
Complex architecture decision? → sequential-thinking
Need cross-session state? → memory
Need version control? → git
Need metrics tracking? → sqlite
```

---

## 🚀 Optimal Workflow Integration

### Task Execution Pattern

```javascript
// 1. Get task (task-master-ai)
const task = await next_task({ tag: "network" });

// 2. Research (context7 + brave-search)
const docs = await get_library_docs({
  context7CompatibleLibraryID: "/espressif/esp-idf/v5.3",
  topic: task.title
});

const bestPractices = await web_search({
  query: `${task.title} ESP32 best practices 2024`
});

// 3. Plan (task-master-ai)
await update_subtask({
  id: task.id,
  prompt: "Research complete. Plan: ..."
});

// 4. Implement (filesystem)
const existing = await read_file({ path: "..." });
await write_file({ path: "...", content: newCode });

// 5. Verify (filesystem + git)
// Build check: cd firmware && idf.py build
await git_add({ files: [...] });
await git_commit({ message: "..." });

// 6. Complete (task-master-ai)
await update_subtask({
  id: task.id,
  prompt: "Implementation complete. Build: ✅"
});

await set_task_status({
  id: task.id,
  status: "done"
});
```

---

**Remember:** Right tool for the right job. Don't use a hammer when you need a screwdriver. 🔧

---

**Version:** 1.0  
**Last Updated:** 2025-10-15
