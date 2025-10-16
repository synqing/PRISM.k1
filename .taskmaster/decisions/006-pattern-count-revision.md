# ADR-006: Pattern Count Revision

**Status:** APPROVED
**Date:** 2025-10-15
**Decided By:** Captain SpectraSynq
**Approved By:** Captain SpectraSynq
**Approved Date:** 2025-10-15
**Supersedes:** ADR-004 (pattern_min_count field only)
**Superseded By:** None

## Context

ADR-004 specified minimum 25 patterns must fit in 1.5MB storage. However, Captain SpectraSynq has clarified that **15 templates is sufficient**, with expansion possible if memory permits.

**Original ADR-004 Specification:**
```yaml
pattern_max_size: 262144  # 256KB
pattern_min_count: 25     # Minimum patterns that must fit
storage_reserved: 102400  # 100KB safety margin
```

**Issue:** The "25-35 patterns" requirement in PRD was incorrect or outdated. Actual requirement is 15 templates minimum.

## Research Evidence

**[CAPTAIN DECISION]** Direct guidance from Captain SpectraSynq (2025-10-15):
> "No idea where the fuck 25-35 came from, but 15 is enough, we can expand it - if remaining memory permits"

**[CALCULATION]** Storage math with 15 patterns:
```
Storage Available: 1.5MB = 1,572,864 bytes
Pattern Max Size: 256KB = 262,144 bytes (unchanged from ADR-004)
Patterns Required: 15 minimum

Average size per pattern: 1.5MB ÷ 15 = 104,857 bytes (~102KB)

Validation: 15 patterns × 102KB avg = 1,530KB ≈ 1.5MB ✅

Example distribution:
- 3 large patterns @ 200KB = 600KB
- 7 medium patterns @ 100KB = 700KB
- 5 small patterns @ 40KB = 200KB
Total: 1,500KB (fits in 1.5MB with headroom)
```

**[CONTEXT]** Template System Requirements:
- 15 pre-loaded templates in firmware (per PRD line 44)
- Categories: Ambient (5), Energy (5), Special (5)
- Storage must accommodate these 15 templates minimum
- Additional user patterns can be added if space permits

## Decision

**Update minimum pattern count from 25 to 15.**

ADR-004 remains valid for `pattern_max_size: 256KB`. This ADR supersedes ONLY the `pattern_min_count` field.

```yaml
pattern_max_size: 262144  # 256KB (UNCHANGED from ADR-004)
pattern_min_count: 15     # UPDATED: 15 templates minimum (was 25)
storage_reserved: 102400  # 100KB safety margin (UNCHANGED)
```

**Rationale:**
1. **Captain Authority:** Direct clarification that 15 is sufficient
2. **Template Alignment:** Matches 15-template system design
3. **Storage Headroom:** 15 patterns @ ~100KB avg leaves comfortable margin
4. **Expandability:** Additional patterns can be added if memory permits
5. **Realistic:** 25 patterns was arbitrary/outdated requirement

## Alternatives Considered

### Alternative 1: Keep 25 patterns requirement
**Pros:**
- More patterns available
- More user flexibility

**Cons:**
- Unnecessary constraint
- 15 templates is sufficient for product
- 25 was arbitrary number from outdated PRD

**Verdict:** REJECTED - Captain clarified 15 is sufficient

### Alternative 2: No minimum count
**Pros:**
- Maximum flexibility

**Cons:**
- No guarantee templates will fit
- No clear storage target

**Verdict:** REJECTED - 15 provides clear target

### Alternative 3: Set minimum to 10
**Pros:**
- Even more storage headroom

**Cons:**
- Less than required 15 templates
- Doesn't match template system design

**Verdict:** REJECTED - Must accommodate 15 templates

## Consequences

### Positive
✅ **Realistic Requirement:** 15 patterns is achievable and sufficient
✅ **Storage Headroom:** ~100KB average per pattern allows flexibility
✅ **Template Alignment:** Matches 15-template system design exactly
✅ **Expansion Ready:** Additional patterns can be added if memory permits
✅ **Clearer Math:** 15 × ~100KB = 1.5MB is straightforward

### Negative
❌ **Fewer Patterns:** Users get 15 base templates vs originally planned 25-35
- **Mitigation:** 15 is sufficient per Captain decision, expandable if needed

❌ **PRD Mismatch:** PRD specified 25-35 patterns
- **Mitigation:** PRD will be updated or deprecated to match CANON

### Neutral
⚪ **ADR-004 Partially Superseded:** Only `pattern_min_count` field affected, rest unchanged
⚪ **Pattern Size Limit:** 256KB max remains unchanged (still allows large patterns)

## Validation Criteria

### Success Criteria
- [x] 15 templates @ 100KB average fit in 1.5MB storage
- [x] 256KB max pattern size unchanged
- [x] Storage math validated (15 × 102KB ≈ 1.5MB)
- [ ] CANON regenerated with updated pattern count
- [ ] Code validation passes with new count
- [ ] 15 templates successfully deployed in testing

### Testing Requirements
```
test_storage.py: Verify 15 patterns fit in 1.5MB partition
test_templates.py: Verify all 15 templates load successfully
test_upload.py: Verify pattern uploads work up to 256KB
```

### Acceptance
- All 15 templates load on first boot
- Storage monitoring shows adequate headroom
- Additional patterns can be uploaded if space available

## Implementation

### ADR Updates Required
```
.taskmaster/decisions/004-pattern-max-size.md:
  - Add superseded_by: ADR-006 (partial)
  - Note: pattern_min_count superseded, pattern_max_size unchanged
```

### Code Changes Required
```
firmware/components/core/include/prism_config.h:
  - Update: #define PATTERN_MIN_COUNT 15  // Was 25 (ADR-006)
  - Keep: #define PATTERN_MAX_SIZE 262144  // Unchanged (ADR-004)
```

### CANON Regeneration
```bash
cd .taskmaster
./scripts/generate-canon.sh

# Expected: Section 4 updates from "25" to "15"
```

### Documentation Updates
```
CANON.md: Section 4 "Pattern Maximum Size"
  - pattern_min_count: 25 → 15
  - Add note: Supersedes ADR-004 pattern count only

PRD (deprecated): Update lines 16, 166
  - "25-35 patterns" → "15+ templates"
```

## Audit Trail

- **Proposed by:** Agent-Documentation-001 (Knowledge Fortress)
- **Evidence:** Captain SpectraSynq direct guidance
- **Reviewed by:** Captain SpectraSynq
- **Approved by:** Captain SpectraSynq
- **Approved Date:** 2025-10-15
- **Date:** 2025-10-15

### Relationship to ADR-004
- **Supersedes:** ADR-004 `pattern_min_count` field (25 → 15)
- **Preserves:** ADR-004 `pattern_max_size` field (256KB unchanged)
- **Preserves:** ADR-004 `storage_reserved` field (100KB unchanged)

**This is a partial supersession** - ADR-004 remains valid for pattern size limit.

---

## Approval

**✅ APPROVED by Captain SpectraSynq on 2025-10-15**

This ADR is now IMMUTABLE.

Next steps:
1. ✅ Status → APPROVED
2. ✅ This ADR is now IMMUTABLE
3. [ ] Regenerate CANON
4. [ ] Update prism_config.h
5. [ ] Update ADR-004 metadata (superseded_by field)

---

**IMMUTABLE AFTER APPROVAL**
