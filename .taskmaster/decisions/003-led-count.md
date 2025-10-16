# ADR-003: LED Count Standardization

**Status:** APPROVED
**Date:** 2025-10-15
**Decided By:** Captain SpectraSynq
**Supersedes:** None
**Superseded By:** None

## Context

K1-Lightwave hardware specification defines LED strip length.

**Conflict:** PRD implies 150 LEDs (line 113). AUTHORITATIVE_SPEC says 320 LEDs. Code has no constant defined yet.

## Research Evidence

- [CITATION] K1-Lightwave Hardware Specification v1.0
- [CITATION] PRISM_AUTHORITATIVE_SPECIFICATION.md Section 7
- [VALIDATION] Hardware BOM confirms WS2812B strip configuration

## Decision

LED count is 320 addressable LEDs.

```yaml
led_count: 320
led_type: WS2812B
led_fps_target: 60
```

## Alternatives Considered

### Alternative 1: Use 150 LEDs (PRD implication)
**Pros:**
- Lower memory usage
- Faster frame rendering

**Cons:**
- Doesn't match actual hardware
- Would leave 170 LEDs dark

**Verdict:** REJECTED - Must match hardware

## Consequences

### Positive
- Matches actual hardware specification
- Full LED strip utilized
- Consistent with AUTHORITATIVE_SPEC

### Negative
- Higher memory usage than 150-LED assumption
- More processing per frame

### Neutral
- Fixed by hardware - not configurable

## Validation Criteria

- [x] Matches K1-Lightwave hardware spec
- [x] Memory budget accommodates 320 LEDs
- [x] Frame rate target (60 FPS) achievable

## Implementation

### Code Changes Required
```
firmware/components/core/include/prism_config.h:
  - Add: #define LED_COUNT 320
  - Add: #define LED_TYPE WS2812B
```

### Documentation Updates
```
CANON.md: Section 4 "LED Configuration"
```

### Tests Required
```
test_led_output.py: Verify all 320 LEDs addressable
test_frame_rate.py: Confirm 60 FPS with 320 LEDs
```

## Audit Trail

- **Proposed by:** Hardware Integration Agent
- **Reviewed by:** Firmware Agent
- **Approved by:** Captain SpectraSynq
- **Implemented:** 2025-10-15
- **Validated:** 2025-10-15

---
**IMMUTABLE AFTER APPROVAL**
