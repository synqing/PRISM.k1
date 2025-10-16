# Agent 2 Handover Brief - Build Integration Fix

**Date:** 2025-10-16 04:15
**Status:** URGENT - Build currently FAILING
**Your Role:** Agent 2 - Network/WiFi/WebSocket Track
**PM:** Project Manager (this Claude Code session)
**Reporting:** Report all progress to PM, await approvals before proceeding

---

## 🎯 IMMEDIATE MISSION

**Fix build integration issues to get clean compile, then proceed to Phase 3.**

You have successfully completed Task 4 Phases 1-2 (TLV Protocol Parser), but there are **API mismatches** preventing the build from succeeding. Your job is to:

1. ✅ **Fix network_manager.c WebSocket integration** (API mismatch)
2. ✅ **Fix storage component fsync header** (missing include)
3. ✅ **Verify build succeeds** (`idf.py build`)
4. ✅ **Report success to PM**
5. ⏭️ **Await PM approval for Phase 3**

---

## 📊 CURRENT STATUS

### Tasks Complete ✅
- **Task 2** (WiFi Manager) - Complete, committed
- **Task 3** (WebSocket Binary Server) - Complete, committed
- **Task 4 Phase 1** (TLV Parser + CRC32) - Complete, committed at `7ca8f8d`
- **Task 4 Phase 2** (Upload State Machine) - Complete, committed at `f351d4a`

### Task 4 Remaining
- **Phase 3** (Storage Integration) - BLOCKED by build failures
- **Phase 4** (Playback Integration) - Pending
- **Phase 5** (Unit Tests) - Pending

### Your Commits
```
f351d4a feat(task-4.2): Implement Phase 2 upload session state machine
7ca8f8d feat(task-4.1): Implement Phase 1 TLV parser with CRC32 validation
adcc46f feat(task-3): Complete WebSocket binary TLV server implementation
```

---

## 🔴 BUILD FAILURES (2 Errors)

### Error 1: network_manager.c API Mismatch

**File:** `firmware/components/network/network_manager.c`
**Lines:** 1240-1315 (handle_ws_frame function)
**Problem:** WebSocket handler calls functions that DON'T EXIST

**Current broken code (lines 1242-1270):**
```c
// ❌ WRONG - These functions don't exist in protocol_parser.h
tlv_frame_t frame;
ret = protocol_parse_tlv(ws_pkt.payload, ws_pkt.len, &frame);  // ❌ No such function
if (ret != ESP_OK) {
    // ... error handling with protocol_create_error_frame() ❌ Also doesn't exist
}
ret = protocol_dispatch_command(client_idx, &frame);  // ❌ Wrong signature
```

**Your ACTUAL API (protocol_parser.h:182-186):**
```c
esp_err_t protocol_dispatch_command(
    const uint8_t* frame_data,   // Raw WebSocket binary data
    size_t frame_len,             // Frame length
    int client_fd                 // WebSocket client file descriptor
);
```

**Root Cause:** The network_manager.c WebSocket integration was written BEFORE you implemented the protocol_parser API. It's calling a hypothetical API that was never built.

**Your protocol_dispatch_command() ALREADY handles:**
- ✅ TLV frame parsing
- ✅ CRC32 validation
- ✅ Command dispatch to handlers
- ✅ Error responses (internally)

**Fix Required:**

Replace lines 1240-1315 in `handle_ws_frame()` with a SIMPLE call to your API:

```c
// Task 4: Dispatch raw WebSocket binary frame to protocol parser
// The parser handles: TLV parsing, CRC validation, command dispatch, error responses
ret = protocol_dispatch_command(
    ws_pkt.payload,   // Raw binary frame from WebSocket
    ws_pkt.len,       // Frame length
    sockfd            // Client file descriptor (get from req)
);

if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Protocol dispatch failed: %s", esp_err_to_name(ret));
    return ESP_FAIL;
}

return ESP_OK;
```

**IMPORTANT:** You need to get the client file descriptor (`sockfd`) from the WebSocket request. Check how `handle_ws_frame()` is called and extract the FD from the `httpd_req_t` structure.

---

### Error 2: Storage Component Missing Header

**File:** `firmware/components/storage/pattern_storage_crud.c:302`
**Error:** `implicit declaration of function 'fsync'`
**Fix:** Add `#include <unistd.h>` at the top of the file

This is Agent 3's code (Task 5), but you can fix it quickly since it's a trivial one-line change.

---

## 📋 YOUR ACTION PLAN

### Step 1: Fix Storage Header (2 minutes)
```bash
cd firmware/components/storage
# Add #include <unistd.h> to pattern_storage_crud.c after existing includes
```

**Location:** After line ~20 (after other system includes)

### Step 2: Fix WebSocket Integration (15 minutes)

1. **Read the context:**
   ```bash
   # Understand current handle_ws_frame implementation
   Read: firmware/components/network/network_manager.c lines 1200-1320

   # Understand your protocol_parser API
   Read: firmware/components/network/include/protocol_parser.h lines 140-220
   ```

2. **Locate client file descriptor:**
   - The `handle_ws_frame()` function receives `httpd_req_t *req`
   - Extract the socket FD from the request structure
   - Check how other WebSocket code in network_manager.c gets the FD

3. **Replace broken API calls:**
   - Delete lines 1240-1315 (all the broken protocol_parse_tlv/create_error_frame code)
   - Replace with simple call to `protocol_dispatch_command(payload, len, sockfd)`
   - Keep minimal error logging

4. **Simplify the handler:**
   Your `protocol_dispatch_command()` is a **complete black box** that handles everything internally:
   - Parsing TLV frames
   - Validating CRC32
   - Dispatching to PUT_BEGIN/PUT_DATA/PUT_END handlers
   - Sending ACK/ERROR responses back to client

   The WebSocket handler should just pass raw data and let your parser handle everything.

### Step 3: Verify Build (5 minutes)
```bash
cd firmware
idf.py build 2>&1 | tee build_output.log

# Check for success
echo $?  # Should be 0
ls -lh build/*.bin  # Should show binary files
```

### Step 4: Commit Fixes
```bash
git add firmware/components/network/network_manager.c
git add firmware/components/storage/pattern_storage_crud.c
git commit -m "fix(task-4): Correct network_manager.c API integration with protocol_parser

Fixes build failures from API mismatch between WebSocket handler and
protocol_parser implementation.

**Changes:**
- Simplified handle_ws_frame() to call protocol_dispatch_command() directly
- Removed calls to non-existent functions (protocol_parse_tlv, protocol_create_error_frame)
- Fixed storage component fsync() header include

**Root Cause:**
network_manager.c WebSocket integration was written before protocol_parser
API was implemented, using hypothetical functions that were never built.

**Resolution:**
protocol_dispatch_command() already handles TLV parsing, CRC validation,
command dispatch, and error responses internally. WebSocket handler now
simply passes raw binary frames to the parser.

**Build Status:**
✅ Clean compile
Binary size: [FILL IN]
Free space: [FILL IN]

🤖 Generated with [Claude Code](https://claude.com/claude-code)

Co-Authored-By: Claude <noreply@anthropic.com>"
```

### Step 5: Report to PM

Send this exact message format:

```
AGENT 2 REPORTS: Build Integration Fix Complete ✅

**Status:** All build errors resolved, clean compile achieved

**Changes Made:**
1. Fixed network_manager.c WebSocket integration (lines 1240-1315)
   - Replaced broken API calls with protocol_dispatch_command()
   - Extracted client FD from httpd_req_t

2. Fixed storage component fsync() header
   - Added #include <unistd.h> to pattern_storage_crud.c

**Build Results:**
✅ Compilation: SUCCESS
Binary size: [SIZE] KB
Flash usage: [PERCENTAGE]%
Free space: [FREE] KB

**Commit:** [SHORT HASH] "fix(task-4): Correct network_manager.c API integration"

**Ready for Phase 3:** Awaiting PM approval to integrate Task 5 storage API

**Questions:** [ANY BLOCKERS OR QUESTIONS]
```

---

## 📚 CRITICAL CONTEXT

### Your Protocol Parser API (Phase 1-2 Complete)

**File:** `firmware/components/network/include/protocol_parser.h`

**Public API Functions:**
```c
// Initialize parser (call once at startup)
esp_err_t protocol_parser_init(void);

// Cleanup (call at shutdown)
void protocol_parser_deinit(void);

// Main entry point - processes raw WebSocket frames
esp_err_t protocol_dispatch_command(
    const uint8_t* frame_data,  // Raw binary TLV frame
    size_t frame_len,            // Frame length (must be >= 7 bytes)
    int client_fd                // WebSocket client FD for responses
);

// Periodic timeout check (call from network_task every 1s)
void protocol_check_upload_timeout(void);

// Query upload status
bool protocol_get_upload_status(
    char* out_filename,
    uint32_t* out_bytes_received,
    uint32_t* out_total_size
);
```

**What protocol_dispatch_command() Does Internally:**
1. Validates frame length (minimum 7 bytes)
2. Parses TLV frame structure: `[TYPE:1][LENGTH:2][PAYLOAD:N][CRC:4]`
3. Validates CRC32 checksum (big-endian)
4. Extracts payload based on LENGTH field (big-endian)
5. Dispatches to appropriate handler:
   - 0x10 (PUT_BEGIN) → `handle_put_begin()`
   - 0x11 (PUT_DATA) → `handle_put_data()`
   - 0x12 (PUT_END) → `handle_put_end()`
   - 0x20 (CONTROL) → stub (Phase 4)
   - 0x30 (STATUS) → stub (Phase 4)
6. Sends ACK/ERROR responses back to client (via client_fd)

**Response Handling:** Your parser handles responses internally. The WebSocket handler does NOT need to:
- Create ACK frames
- Create ERROR frames
- Parse return codes to determine response types
- Send responses manually

All of that is handled inside `protocol_dispatch_command()`.

### Upload State Machine (Phase 2 Complete)

**States:** IDLE → RECEIVING → VALIDATING → STORING → IDLE

**Handlers Implemented:**
- `handle_put_begin()` - Allocates buffer, initializes session
- `handle_put_data()` - Receives chunks, validates offsets
- `handle_put_end()` - Validates CRC, writes to storage (stub)

**Storage Integration (Phase 3 - NOT YET DONE):**
- Currently has a stub at line ~504 in protocol_parser.c
- Needs to call Task 5 API: `template_storage_write()`
- Waiting for build to pass before implementing

---

## 🚫 WHAT NOT TO DO

### ❌ Don't Rewrite protocol_parser.c
Your Phase 1-2 implementation is correct and committed. Don't touch it.

### ❌ Don't Add New Functions to protocol_parser.h
The API is complete. Don't add `protocol_parse_tlv()` or `protocol_create_error_frame()`. Those functions were never supposed to exist - they're implementation details.

### ❌ Don't Change Function Signatures
Your `protocol_dispatch_command(data, len, fd)` signature is correct per your architecture.

### ❌ Don't Implement Phase 3 Yet
Fix the build first. Phase 3 (storage integration) requires PM approval.

---

## 🎯 SUCCESS CRITERIA

**Build Fix Complete When:**
1. ✅ `idf.py build` completes with exit code 0
2. ✅ No compilation errors in network or storage components
3. ✅ Binary files generated in `firmware/build/`
4. ✅ Changes committed with proper commit message
5. ✅ PM notified with build report

**Then:** STOP and await PM approval for Phase 3.

---

## 📖 KEY FILES

### Files You Need to Modify
```
firmware/components/network/network_manager.c      (Fix API calls, ~20 line change)
firmware/components/storage/pattern_storage_crud.c (Add header, 1 line)
```

### Files You Should NOT Touch
```
firmware/components/network/protocol_parser.c      (Your Phase 1-2 code, committed)
firmware/components/network/include/protocol_parser.h (Your API, committed)
```

### Files to Read for Context
```
firmware/components/network/include/protocol_parser.h (Your API reference)
firmware/components/network/network_manager.c         (Understand handle_ws_frame)
.taskmaster/docs/prism-firmware-prd.txt              (PRD authority, lines 148-157)
```

---

## 🔧 DEBUGGING TIPS

### If Build Still Fails After Your Fix

**Check for:**
1. Missing client FD extraction from httpd_req_t
2. Wrong number of arguments to protocol_dispatch_command()
3. Typos in function names
4. Missing header includes

**Get Full Error Output:**
```bash
idf.py build 2>&1 | tee build_full.log
grep -i "error:" build_full.log
```

**Check Your Changes Compile in Isolation:**
```bash
idf.py build 2>&1 | grep "network_manager.c"
```

### If You Need Help

**Report to PM with:**
- Exact error message (copy-paste)
- File and line number
- What you tried
- Current git diff of your changes

---

## 📞 COMMUNICATION PROTOCOL

### Report Format

**Every message to PM should start with:**
```
AGENT 2 REPORTS: [SUBJECT]
```

**Status updates should include:**
- Current subtask status
- Blockers (if any)
- Questions (if any)
- Next steps (awaiting approval or proceeding)

**When blocked:**
```
AGENT 2 REPORTS: BLOCKED - [REASON]

**Issue:** [DESCRIBE PROBLEM]
**Attempted:** [WHAT YOU TRIED]
**Need:** [WHAT YOU NEED FROM PM]
```

---

## 🎓 MULTI-AGENT CONTEXT

### Other Agents
- **Agent 3** (Storage Track) - Task 5 (Pattern Storage), has uncommitted work
- **Agent 4** (Hardware Track) - Task 8 (LED Driver), complete
- **PM** (This session) - Coordination, approvals, build verification

### Coordination Rules
1. **Commit after EVERY subtask** - Never leave work uncommitted
2. **Report to PM before proceeding** - Get approval for major changes
3. **Build verification before moving forward** - Always ensure clean compile
4. **Respect git history** - Pull before push, don't force-push

---

## 🚀 READY TO START?

**Your immediate next action:**

1. Read this entire brief (you're doing it now!)
2. Check current git status: `git status`
3. Verify your commits are in history: `git log --oneline -5`
4. Read the broken code: `firmware/components/network/network_manager.c:1240-1315`
5. Read your API: `firmware/components/network/include/protocol_parser.h:140-220`
6. Fix storage header first (quick win)
7. Fix network_manager.c integration
8. Build and verify
9. Commit with proper message
10. Report to PM

**Estimated Time:** 20-30 minutes

**Questions?** Ask PM before starting if anything is unclear.

**Good luck, Agent 2! Let's get this build passing! 💪**

---

**PM Contact:** This Claude Code session
**Session Started:** 2025-10-16 04:15
**Priority:** URGENT
**Expected Completion:** Within 30 minutes
