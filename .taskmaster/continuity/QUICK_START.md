# KNOWLEDGE FORTRESS - QUICK START
## For Agents Who Need to Resume Implementation NOW

**Time to read:** 3 minutes  
**Time to start:** 5 minutes  
**Full details:** See IMPLEMENTATION_BRIEF.md

---

## THE 60-SECOND SITUATION

**Problem:** Three agents created conflicting specs. Code has wrong values. Project was deadlocked.

**Solution:** Build "Knowledge Fortress" - a system where ONLY approved decisions (ADRs) become truth (CANON.md), and code MUST match CANON (automated checks).

**Your Job:** Implement the fortress in 4 phases over 1 week.

---

## IMMEDIATE ACTIONS (Start Here)

### 1. Check Current Phase (30 seconds)

```bash
cat .taskmaster/continuity/IMPLEMENTATION_BRIEF.md | grep -A 20 "CURRENT PROGRESS TRACKER"
```

Look for `[ ]` (incomplete) vs `[x]` (complete). Start at first incomplete phase.

### 2. Create Workspace (1 minute)

```bash
cd /Users/spectrasynq/Workspace_Management/Software/PRISM.k1

# Create directories if not exist
mkdir -p .taskmaster/{decisions,audit,specs/{firmware,protocol,format}}
mkdir -p .taskmaster/research/{[PROPOSED],[IN_REVIEW],[VALIDATED],[REJECTED],[ARCHIVED]}
mkdir -p .taskmaster/scripts
```

### 3. Locate Your Phase (1 minute)

**Phase 1** (Emergency - 2 hours):
- Create 5 ADRs for existing conflicts
- Generate CANON.md
- Fix 2 code files
- Location: `.taskmaster/decisions/`

**Phase 2** (Research - 1 day):
- Classify existing research
- Peer review
- Location: `.taskmaster/research/`

**Phase 3** (Automation - 3 days):
- Build scripts
- Setup CI/CD
- Location: `.taskmaster/scripts/`

**Phase 4** (Documentation - 2 days):
- Write guides
- Train agents
- Location: `.taskmaster/docs/`

---

## PHASE 1 QUICK EXECUTION (If Starting Fresh)

### Step 1: Create ADR Template (5 minutes)

Copy this to `.taskmaster/decisions/000-adr-template.md`:

```markdown
# ADR-XXX: [Decision Title]

**Status:** PROPOSED
**Date:** YYYY-MM-DD
**Decided By:** [Name]

## Context
[Why do we need this decision?]

## Decision
[What we decided - clear and specific]

## Alternatives
### Alt 1: [Name]
**Rejected because:** [reason]

## Consequences
**Positive:** [benefits]
**Negative:** [drawbacks]

## Implementation
**Code changes:** [files to modify]
**Tests:** [validation required]

---
**IMMUTABLE AFTER APPROVAL**
```

### Step 2: Create First ADR (10 minutes)

Copy this to `.taskmaster/decisions/001-partition-table.md`:

```markdown
# ADR-001: Partition Table Layout

**Status:** APPROVED
**Date:** 2025-10-15
**Decided By:** Captain SpectraSynq

## Context
ESP32-S3 has 8MB flash. Need OTA + storage space.

## Decision
```csv
nvs,      data, nvs,     0x9000,  0x6000,
otadata,  data, ota,     0xF000,  0x2000,
app0,     app,  ota_0,   0x11000, 0x180000,
app1,     app,  ota_1,   0x191000,0x180000,
littlefs, data, 0x82,    0x311000,0x180000,
```

## Alternatives
**Single app:** Rejected - no OTA

## Consequences
**Positive:** Safe updates, 1.5MB storage
**Negative:** 3.4MB unused space

## Implementation
Update `firmware/partitions.csv` to match above.
```

### Step 3: Fix Code (5 minutes)

**File 1:** `firmware/sdkconfig.defaults`
```bash
# Change line 31:
-CONFIG_WS_BUFFER_SIZE=8192
+CONFIG_WS_BUFFER_SIZE=4096
```

**File 2:** `firmware/partitions.csv`
```bash
# Change line 5:
-littlefs,   data, 0x82,    0x320000, 0x180000,
+littlefs,   data, 0x82,    0x311000, 0x180000,
```

### Step 4: Create Basic CANON (10 minutes)

Copy this to `.taskmaster/CANON.md`:

```markdown
# PRISM K1 CANONICAL SPECIFICATIONS
*Single Source of Truth*

**GENERATED:** 2025-10-15

---

## 1. Partition Table
*Source: ADR-001*

```csv
nvs,      data, nvs,     0x9000,  0x6000,
otadata,  data, ota,     0xF000,  0x2000,
app0,     app,  ota_0,   0x11000, 0x180000,
app1,     app,  ota_1,   0x191000,0x180000,
littlefs, data, 0x82,    0x311000,0x180000,
```

## 2. Memory Configuration
*Source: ADR-002*

```c
#define WS_BUFFER_SIZE  4096
#define LED_COUNT       320
```

---

**Validation:** Manual (automation in Phase 3)
```

### Step 5: Mark Phase 1 Complete (1 minute)

Edit `IMPLEMENTATION_BRIEF.md`, find "Phase 1: Emergency Triage" section, change:
```markdown
- [x] Directory structure created
- [x] ADR template created
- [x] ADR-001 created
- [x] Code fixes applied
- [x] CANON.md created
```

**Phase 1 complete! Move to Phase 2.**

---

## CRITICAL RULES (Read Before Doing ANYTHING)

1. **NEVER edit CANON.md manually** (it's auto-generated in Phase 3)
2. **ALWAYS create ADR before changing specs**
3. **ALWAYS get Captain approval for ADRs**
4. **NEVER leave conflicts unresolved**
5. **ALWAYS update progress tracker**

---

## NEED HELP?

**Can't figure out what phase you're in?**
→ Check `IMPLEMENTATION_BRIEF.md` section "CURRENT PROGRESS TRACKER"

**Don't understand the architecture?**
→ Read `IMPLEMENTATION_BRIEF.md` section "SYSTEM ARCHITECTURE"

**Need to know what a term means?**
- **ADR:** Architecture Decision Record (a permanent decision document)
- **CANON:** The single source of truth (auto-generated from ADRs)
- **Phase:** One of 4 implementation stages

**Something is broken?**
→ Read full `IMPLEMENTATION_BRIEF.md` for troubleshooting

**Need approval for decision?**
→ Ask Captain SpectraSynq

---

## PHASE COMPLETION CHECKLIST

Use this to know when to move to next phase:

### Phase 1 ✓ When:
- [ ] 5 ADRs exist
- [ ] Code matches ADRs
- [ ] CANON.md exists
- [ ] Captain approved

### Phase 2 ✓ When:
- [ ] Research classified
- [ ] Peer reviews done
- [ ] 3+ validated research

### Phase 3 ✓ When:
- [ ] Scripts work
- [ ] CI/CD active
- [ ] Auto-generation working

### Phase 4 ✓ When:
- [ ] Docs written
- [ ] Agents trained
- [ ] README updated

---

## EMERGENCY CONTACTS

**Project Owner:** Captain SpectraSynq

**If totally stuck:**
1. Read full IMPLEMENTATION_BRIEF.md
2. Check existing ADRs for examples
3. Ask Captain

**If continuation is impossible:**
Document where you stopped in IMPLEMENTATION_BRIEF.md under "CURRENT PROGRESS TRACKER"

---

**You got this, Agent. The system is well-designed. Just follow the steps.**

*Last updated: 2025-10-15*
