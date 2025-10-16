# Architecture Decision Record (ADR) Writing Guide

**Version:** 1.0.0
**Last Updated:** 2025-10-15
**Status:** ACTIVE

---

## Purpose

This guide teaches you how to write high-quality Architecture Decision Records (ADRs) for the PRISM K1 project. ADRs are immutable documents that record significant architectural decisions with their context, rationale, and consequences.

## What is an ADR?

An **Architecture Decision Record** captures an important architectural decision made along with its context and consequences.

**Key characteristics:**
- **Immutable:** Once approved, cannot be edited (create new ADR to supersede)
- **Evidence-based:** Every decision must cite research or measurements
- **Traceable:** Links to research, specifications, and other ADRs
- **Timestamped:** Creation and approval dates documented
- **Auditable:** Complete history of who decided what and why

**Not an ADR:**
- Implementation details (code-level decisions)
- Temporary workarounds
- Personal preferences without evidence
- Decisions that can be easily reversed

---

## When to Create an ADR

### DO create an ADR for:

✅ **Configuration constants** - Buffer sizes, timeouts, limits
✅ **Architectural patterns** - Memory management strategy, data flow
✅ **Technology choices** - Why WebSocket vs REST, why LittleFS vs SPIFFS
✅ **Data formats** - Binary protocol structure, partition layout
✅ **Performance trade-offs** - Reliability vs speed decisions
✅ **Security decisions** - Validation strategies, encryption choices
✅ **Hardware constraints** - LED count, memory allocation

### DON'T create an ADR for:

❌ **Trivial choices** - Variable names, code style
❌ **Reversible decisions** - Easy to change without impact
❌ **Implementation details** - Which function to use
❌ **Personal preferences** - Without evidence or impact
❌ **Obvious decisions** - "Use standard library function X"

---

## ADR Lifecycle

```
┌────────┐
│ DRAFT  │ ← Initial creation
└────┬───┘
     │
     ▼
┌────────┐
│ REVIEW │ ← Captain reviews
└────┬───┘
     │
     ├──► NEEDS_REVISION ──┐
     │                     │
     └──► APPROVED ────────┤
                           ▼
                    ┌──────────┐
                    │ IMMUTABLE│
                    └──────────┘
```

**Status Values:**
- **DRAFT** - Being written, not yet complete
- **PROPOSED** - Complete, ready for review
- **APPROVED** - Accepted by Captain, now immutable
- **SUPERSEDED** - Replaced by newer ADR

---

## ADR Template Structure

### Required Sections

Every ADR MUST include these sections:

1. **Header (YAML frontmatter)**
2. **Title**
3. **Context**
4. **Research Evidence**
5. **Decision**
6. **Alternatives Considered**
7. **Consequences**
8. **Validation Criteria**
9. **Implementation**
10. **Audit Trail**

---

## Section-by-Section Guide

### 1. Header (YAML Frontmatter)

```yaml
# ADR-XXX: [Decision Title]

**Status:** [DRAFT | PROPOSED | APPROVED | SUPERSEDED]
**Date:** YYYY-MM-DD
**Decided By:** Captain SpectraSynq
**Supersedes:** [ADR-XXX] or None
**Superseded By:** [ADR-XXX] or None
```

**Guidelines:**
- Number format: ADR-001, ADR-002, etc. (zero-padded to 3 digits)
- Date format: ISO 8601 (YYYY-MM-DD)
- Supersedes/Superseded By: Track decision evolution

**Example:**
```markdown
# ADR-002: WebSocket Buffer Size

**Status:** APPROVED
**Date:** 2025-10-15
**Decided By:** Captain SpectraSynq
**Supersedes:** None
**Superseded By:** None
```

---

### 2. Context

**Purpose:** Explain WHY a decision is needed.

**Answer these questions:**
- What situation or problem requires a decision?
- What are the constraints?
- What are the requirements?
- What makes this decision important?

**Structure:**
```markdown
## Context

[2-4 paragraphs explaining the situation]

**Constraints:**
- Constraint 1
- Constraint 2

**Requirements:**
- Requirement 1
- Requirement 2

**Conflict:** [If resolving conflicting specs, document them here]
```

**Good example:**
```markdown
## Context

ESP32-S3 has 512KB SRAM with ~200KB available for heap after system overhead.
WebSocket frame buffer size must balance three competing needs:
1. Larger buffers = faster uploads
2. Smaller buffers = less fragmentation
3. Buffer must fit multiple concurrent operations

**Constraints:**
- Maximum heap: ~200KB usable
- WiFi stack: ~50-70KB (variable)
- 2 concurrent connections required
- Must maintain 60 FPS LED updates

**Conflict:** PRD specifies 8192 bytes. Research shows 4096 is optimal.
```

**Bad example:**
```markdown
## Context

We need to pick a buffer size. Different sizes have different trade-offs.
```
*(Too vague, no specific constraints or requirements)*

---

### 3. Research Evidence

**Purpose:** Cite the research that informs this decision.

**Required:**
- Link to validated research documents
- Cite specific measurements
- Reference external authoritative sources

**Format:**
```markdown
## Research Evidence

- [VALIDATED] research/[VALIDATED]/filename.md
- [MEASUREMENT] Direct measurement on hardware
- [CITATION] External documentation
- [PRODUCTION] Production data from deployed devices

**Key Findings:**
- Finding 1 with data
- Finding 2 with data
```

**Good example:**
```markdown
## Research Evidence

- [VALIDATED] research/[VALIDATED]/esp32_constraints_research.md
- [VALIDATED] research/[VALIDATED]/websocket_validation.md
- [MEASUREMENT] 24-hour fragmentation test with various buffer sizes
- [MEASUREMENT] Throughput tests: 1KB, 2KB, 4KB, 8KB, 16KB frames

**Key Finding:** After 24 hours with 8KB buffers, heap fragmented to 78%
despite 165KB free. Only 18KB largest block remaining. 4KB provides 98%
allocation success rate with acceptable throughput.
```

**Bad example:**
```markdown
## Research Evidence

Research shows 4KB is good.
```
*(No citations, no specific data, no traceability)*

---

### 4. Decision

**Purpose:** State the decision clearly and unambiguously.

**Requirements:**
- ONE clear decision (not multiple options)
- Specific values, not ranges
- Machine-readable format (YAML/JSON) if applicable
- No ambiguity

**Format:**
```markdown
## Decision

[Clear statement of the decision]

```yaml
# Machine-readable decision (if applicable)
key: value
setting: value
```

**Rationale:** [1-2 sentences WHY this decision was made]
```

**Good example:**
```markdown
## Decision

Set WebSocket frame buffer to 4096 bytes (4KB).

```yaml
ws_buffer_size: 4096
ws_max_clients: 2
ws_timeout_ms: 5000
```

**Rationale:** 4KB provides 98% allocation success rate with acceptable
throughput, while 8KB drops to 85% success after 12 hours due to
fragmentation.
```

**Bad example:**
```markdown
## Decision

Use a buffer size that works well, probably between 2KB and 8KB depending
on the situation.
```
*(Ambiguous, no specific value, conditional)*

---

### 5. Alternatives Considered

**Purpose:** Document why other approaches were rejected.

**Requirements:**
- At least 2 alternatives
- Honest pros and cons for each
- Clear rejection rationale

**Format:**
```markdown
## Alternatives Considered

### Alternative 1: [Name]
**Pros:**
- Pro 1
- Pro 2

**Cons:**
- Con 1
- Con 2

**Verdict:** REJECTED - [Specific reason]

### Alternative 2: [Name]
[Same structure]
```

**Good example:**
```markdown
## Alternatives Considered

### Alternative 1: Keep 8KB (PRD specification)
**Pros:**
- Higher peak throughput (7.5Mbps vs 6.2Mbps)
- Matches original specification
- Fewer frames for large uploads

**Cons:**
- 85% allocation success rate after 12h
- Severe fragmentation after 24h (78%)
- Memory exhaustion risk in long-running systems

**Verdict:** REJECTED - Reliability over speed. 24-hour test showed
unacceptable fragmentation despite adequate throughput.

### Alternative 2: Use 2KB buffers
**Pros:**
- Very low fragmentation risk
- 100% allocation success
- Smallest memory footprint

**Cons:**
- 40% throughput reduction (only 3.7Mbps)
- More CPU overhead per byte
- May not meet 500KB/s target

**Verdict:** REJECTED - Too slow for pattern uploads. Target is 500KB/s,
this achieves only 463KB/s.
```

**Bad example:**
```markdown
## Alternatives Considered

We could use other sizes but they're not as good.
```
*(No specific alternatives, no pros/cons, no data)*

---

### 6. Consequences

**Purpose:** Document the impacts of this decision.

**Categories:**
- **Positive:** Benefits gained
- **Negative:** Drawbacks accepted
- **Neutral:** Unchangeable facts

**Format:**
```markdown
## Consequences

### Positive
- Benefit 1 with impact
- Benefit 2 with impact

### Negative
- Drawback 1 with mitigation (if any)
- Drawback 2 with mitigation

### Neutral
- Unchangeable fact 1
- Unchangeable fact 2
```

**Good example:**
```markdown
## Consequences

### Positive
- 98% allocation success rate (measured over 24 hours)
- Acceptable throughput: 6.2Mbps = 775KB/s > 500KB/s target
- Heap remains healthy after 24h stress test
- Supports 2 concurrent connections without issues

### Negative
- Not as fast as 8KB buffers (trade-off accepted for reliability)
- Requires PRD deviation (documented in this ADR)
- More frames needed for large transfers (minimal impact)

### Neutral
- Standard size for embedded WebSocket implementations
- Buffer size affects total memory budget (88KB for pools)
```

**Bad example:**
```markdown
## Consequences

This is a good decision with positive consequences.
```
*(No specifics, no negative consequences acknowledged)*

---

### 7. Validation Criteria

**Purpose:** Define how to verify this decision is correct.

**Format:**
```markdown
## Validation Criteria

- [ ] Criterion 1 (how to test)
- [ ] Criterion 2 (how to test)
- [ ] Criterion 3 (how to test)
```

**Good example:**
```markdown
## Validation Criteria

- [x] 24-hour stress test passes with <5% fragmentation
- [x] Upload speed >100KB/s achieved (measured: 775KB/s)
- [x] 2 concurrent connections supported without allocation failures
- [x] No malloc failures under normal load
- [x] Heap monitor shows >20KB largest block after 24h
```

**Bad example:**
```markdown
## Validation Criteria

- [ ] It works
```
*(Not measurable, no specific test)*

---

### 8. Implementation

**Purpose:** Specify what needs to change in code.

**Format:**
```markdown
## Implementation

### Code Changes Required
```
file/path: Change description
```

### Documentation Updates
```
CANON.md: Section updated
specs/path.md: Generated
```

### Tests Required
```
test_name: What it verifies
```
```

**Good example:**
```markdown
## Implementation

### Code Changes Required
```
firmware/sdkconfig.defaults:
  - Change CONFIG_WS_BUFFER_SIZE from 8192 to 4096

firmware/components/network/websocket.c:
  - Update buffer pool allocation to use WS_BUFFER_SIZE constant
```

### Documentation Updates
```
CANON.md: Section 2 "Memory Configuration" auto-updated
specs/protocol/websocket-spec.md: Regenerate from ADR
```

### Tests Required
```
test_websocket_stress.py: 24-hour upload/download cycle
test_concurrent_clients.py: 2 simultaneous connections
test_fragmentation.py: Monitor heap health over time
```
```

---

### 9. Audit Trail

**Purpose:** Track who proposed, reviewed, approved, and implemented.

**Format:**
```markdown
## Audit Trail

- **Proposed by:** [Agent name/role]
- **Reviewed by:** Captain SpectraSynq
- **Approved by:** Captain SpectraSynq
- **Implemented:** YYYY-MM-DD
- **Validated:** YYYY-MM-DD

---
**IMMUTABLE AFTER APPROVAL**
To change this decision, create new ADR referencing this one.
```

---

## Writing Process

### Step 1: Create ADR File

Use the script:
```bash
./scripts/create-adr.sh
```

This will:
- Auto-assign next ADR number
- Copy template
- Pre-populate metadata
- List validated research for citation

### Step 2: Complete All Sections

Work through each section systematically:
1. Context - Why is this decision needed?
2. Research Evidence - What research supports this?
3. Decision - What exactly are we deciding?
4. Alternatives - What else did we consider?
5. Consequences - What are the impacts?
6. Validation - How do we verify this?
7. Implementation - What changes are needed?
8. Audit Trail - Who was involved?

### Step 3: Self-Review Checklist

Before submitting for Captain review:

- [ ] All sections present and complete
- [ ] Context explains the problem clearly
- [ ] Research evidence cited (no opinions without data)
- [ ] Decision is unambiguous (specific values)
- [ ] At least 2 alternatives documented
- [ ] Pros/cons honest for each alternative
- [ ] Consequences include negative impacts
- [ ] Validation criteria are measurable
- [ ] Implementation is specific
- [ ] Audit trail complete

### Step 4: Submit for Review

Change status to PROPOSED and notify Captain.

### Step 5: Address Feedback

If Captain requests revisions:
1. Read feedback carefully
2. Update ADR to address concerns
3. Mark changes clearly
4. Resubmit for review

### Step 6: Implementation

Once APPROVED:
1. Status becomes APPROVED
2. ADR is now IMMUTABLE
3. Run `./scripts/generate-canon.sh`
4. Implement changes
5. Mark validation criteria as complete

---

## Common Mistakes

### Mistake 1: Vague Decision

**Bad:**
```markdown
## Decision
Use a good buffer size for WebSocket.
```

**Good:**
```markdown
## Decision
Set WebSocket frame buffer to 4096 bytes (4KB).

```yaml
ws_buffer_size: 4096
```
```

### Mistake 2: Missing Evidence

**Bad:**
```markdown
## Research Evidence
Everyone knows 4KB is best.
```

**Good:**
```markdown
## Research Evidence
- [VALIDATED] research/[VALIDATED]/websocket_validation.md
- [MEASUREMENT] 24-hour stress test: 4KB had 98% success vs 85% for 8KB
```

### Mistake 3: No Alternatives

**Bad:**
```markdown
## Alternatives Considered
None. This is obviously the right choice.
```

**Good:**
```markdown
## Alternatives Considered

### Alternative 1: 8KB buffers
**Pros:** Higher throughput
**Cons:** 85% success rate, fragmentation issues
**Verdict:** REJECTED - Reliability > Speed

### Alternative 2: 2KB buffers
**Pros:** 100% success rate
**Cons:** Only 3.7Mbps throughput
**Verdict:** REJECTED - Too slow
```

### Mistake 4: Hiding Negative Consequences

**Bad:**
```markdown
## Consequences

### Positive
Everything is great!
```

**Good:**
```markdown
## Consequences

### Positive
- 98% allocation success
- 6.2Mbps throughput

### Negative
- Not as fast as 8KB (accepted trade-off)
- Deviates from original PRD
```

### Mistake 5: Unmeasurable Validation

**Bad:**
```markdown
## Validation Criteria
- [ ] Works well
- [ ] Fast enough
```

**Good:**
```markdown
## Validation Criteria
- [x] 24-hour test: <5% fragmentation (measured: 3.2%)
- [x] Throughput: >500KB/s (measured: 775KB/s)
```

---

## ADR Relationships

### Superseding an ADR

When an approved ADR needs changing:

1. **Create new ADR**
2. **Reference original in "Supersedes" field**
3. **Document why change needed**
4. **Get approval**
5. **Update original ADR's "Superseded By" field**

**Example:**
```markdown
# ADR-012: Increased WebSocket Buffer to 8KB

**Status:** APPROVED
**Supersedes:** ADR-002
**Date:** 2025-11-20

## Context

Production data from 6 months shows 4KB buffers (ADR-002) are too slow
for new high-resolution patterns. Need to revisit buffer size decision.

## Research Evidence

- [PRODUCTION] 6 months deployment data: 4KB causes user complaints
- [MEASUREMENT] New memory pool architecture eliminates fragmentation
- [VALIDATED] research/[VALIDATED]/buffer_size_revisited.md
```

### Referencing Other ADRs

**In Context section:**
```markdown
## Context

Per ADR-003, we have 320 LEDs which requires 14.4KB DMA buffer.
This affects our memory budget for WebSocket buffers.
```

**In Evidence section:**
```markdown
## Research Evidence

- [ADR] ADR-001 defines partition table layout
- [ADR] ADR-003 specifies LED count (320)
```

---

## Quality Checklist

Use this before submitting for review:

### Structure
- [ ] All 9 required sections present
- [ ] YAML header complete
- [ ] Footer with IMMUTABLE notice

### Context
- [ ] Problem clearly explained
- [ ] Constraints documented
- [ ] Requirements listed
- [ ] Conflicts identified (if any)

### Evidence
- [ ] Research documents cited
- [ ] Measurements included
- [ ] External sources referenced
- [ ] No unsupported opinions

### Decision
- [ ] Unambiguous statement
- [ ] Specific values (not ranges)
- [ ] Machine-readable format (if applicable)
- [ ] Rationale provided

### Alternatives
- [ ] Minimum 2 alternatives
- [ ] Honest pros/cons for each
- [ ] Clear rejection rationale
- [ ] Evidence supports verdicts

### Consequences
- [ ] Positive impacts listed
- [ ] Negative impacts acknowledged
- [ ] Neutral facts included
- [ ] Realistic assessment

### Validation
- [ ] Measurable criteria
- [ ] Testable requirements
- [ ] Acceptance thresholds defined
- [ ] Checkboxes for tracking

### Implementation
- [ ] Specific file changes
- [ ] Documentation updates listed
- [ ] Tests defined
- [ ] Clear action items

### Audit Trail
- [ ] All roles documented
- [ ] Dates included
- [ ] IMMUTABLE notice present

---

## Examples

See `.taskmaster/examples/` for complete examples:
- `example-adr-with-review.md` - Full ADR with review process
- ADR-001 through ADR-005 - Real approved ADRs

---

## Tools

### Creating New ADR
```bash
./scripts/create-adr.sh
```

### Generating CANON from ADRs
```bash
./scripts/generate-canon.sh
```

### Validating ADR Format
```bash
# Automated in CI/CD
# Checks for required sections
```

---

## References

- **Methodology:** [METHODOLOGY.md](METHODOLOGY.md)
- **Validation Guide:** [VALIDATION_GUIDE.md](VALIDATION_GUIDE.md)
- **ADR Template:** [decisions/000-adr-template.md](decisions/000-adr-template.md)
- **Real Examples:** [decisions/001-partition-table.md](decisions/001-partition-table.md)

---

**Document Status:** ACTIVE
**Last Review:** 2025-10-15
**Next Review:** Quarterly or when ADR process changes
**Owner:** Captain SpectraSynq
