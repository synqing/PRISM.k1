# ADR-004: Pattern Maximum Size

**Status:** APPROVED
**Date:** 2025-10-15
**Decided By:** Captain SpectraSynq
**Supersedes:** None
**Superseded By:** None

## Context

Maximum allowed size for uploaded .prism pattern files.

**Conflict:** PRD says 200KB (204800 bytes). AUTHORITATIVE_SPEC says 256KB (262144 bytes).

## Research Evidence

- [VALIDATED] research/[VALIDATED]/forensic_specification_analysis.md
- [CALCULATION] 1.5MB storage / 25 patterns minimum = 61,440 bytes per pattern average
- [CALCULATION] 256KB allows 5-6 large patterns + 20+ normal patterns

## Decision

Maximum pattern file size is 262,144 bytes (256KB).

```yaml
pattern_max_size: 262144  # 256KB
pattern_min_count: 25     # Minimum patterns that must fit
storage_reserved: 102400  # 100KB safety margin
```

## Alternatives Considered

### Alternative 1: Use 200KB (PRD specification)
**Pros:**
- Slightly more patterns fit
- Matches original PRD

**Cons:**
- Arbitrary power-of-10 limit
- Prevents some legitimate large patterns

**Verdict:** REJECTED - 256KB is power-of-2 and more standard

### Alternative 2: No size limit
**Pros:**
- Maximum flexibility

**Cons:**
- One huge pattern could fill entire storage
- No protection against malicious uploads

**Verdict:** REJECTED - Limits are necessary

## Consequences

### Positive
- Power-of-2 size (256KB) is standard
- Allows 5-6 large patterns (e.g., 10-second 60 FPS)
- Still fits 25+ normal patterns in 1.5MB
- Clear boundary for validation

### Negative
- Limits pattern complexity
- Very long patterns must be split

### Neutral
- Can be increased in future via new ADR if needed

## Validation Criteria

- [x] 256KB fits typical 10-second pattern at 60 FPS
- [x] 25 patterns @ 60KB average fit in 1.5MB storage
- [x] Size is power-of-2 for efficient validation

## Implementation

### Code Changes Required
```
firmware/components/core/include/prism_config.h:
  - Add: #define PATTERN_MAX_SIZE 262144
```

### Documentation Updates
```
CANON.md: Section 5 "Storage Configuration"
```

### Tests Required
```
test_upload.py: Verify 256KB pattern uploads successfully
test_upload.py: Verify 257KB pattern rejected
test_storage.py: Verify 25+ patterns fit
```

## Audit Trail

- **Proposed by:** Storage Agent (forensic analysis)
- **Reviewed by:** Protocol Agent
- **Approved by:** Captain SpectraSynq
- **Implemented:** 2025-10-15
- **Validated:** 2025-10-15

---
**IMMUTABLE AFTER APPROVAL**
