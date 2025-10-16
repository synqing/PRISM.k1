# ADR-005: Storage Mount Path

**Status:** APPROVED
**Date:** 2025-10-15
**Decided By:** Captain SpectraSynq
**Supersedes:** None
**Superseded By:** None

## Context

LittleFS filesystem mount point in VFS.

**Conflict:** PRD says `/littlefs` (line 115). Some newer docs say `/prism`. Code not yet implemented.

## Research Evidence

- [CITATION] prism-firmware-prd.txt line 115
- [STANDARD] ESP-IDF LittleFS convention: `/littlefs`

## Decision

Mount LittleFS at `/littlefs` path.

```yaml
storage_mount_path: "/littlefs"
storage_type: "littlefs"
storage_label: "littlefs"
```

## Alternatives Considered

### Alternative 1: Use `/prism` path
**Pros:**
- More specific to project

**Cons:**
- Deviates from PRD
- Non-standard for ESP-IDF
- No technical benefit

**Verdict:** REJECTED - Follow PRD and convention

## Consequences

### Positive
- Matches PRD specification
- Standard ESP-IDF convention
- Clear purpose (filesystem root)

### Negative
- None identified

### Neutral
- Path is configurable if needed later

## Validation Criteria

- [x] Matches PRD line 115
- [x] Standard ESP-IDF convention
- [x] No conflicts with other paths

## Implementation

### Code Changes Required
```
firmware/components/storage/storage_init.c:
  - Set mount point: "/littlefs"
```

### Documentation Updates
```
CANON.md: Section 5 "Storage Configuration"
```

### Tests Required
```
test_filesystem.py: Verify mount at /littlefs
test_filesystem.py: Verify read/write operations
```

## Audit Trail

- **Proposed by:** Storage Agent
- **Reviewed by:** Filesystem Agent
- **Approved by:** Captain SpectraSynq
- **Implemented:** 2025-10-15
- **Validated:** 2025-10-15

---
**IMMUTABLE AFTER APPROVAL**
