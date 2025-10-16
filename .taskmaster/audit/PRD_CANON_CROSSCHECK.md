# PRD vs CANON Cross-Check Analysis

**Date:** 2025-10-15
**Analyst:** Agent-Documentation-001
**Purpose:** Identify conflicts between PRD requirements and CANON specifications

---

## 🎯 Executive Summary

**Status:** ⚠️ **CONFLICTS DETECTED**

Found **3 major conflicts** between PRD and CANON specifications:
1. WebSocket Buffer Size (8KB vs 4KB)
2. LED Count (150 vs 320)
3. Maximum Pattern Size (200KB vs 256KB)

**Recommendation:** CANON specifications are evidence-based (ADRs with research). Suggest PRD update to match CANON, OR create new ADRs if PRD requirements have changed.

---

## 📊 Detailed Comparison

### ✅ **MATCHES (Specifications Aligned)**

| Specification | PRD Value | CANON Value | Status |
|--------------|-----------|-------------|---------|
| **Storage Path** | `/littlefs` (line 115) | `/littlefs` (CANON §5) | ✅ MATCH |
| **Storage Type** | LittleFS | LittleFS | ✅ MATCH |
| **WebSocket Protocol** | Binary TLV | Not in CANON | ℹ️ PRD more detailed |
| **LED Type** | WS2812B | WS2812B (CANON §3) | ✅ MATCH |
| **LED FPS Target** | 60 FPS (line 112) | 60 FPS (CANON §3) | ✅ MATCH |

---

### ⚠️ **CONFLICTS (Specifications Differ)**

#### **Conflict 1: WebSocket Buffer Size**

| Source | Value | Evidence | Authority |
|--------|-------|----------|-----------|
| **PRD** | `8192` bytes (8KB) | Line 109: `#define WS_BUFFER_SIZE 8192` | Original requirement |
| **CANON** | `4096` bytes (4KB) | ADR-002: Research shows 98% success rate vs 85% for 8KB after 12h | Evidence-based decision |

**Analysis:**
- ADR-002 documents research showing 8KB causes fragmentation after 12 hours
- 4KB maintains 98% allocation success over 24+ hours
- PRD predates fragmentation research

**Recommendation:** ✅ **Use CANON (4KB)** - Evidence shows 8KB is unreliable long-term

**Impact:** Affects `websocket_handler.c`, protocol throughput calculations

---

#### **Conflict 2: LED Count**

| Source | Value | Evidence | Authority |
|--------|-------|----------|-----------|
| **PRD** | `150` LEDs | Line 113: `#define LED_COUNT 150` | Original requirement |
| **CANON** | `320` LEDs | ADR-003: LED Count Standardization | Approved decision |

**Analysis:**
- PRD specifies 150 LEDs (testing hardware: line 316)
- CANON specifies 320 LEDs (more than 2x difference)
- This affects memory calculations, pattern sizing, frame buffer

**Memory Impact:**
- 150 LEDs: 450 bytes per frame (150 × 3 RGB)
- 320 LEDs: 960 bytes per frame (320 × 3 RGB)
- **2.13x increase in memory requirements**

**Recommendation:** ⚠️ **NEED CAPTAIN DECISION**
- If hardware changed to 320 LEDs → Use CANON
- If still using 150 LED hardware → Update ADR-003

**Impact:** Affects ALL LED-related code, memory budgets, pattern formats

---

#### **Conflict 3: Maximum Pattern Size**

| Source | Value | Evidence | Authority |
|--------|-------|----------|-----------|
| **PRD** | `204800` bytes (200KB) | Line 117: `#define MAX_PATTERN_SIZE 204800` | Original requirement |
| **CANON** | `262144` bytes (256KB) | ADR-004: Pattern Maximum Size | Approved decision |

**Analysis:**
- PRD: 200KB limit
- CANON: 256KB limit (28% larger)
- Both fit within 1.5MB storage partition

**Storage Impact:**
- PRD 200KB: ~7 patterns minimum (1.5MB ÷ 200KB = 7.5)
- CANON 256KB: ~6 patterns minimum (1.5MB ÷ 256KB = 5.86)
- PRD requires 25-35 patterns (line 16)

**Recommendation:** ⚠️ **INVESTIGATE**
- Need to verify: Can 25+ patterns of 256KB each fit in 1.5MB?
- CANON specifies "minimum 25 patterns must fit" (line 117)
- Math: 25 × 256KB = 6.4MB (EXCEEDS 1.5MB partition!)

**🚨 POTENTIAL ISSUE:** CANON specification may be mathematically impossible!

**Impact:** Affects storage system, upload validation, pattern manager

---

### 📝 **MISSING IN CANON (PRD Specifications Not Yet in ADRs)**

These PRD requirements have NO corresponding CANON specifications:

#### **Network & Protocol**

| PRD Specification | Line | Status in CANON |
|-------------------|------|-----------------|
| WebSocket message types (PUT_BEGIN, PUT_DATA, etc.) | 24 | ❌ Missing |
| Binary TLV format details | 124-132 | ❌ Missing |
| Error codes (0x01, 0x02, 0x03) | 27 | ❌ Missing |
| Message state machine | 135-141 | ❌ Missing |
| WebSocket timeout: 5000ms | 110 | ✅ In CANON (§2) |
| Max clients: (not in PRD) | - | ✅ In CANON (§2): 2 clients |

**Recommendation:** Create ADR-006: WebSocket Protocol Specification

---

#### **Template System**

| PRD Specification | Line | Status in CANON |
|-------------------|------|-----------------|
| 15 templates required | 17, 44, 100 | ❌ Missing |
| Template categories (Ambient 5, Energy 5, Special 5) | 45-46 | ❌ Missing |
| Template storage location (flash vs LittleFS) | 44 | ❌ Missing |
| Template deployment mechanism | 46 | ❌ Missing |

**Recommendation:** Create ADR-007: Template System Architecture

---

#### **Performance Requirements**

| PRD Specification | Line | Status in CANON |
|-------------------|------|-----------------|
| <100ms pattern switch | 14, 299 | ❌ Missing |
| <150KB heap usage | 13, 149, 301 | ❌ Missing |
| 99.9% uptime | 15 | ❌ Missing |
| Zero fragmentation after 24h | 13, 302 | ❌ Missing |
| 500KB/s WebSocket throughput | 18, 159, 325 | ❌ Missing |

**Recommendation:** Create ADR-008: Performance Targets & Success Criteria

---

#### **Storage Details**

| PRD Specification | Line | Status in CANON |
|-------------------|------|-----------------|
| Pattern header size (1KB) | 33 | ❌ Missing |
| Hot cache (3-5 patterns in RAM) | 42 | ❌ Missing |
| CRC32 validation | 34 | ❌ Missing |
| LRU eviction policy | 89 | ❌ Missing |

**Recommendation:** Create ADR-009: Storage System Implementation

---

#### **Hardware & Architecture**

| PRD Specification | Line | Status in CANON |
|-------------------|------|-----------------|
| ESP32-S3 dual-core 240MHz | 59 | ❌ Missing |
| 512KB RAM (300KB usable) | 60 | ❌ Missing |
| 8MB Flash | 61 | ❌ Missing |
| RMT peripheral for LED | 62 | ❌ Missing |
| WiFi AP mode | 50 | ❌ Missing |
| mDNS: "prism-k1.local" | 51 | ❌ Missing |

**Recommendation:** Create ADR-010: Hardware Configuration

---

## 🎯 **Priority Actions**

### **Immediate (Blocking)**

1. **Resolve LED Count Conflict**
   - Captain decision: 150 or 320 LEDs?
   - Update either PRD or ADR-003
   - Affects ALL subsequent work

2. **Resolve Buffer Size Conflict**
   - Recommend: Use CANON (4KB) based on fragmentation research
   - Update PRD line 109
   - Or: Challenge ADR-002 with new evidence

3. **Investigate Pattern Size Math**
   - CANON: 256KB max + 25 patterns min = 6.4MB required
   - Storage: Only 1.5MB available
   - **Mathematics don't work!**
   - Likely need: Smaller max size OR fewer required patterns

### **High Priority (This Week)**

4. **Create Missing ADRs**
   - ADR-006: WebSocket Protocol Specification
   - ADR-007: Template System (15 templates)
   - ADR-008: Performance Targets
   - ADR-009: Storage Implementation Details
   - ADR-010: Hardware Configuration

5. **Update CANON**
   - Regenerate from all new ADRs
   - Validate code synchronization
   - Update cross-references

### **Medium Priority (Next Week)**

6. **Validate 56 Deprecated Tasks**
   - Cross-check each task against updated CANON
   - Fix the 5-10% drift
   - Re-validate task dependencies

7. **Validate Template Specifications**
   - Cross-check against CANON
   - Fix the 5% conflicts
   - Ensure all 15 templates defined

---

## 📋 **Recommendations for Captain**

### **Decision Required #1: LED Count**

**Options:**
- **A) Use CANON (320 LEDs)** - If hardware design changed
- **B) Use PRD (150 LEDs)** - If using original hardware
- **C) Support both** - Create configuration option (increases complexity)

**My recommendation:** Choose ONE. Configuration increases complexity without clear benefit.

---

### **Decision Required #2: Pattern Size Math**

**The Problem:**
- CANON: 256KB max per pattern
- CANON: 25 patterns minimum
- Storage: 1.5MB available
- Math: 25 × 256KB = 6.4MB ❌ IMPOSSIBLE

**Options:**
- **A) Reduce max pattern size** to 60KB (25 × 60KB = 1.5MB)
- **B) Reduce pattern count** to 6 minimum (6 × 256KB = 1.5MB)
- **C) Challenge assumption** - Patterns use compression/deduplication

**My recommendation:** Need to understand if "256KB max" means uncompressed or on-disk size. If PRD's "structural efficiency: 40% size reduction" (line 32) is real, then 256KB uncompressed → ~150KB on disk → ~10 patterns fit.

**Action:** Create ADR documenting actual storage math with compression.

---

### **Decision Required #3: Process Going Forward**

**Options:**
- **A) PRD is authoritative** - Update all ADRs to match PRD
- **B) CANON is authoritative** - Update PRD to match CANON
- **C) Hybrid** - CANON for technical decisions, PRD for business requirements

**My recommendation:** **Option C (Hybrid)**
- Business requirements (15 templates, 60s setup) → PRD
- Technical implementations (buffer sizes, partition layout) → CANON
- When they conflict → Research + ADR resolves it

---

## 🔄 **Suggested Workflow**

```
1. Captain resolves 3 conflicts (LED count, buffer, pattern size)
   ↓
2. Update either PRD or CANON to reflect decisions
   ↓
3. Create 5 missing ADRs (protocol, templates, performance, storage, hardware)
   ↓
4. Regenerate CANON from all ADRs
   ↓
5. Cross-check 56 deprecated tasks against updated CANON
   ↓
6. Fix drifted tasks (5-10%)
   ↓
7. Cross-check template specs against updated CANON
   ↓
8. Fix drifted templates (5%)
   ↓
9. Create clean task breakdown with CANON references
   ↓
10. Begin autonomous multi-agent execution
```

---

## 📊 **Summary Statistics**

**PRD Specifications:** ~50 distinct requirements
**CANON Specifications:** 5 ADRs covering ~8 specifications
**Coverage:** ~16% (8/50)

**Conflicts:** 3 major
**Matches:** 5 confirmed
**Missing in CANON:** ~42 specifications

**Conclusion:** CANON is 84% incomplete relative to PRD scope. Need ~10-15 more ADRs to achieve full coverage.

---

## ✅ **Next Steps (Awaiting Captain Approval)**

**Option A: Resolve conflicts first, then continue**
1. Captain decides 3 conflicts
2. Update PRD or CANON
3. Proceed with task validation

**Option B: Create missing ADRs first**
1. Draft 5 high-priority ADRs
2. Captain reviews and approves
3. Update CANON
4. Then resolve conflicts

**Option C: Validate tasks with current CANON**
1. Accept conflicts for now
2. Validate 56 tasks against partial CANON
3. Flag conflicts for later resolution

**My recommendation: Option A** - Resolve conflicts first, provides solid foundation.

---

**Status:** AWAITING CAPTAIN DECISION
**Blocking:** LED count, buffer size, pattern size math
**Ready for:** ADR creation once conflicts resolved
