# Specification Conflict Resolution Example

**Document Type:** Conflict Resolution Case Study
**Date:** 2025-10-17
**Status:** RESOLVED
**Resolution:** ADR-007 (Filesystem Mount Path Specification)

---

## Overview

This document demonstrates the **Specification Conflict Resolution Process** used in the PRISM K1 Knowledge Fortress system. It shows how to identify, document, research, and resolve conflicts between competing specifications.

**Key Learning:**
- Conflicts must be resolved through evidence-based ADR
- Never implement either specification until conflict resolved
- CANON becomes the new single source of truth after resolution

---

## Conflict Discovery

### When Discovered

**Date:** 2025-10-17 09:00:00 UTC
**Discovered By:** Agent-Implementation-003
**Task Context:** Implementing LittleFS storage initialization

### How Discovered

Agent was implementing storage mount code and found conflicting specifications:

```c
// Agent attempted to implement this:
esp_vfs_littlefs_conf_t conf = {
    .base_path = ???,  // WHICH PATH TO USE?
    ...
};
```

**Two different specifications found:**

1. **PRD (Product Requirements Document) - Line 287:**
   ```
   Storage mount path: /prism
   ```

2. **PRISM_AUTHORITATIVE_SPECIFICATION.md - Line 1453:**
   ```yaml
   storage:
     mount_path: "/littlefs"
   ```

**Immediate Action Taken:**
Agent **STOPPED implementation** and documented the conflict (correct procedure).

---

## Conflict Documentation

### Conflict Report Template

```markdown
## SPECIFICATION CONFLICT

**Discovered:** 2025-10-17 09:00:00 UTC
**Discovered By:** Agent-Implementation-003
**Component:** Storage subsystem (LittleFS mount path)
**Impact:** BLOCKING (cannot implement without resolution)

### Conflicting Specifications

**Source A:** PRD Section 5.3 (line 287)
- **Specification:** Mount path = `/prism`
- **Date:** 2025-09-15 (PRD last updated)
- **Authority:** Original product requirements

**Source B:** PRISM_AUTHORITATIVE_SPECIFICATION.md (line 1453)
- **Specification:** Mount path = `/littlefs`
- **Date:** 2025-10-10 (last specification reconciliation)
- **Authority:** Technical specification (post-reconciliation)

**Current Code:** Not implemented (blocked on this conflict)

### Context

LittleFS filesystem requires VFS mount path for file access. Application code will use this path to open files:
```c
FILE *fp = fopen("/???/patterns/default.bin", "r");
```

Both `/prism` and `/littlefs` would work technically, but:
- All code must use consistent path
- Future ADRs will reference this path
- Documentation must specify correct path

### Why This Matters

**Impact Assessment:**

1. **Code Consistency:** All file operations must use same base path
2. **API Surface:** Path becomes part of public API (WebSocket commands use paths)
3. **Documentation:** User-facing docs will specify path
4. **Migration:** Changing path later requires data migration
5. **Tooling:** Upload/download scripts hard-code path

**This is a BLOCKING conflict.** Cannot proceed with implementation until resolved.

### Specifications Authority Hierarchy

According to METHODOLOGY.md, authority order is:

1. **CANON.md** (highest) - Currently does NOT specify mount path (conflict pre-dates CANON)
2. **Approved ADRs** - No ADR exists for mount path yet
3. **Validated Research** - No research on mount path (naming decision, not technical)
4. **PRISM_AUTHORITATIVE_SPECIFICATION.md** - Specifies `/littlefs`
5. **PRD** - Specifies `/prism`
6. **Code** - Not yet implemented

**Conclusion:** PRISM_AUTHORITATIVE_SPECIFICATION.md has higher authority than PRD, but this conflict requires ADR to become definitive.
```

---

## Conflict Analysis

### Step 1: Understand Both Positions

**Position A: `/prism` (from PRD)**

**Rationale:**
- Product name is "PRISM K1" - mount path reflects product identity
- User-facing path should be memorable and branded
- Marketing preference for branded naming

**Evidence:**
- PRD Section 5.3 explicitly specifies `/prism`
- No technical justification provided in PRD

**Stakeholder:** Product team (original requirement)

---

**Position B: `/littlefs` (from PRISM_AUTHORITATIVE_SPECIFICATION.md)**

**Rationale:**
- ESP-IDF convention: mount path matches filesystem type
- Debugging clarity: path name indicates filesystem implementation
- Technical best practice for embedded systems

**Evidence:**
- ESP-IDF examples use `/littlefs` pattern (e.g., spiffs → `/spiffs`, littlefs → `/littlefs`)
- PRISM_AUTHORITATIVE_SPECIFICATION.md was result of specification reconciliation (Oct 2025)
- Technical team consensus during reconciliation

**Stakeholder:** Engineering team (technical specification)

### Step 2: Research Both Approaches

#### Research: ESP-IDF Naming Conventions

**Source:** ESP-IDF examples and documentation

**Findings:**

1. **ESP-IDF littlefs example** (official):
   ```c
   esp_vfs_littlefs_conf_t conf = {
       .base_path = "/littlefs",  // Uses filesystem-type naming
       ...
   };
   ```

2. **ESP-IDF spiffs example** (official):
   ```c
   esp_vfs_spiffs_conf_t conf = {
       .base_path = "/spiffs",  // Also uses filesystem-type naming
       ...
   };
   ```

3. **ESP-IDF FAT filesystem example** (official):
   ```c
   esp_vfs_fat_mount_config_t conf = {
       .base_path = "/sdcard",  // Uses storage-medium naming (exception)
       ...
   };
   ```

**Pattern Observed:**
- Virtual filesystems: named by filesystem type (`/spiffs`, `/littlefs`)
- Physical storage: named by storage medium (`/sdcard`, `/usb`)
- LittleFS is a virtual filesystem type

**Conclusion from ESP-IDF patterns:**
ESP-IDF convention strongly favors `/littlefs` for LittleFS virtual filesystem.

#### Research: Industry Best Practices

**Sources:**
- Linux FHS (Filesystem Hierarchy Standard)
- Embedded Linux naming conventions
- POSIX filesystem mounting standards

**Findings:**

**Linux mounting conventions:**
- Temporary mounts: `/mnt/<descriptive-name>` or `/media/<descriptive-name>`
- Persistent mounts: Named by purpose (e.g., `/home`, `/var`, `/usr`)
- Virtual filesystems: Named by type (e.g., `/proc`, `/sys`, `/dev`)

**Best Practice:**
Virtual filesystems (like LittleFS) typically named by filesystem type for:
1. **Debuggability:** Path indicates implementation
2. **Portability:** Code portable across products using same filesystem
3. **Clarity:** Developer immediately knows filesystem type from path

**Product-specific paths** (like `/prism`) used when:
- Path is user-facing (command-line tools, file browsers)
- Branding important for user experience
- Multiple filesystems need product-scoped namespace

#### Research: User Visibility Analysis

**Question:** Is mount path user-visible in PRISM K1?

**WebSocket API Analysis:**

Pattern upload endpoint:
```json
{
  "command": "upload_pattern",
  "name": "rainbow.bin",
  "data": "<base64>"
}
```

Pattern list endpoint:
```json
{
  "command": "list_patterns",
  "response": ["rainbow.bin", "chase.bin", "strobe.bin"]
}
```

**Finding:** WebSocket API uses **pattern names only**, not full paths.

Mount path is **internal implementation detail**, not exposed to users.

**Conclusion:** User never sees mount path. Branding argument (`/prism`) is not relevant.

### Step 3: Impact Analysis

**If we choose `/prism`:**
- ✅ PRD requirement satisfied (product team happy)
- ❌ Deviates from ESP-IDF convention (debugging harder)
- ❌ Overrides technical team's reconciliation decision
- ❌ Less clear what filesystem type is used
- ⚠️ May confuse future developers ("Why is LittleFS mounted at /prism?")

**If we choose `/littlefs`:**
- ✅ Follows ESP-IDF convention (consistent with examples)
- ✅ Clear filesystem type from path (debuggability)
- ✅ Aligns with technical specification reconciliation
- ✅ Standard practice for embedded systems
- ❌ Overrides PRD (product team may object)
- ✅ But: Users never see path (API uses pattern names only)

**Memory/Performance Impact:**
Both paths have same length (7 chars), so:
- No memory difference
- No performance difference
- No technical advantage either way

**Conclusion:**
This is a **naming convention choice**, not a technical performance decision.

---

## Resolution Research

### Research Document Created

**Title:** "Filesystem Mount Path Naming Convention Research"
**Status:** PROPOSED → IN_REVIEW → VALIDATED
**Author:** Agent-Implementation-003
**Date:** 2025-10-17

**Research Question:**
Should LittleFS be mounted at `/prism` (product-branded) or `/littlefs` (filesystem-type) for PRISM K1?

**Methodology:**
1. Survey ESP-IDF examples (5 filesystem examples analyzed)
2. Analyze WebSocket API for path visibility
3. Review Linux/POSIX mounting conventions
4. Interview developers on debugging preferences (3 developers surveyed)

**Key Findings:**

1. **ESP-IDF Convention:** 100% of virtual filesystem examples use filesystem-type naming (`/littlefs`, `/spiffs`)

2. **User Visibility:** Mount path is NOT user-visible (WebSocket API uses pattern names only)

3. **Debugging Clarity:** 3/3 developers surveyed prefer `/littlefs` for debugging clarity

4. **Portability:** Code using `/littlefs` is portable across products using LittleFS

5. **No Technical Impact:** Both paths have identical performance and memory characteristics

**Conclusion:**
`/littlefs` is the superior choice because:
- Follows established ESP-IDF convention
- Improves debuggability (path indicates filesystem type)
- Users never see path (branding argument irrelevant)
- Aligns with PRISM_AUTHORITATIVE_SPECIFICATION.md
- Standard practice for embedded virtual filesystems

**Recommendation:**
Mount LittleFS at `/littlefs`, document rationale in ADR, update PRD to match.

---

### Captain Review of Research

**Reviewer:** Captain SpectraSynq
**Date:** 2025-10-17 14:00:00 UTC
**Decision:** APPROVED → VALIDATED

**Review Comments:**

> **Methodology: EXCELLENT**
> Thorough analysis of ESP-IDF conventions, user visibility, and developer preferences.
> Survey of 3 developers provides qualitative validation.
>
> **Conclusion: WELL-SUPPORTED**
> Evidence clearly supports `/littlefs` choice. User visibility analysis particularly
> compelling - branding irrelevant if users never see path.
>
> **Recommendation: APPROVED**
> This research is validated and can be cited in ADR.
>
> **Next Steps:**
> Create conflict resolution ADR:
> 1. Document the conflict (both specifications)
> 2. Present research findings as evidence
> 3. Make clear decision: `/littlefs`
> 4. Supersede PRD requirement with ADR
> 5. Update CANON to make this definitive

---

## Resolution ADR

### ADR-007: Filesystem Mount Path Specification

**Status:** APPROVED
**Date:** 2025-10-17
**Category:** conflict-resolution

---

#### Context

**Conflict Identified:**

Two specifications provided different mount paths for LittleFS storage:

1. **PRD Section 5.3 (2025-09-15):** Mount path = `/prism`
2. **PRISM_AUTHORITATIVE_SPECIFICATION.md (2025-10-10):** Mount path = `/littlefs`

This conflict **blocked storage implementation** because code requires single, unambiguous path.

**Why This Decision Is Needed:**

- Storage initialization cannot proceed without definitive path
- All file operations depend on this constant
- WebSocket API endpoints will reference this path internally
- Future code and documentation must use consistent path
- This becomes part of device's API surface

**Constraints:**

- Must follow ESP-IDF VFS conventions
- Must support standard file operations (`fopen`, `fread`, etc.)
- Path must be absolute starting with `/`
- Path length <32 bytes (VFS limitation)

---

#### Research Evidence

**[VALIDATED] Research: Filesystem Mount Path Naming Convention**
- Location: `.taskmaster/research/[VALIDATED]/fs-mount-path-naming.md`
- Validated by: Captain SpectraSynq on 2025-10-17

**Key Evidence:**

1. **ESP-IDF Convention (100% consistency):**
   - All virtual filesystem examples use filesystem-type naming
   - `/littlefs` for LittleFS, `/spiffs` for SPIFFS
   - Pattern consistent across all ESP-IDF v5.x examples

2. **User Visibility (API analysis):**
   - WebSocket API uses pattern names only, not full paths
   - Users interact via: `upload_pattern(name="rainbow.bin")`
   - Mount path is internal implementation detail
   - **Branding argument for `/prism` is not applicable**

3. **Developer Preference (qualitative survey):**
   - 3/3 developers surveyed prefer `/littlefs` for debugging
   - Rationale: "Path name indicates filesystem type immediately"
   - Improves troubleshooting and code clarity

4. **Technical Impact (performance analysis):**
   - Both paths same length (7 chars): no memory difference
   - No performance impact either way
   - Decision is purely about naming convention

---

#### Decision

**Mount LittleFS filesystem at `/littlefs`.**

```yaml
storage:
  filesystem: littlefs
  mount_path: "/littlefs"
  partition_label: "storage"
  max_files: 10
```

**Rationale:**

1. **Follows ESP-IDF Convention:** Consistent with all ESP-IDF virtual filesystem examples
2. **Improves Debuggability:** Path name indicates filesystem type (LittleFS)
3. **User Visibility:** Mount path is internal (users never see it), so branding irrelevant
4. **Technical Alignment:** Matches PRISM_AUTHORITATIVE_SPECIFICATION.md (post-reconciliation)
5. **Portability:** Code portable to other projects using LittleFS
6. **Standard Practice:** Aligns with embedded Linux and POSIX conventions for virtual filesystems

**PRD Update Required:**
- PRD Section 5.3 will be updated to `/littlefs` with rationale
- Product team notified of change with justification

---

#### Alternatives Considered

**Alternative 1: Use `/prism` (PRD specification)**

**Pros:**
- Satisfies original PRD requirement
- Product-branded path
- Marketing team preference

**Cons:**
- Deviates from ESP-IDF convention (only example doing so)
- Less clear what filesystem type is used (debugging harder)
- Branding advantage irrelevant (users never see mount path)
- Overrides technical team's specification reconciliation
- May confuse future developers

**Why Rejected:**
User visibility analysis shows mount path is internal implementation detail.
Branding argument only valid if users see path, which they don't (WebSocket API
uses pattern names only). Technical considerations (convention, debuggability)
outweigh branding for internal implementation detail.

---

**Alternative 2: Use `/storage` (generic naming)**

**Pros:**
- Neutral name (neither product-branded nor filesystem-type)
- Descriptive of purpose

**Cons:**
- Does not follow ESP-IDF convention
- Less clear what filesystem implementation is used
- Creates third specification (adds to conflict, not resolves)
- No clear advantage over `/littlefs`

**Why Rejected:**
Introduces third option without solving underlying question: "Follow ESP-IDF
convention or deviate?" Research shows ESP-IDF convention has clear benefits
(debuggability, consistency). Creating neutral name gains nothing.

---

**Alternative 3: Make mount path configurable**

**Pros:**
- Allows different products to choose preferred path
- Flexibility for future use cases

**Cons:**
- Unnecessary complexity (no validated need for configurability)
- Increases testing matrix (must test all path variants)
- Complicates documentation ("mount path is configurable")
- YAGNI: no evidence we'll ever need different path

**Why Rejected:**
Premature optimization. No validated use case for configurable mount path.
Single definitive path provides clarity and reduces complexity. Can revisit
if future requirement emerges, but not needed now.

---

#### Consequences

**Positive:**

✅ **Conflict Resolved:** Single definitive specification in CANON

✅ **ESP-IDF Alignment:** Follows established framework conventions

✅ **Debuggability:** Path name immediately indicates LittleFS

✅ **Consistency:** Matches PRISM_AUTHORITATIVE_SPECIFICATION.md

✅ **Standard Practice:** Aligns with embedded systems conventions

✅ **Future-Proof:** Convention-following code ages better

**Negative:**

❌ **PRD Override:** Requires updating PRD (documentation work)

❌ **Product Team Explanation:** Need to justify why branding irrelevant

**Neutral:**

⚪ **No Technical Impact:** Both paths perform identically

---

#### Validation Criteria

**Success Criteria:**

✅ **Conflict Eliminated:** Only one specification exists in CANON

✅ **Code Consistency:** All file operations use `/littlefs` consistently

✅ **Documentation Aligned:** PRD, CANON, and code all specify `/littlefs`

✅ **No Ambiguity:** Developers have zero confusion about mount path

**Testing:**

1. Storage initialization mounts at `/littlefs` successfully
2. File operations (`fopen("/littlefs/patterns/test.bin", "r")`) work correctly
3. WebSocket API uses paths correctly (`/littlefs/patterns/<name>`)
4. Pattern upload/download functionality works end-to-end

---

#### Implementation

**Affected Components:**

1. **prism_config.h** (auto-generated from CANON):
   ```c
   #define STORAGE_MOUNT_PATH "/littlefs"  /**< LittleFS VFS mount point (ADR-007) */
   ```

2. **storage_init.c**:
   ```c
   esp_vfs_littlefs_conf_t conf = {
       .base_path = STORAGE_MOUNT_PATH,  // Uses constant from prism_config.h
       .partition_label = "storage",
       .format_if_mount_failed = true,
       .dont_mount = false,
   };
   esp_vfs_littlefs_register(&conf);
   ```

3. **pattern_manager.c**:
   ```c
   // Pattern file paths use STORAGE_MOUNT_PATH
   char filepath[64];
   snprintf(filepath, sizeof(filepath), "%s/patterns/%s",
            STORAGE_MOUNT_PATH, pattern_name);
   FILE *fp = fopen(filepath, "rb");
   ```

4. **PRD Section 5.3** (documentation update):
   ```diff
   - Storage mount path: /prism
   + Storage mount path: /littlefs
   +
   + Rationale: Follows ESP-IDF convention for virtual filesystems.
   + Mount path is internal implementation detail (not user-visible).
   + See ADR-007 for detailed justification.
   ```

**Migration Steps:**

1. Update CANON.md via `generate-canon.sh` (includes ADR-007)
2. Regenerate prism_config.h via `sync-code-to-canon.sh`
3. Implement storage_init.c using `STORAGE_MOUNT_PATH` constant
4. Update PRD Section 5.3 with new path and rationale
5. Notify product team of change with explanation
6. Test all file operations with `/littlefs` path

**No Breaking Changes:**
Storage not yet implemented in production, so this change has zero backward compatibility impact.

---

#### References

**[VALIDATED] Research:** Filesystem Mount Path Naming Convention
- Location: `.taskmaster/research/[VALIDATED]/fs-mount-path-naming.md`
- Validated: 2025-10-17 by Captain SpectraSynq

**[CITATION] ESP-IDF LittleFS Example:**
- URL: https://github.com/espressif/esp-idf/tree/v5.2/examples/storage/littlefs
- File: `littlefs_example_main.c`, line 47
- Shows: `base_path = "/littlefs"`

**[CITATION] ESP-IDF VFS Documentation:**
- URL: https://docs.espressif.com/projects/esp-idf/en/v5.2/esp32s3/api-reference/storage/vfs.html
- Section: "Virtual Filesystem Component"

**Conflicting Specifications (Resolved):**
- ~~PRD Section 5.3 (2025-09-15): `/prism`~~ → Superseded by ADR-007
- PRISM_AUTHORITATIVE_SPECIFICATION.md (2025-10-10): `/littlefs` → Confirmed by ADR-007

---

#### Metadata

**ADR Number:** 007
**Title:** Filesystem Mount Path Specification
**Status:** APPROVED (Immutable)
**Category:** conflict-resolution
**Date:** 2025-10-17
**Author:** Agent-Implementation-003
**Approved By:** Captain SpectraSynq
**Approved Date:** 2025-10-17

**Supersedes:**
- PRD Section 5.3 (Storage mount path specification)

**Superseded By:** None (current)

**Dependencies:** None

**Tags:** filesystem, littlefs, mount-path, conflict-resolution, naming-convention

---

## Resolution Outcome

### Final Authority

**After ADR-007 approval:**

1. **CANON.md updated** (via `generate-canon.sh`)
   - Now specifies: `storage.mount_path: "/littlefs"`
   - Cites: ADR-007 as source

2. **PRD Section 5.3 updated**
   - Changed: `/prism` → `/littlefs`
   - Added: "See ADR-007" reference
   - Rationale documented

3. **Code implemented**
   - `prism_config.h` generated with `STORAGE_MOUNT_PATH "/littlefs"`
   - All file operations use constant
   - Tests passing

4. **Conflict resolved**
   - Only one specification exists: `/littlefs` in CANON
   - All documents reference CANON
   - Zero ambiguity

### Authority Hierarchy (After Resolution)

```
CANON.md (specifies /littlefs, cites ADR-007)
    ↑
ADR-007 (APPROVED, immutable, cites validated research)
    ↑
Research (VALIDATED, shows /littlefs optimal)
    ↑
PRD (updated to match CANON, references ADR-007)
```

**CANON is now the single source of truth.**

---

## Lessons Learned

### What Went Well

✅ **Agent stopped immediately** when conflict discovered (correct procedure)

✅ **Conflict thoroughly documented** with both specifications and context

✅ **Research conducted** before making decision (evidence-based)

✅ **Captain review sought** for research validation

✅ **ADR created** to make decision immutable and auditable

✅ **CANON updated** to establish single source of truth

### Process Validation

This conflict resolution followed the **Knowledge Fortress methodology** exactly:

1. ✅ **Discovery:** Conflict identified during implementation
2. ✅ **Stop:** Agent stopped implementation (did not guess)
3. ✅ **Document:** Both specifications documented with sources
4. ✅ **Research:** Evidence gathered via research document
5. ✅ **Validate:** Captain reviewed and validated research
6. ✅ **Decide:** ADR created with evidence-based decision
7. ✅ **Approve:** Captain approved ADR
8. ✅ **Canonize:** CANON updated from ADR
9. ✅ **Implement:** Code matches CANON
10. ✅ **Update:** Conflicting documents updated to reference CANON

**Result:** Zero ambiguity. Single source of truth. Audit trail complete.

### Key Takeaways

**For Future Conflicts:**

1. **Never implement either specification until resolved** - Agent did this correctly
2. **Document both sides fairly** - Show both rationales, not just preferred one
3. **Research with open mind** - Don't pre-decide, let evidence guide
4. **User visibility matters** - Branding argument invalid for internal paths
5. **Convention compliance** - Following framework conventions improves maintainability
6. **ADR is key** - Conflict resolution ADR provides definitive answer
7. **Update all sources** - PRD, CANON, code must all align post-resolution

**Conflict Resolution Formula:**

```
Conflict Discovered
    → STOP implementation
    → Document both specifications
    → Conduct validation research
    → Get Captain approval on research
    → Create conflict resolution ADR
    → Get Captain approval on ADR
    → Update CANON from ADR
    → Update conflicting documents
    → Implement from CANON
    → Validate all sources aligned
```

---

## Summary

**Conflict:** PRD specified `/prism`, PRISM_AUTHORITATIVE_SPECIFICATION.md specified `/littlefs`

**Resolution:** ADR-007 chose `/littlefs` based on validated research

**Evidence:** ESP-IDF convention, user visibility analysis, developer survey

**Outcome:** CANON now specifies `/littlefs` definitively, all documents aligned

**Time to Resolution:** 6 hours (conflict discovery → ADR approval → implementation)

**Status:** ✅ RESOLVED - Zero ambiguity, single source of truth established

---

**End of Conflict Resolution Example**

**Document Purpose:** Training material demonstrating conflict resolution process in Knowledge Fortress system.
