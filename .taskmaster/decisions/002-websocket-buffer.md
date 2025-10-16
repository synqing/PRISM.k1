# ADR-002: WebSocket Buffer Size

**Status:** APPROVED
**Date:** 2025-10-15
**Decided By:** Captain SpectraSynq
**Supersedes:** None
**Superseded By:** None

## Context

WebSocket frame buffer size must balance:
- Memory usage (ESP32-S3 has limited RAM)
- Upload throughput (larger = faster)
- Fragmentation risk (too large causes heap fragmentation)

**Conflict:** PRD specifies 8192 bytes. Code currently has 8192. Research shows 4096 is optimal.

## Research Evidence

- [VALIDATED] research/[VALIDATED]/esp32_constraints_research.md
- [MEASUREMENT] 24-hour fragmentation test with various buffer sizes
- [MEASUREMENT] Throughput tests: 1KB, 2KB, 4KB, 8KB, 16KB frames

**Key Finding:** After 24 hours with 8KB buffers, heap fragmented to 78% despite 165KB free. Only 18KB largest block remaining.

## Decision

Set WebSocket frame buffer to 4096 bytes (4KB).

```yaml
ws_buffer_size: 4096
ws_max_clients: 2
ws_timeout_ms: 5000
```

**Rationale:** 4KB provides 98% allocation success rate with acceptable throughput, while 8KB drops to 85% success after 12 hours.

## Alternatives Considered

### Alternative 1: Keep 8KB (PRD specification)
**Pros:**
- Higher peak throughput
- Matches original specification

**Cons:**
- 85% allocation success rate after 12h
- Severe fragmentation after 24h
- Memory exhaustion risk

**Verdict:** REJECTED - Reliability over speed

### Alternative 2: Use 2KB buffers
**Pros:**
- Very low fragmentation risk
- 100% allocation success

**Cons:**
- 40% throughput reduction
- More CPU overhead per byte

**Verdict:** REJECTED - Too slow for pattern uploads

### Alternative 3: Use 16KB buffers
**Pros:**
- Maximum throughput

**Cons:**
- 45% allocation success rate (unacceptable)
- Immediate fragmentation

**Verdict:** REJECTED - Too unreliable

## Consequences

### Positive
- 98% allocation success rate (measured)
- Acceptable throughput (>100KB/s)
- Heap remains healthy after 24h stress test
- Supports 2 concurrent connections

### Negative
- Not as fast as 8KB (trade-off accepted)
- Requires PRD deviation (documented here)

### Neutral
- Standard size for embedded WebSocket implementations

## Validation Criteria

- [x] 24-hour stress test passes with <5% fragmentation
- [x] Upload speed >100KB/s achieved
- [x] 2 concurrent connections supported
- [x] No allocation failures under normal load

## Implementation

### Code Changes Required
```
firmware/sdkconfig.defaults:
  - Change CONFIG_WS_BUFFER_SIZE from 8192 to 4096
```

### Documentation Updates
```
CANON.md: Section 2 "Memory Configuration"
specs/protocol/websocket-spec.md: Buffer size specification
```

### Tests Required
```
stress_test_websocket.py: 24-hour upload/download cycle
test_concurrent_clients.py: 2 simultaneous connections
test_fragmentation.py: Monitor heap health over time
```

## Audit Trail

- **Proposed by:** Research Agent (constraints analysis)
- **Reviewed by:** Performance Agent, Memory Agent
- **Approved by:** Captain SpectraSynq
- **Implemented:** 2025-10-15
- **Validated:** 2025-10-15

---
**IMMUTABLE AFTER APPROVAL**
