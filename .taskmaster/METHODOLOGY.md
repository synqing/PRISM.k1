# PRISM K1 Knowledge Fortress Methodology

**Version:** 1.0.0
**Last Updated:** 2025-10-15
**Status:** ACTIVE

---

## Purpose

This document defines the research-first methodology for the PRISM K1 Knowledge Fortress system - a military-grade knowledge management framework preventing specification drift and ensuring evidence-based decision-making.

## Core Principles

### 1. Research Before Decisions
**Rule:** Never make technical decisions without validated research.

**Process:**
1. Question arises ("What buffer size should we use?")
2. Research conducted with methodology documented
3. Research reviewed by Captain
4. Research moved to [VALIDATED]
5. ADR created from validated research
6. Decision implemented

**Why:** Prevents opinion-based decisions, ensures reproducibility.

### 2. Single Source of Truth (CANON.md)
**Rule:** CANON.md is THE authoritative specification. Code MUST match CANON.

**Process:**
1. ADRs created/approved
2. CANON.md auto-generated from ADRs
3. Code synchronized to CANON
4. Validation enforced via CI/CD

**Why:** Eliminates conflicting specifications, provides unambiguous reference.

### 3. Immutable Decision Records
**Rule:** Approved ADRs cannot be edited. Create new ADR to supersede.

**Process:**
1. ADR drafted
2. ADR reviewed
3. ADR approved (status: APPROVED)
4. ADR becomes immutable
5. Changes require new ADR referencing original

**Why:** Maintains audit trail, prevents revisionist history.

### 4. Captain Review Required
**Rule:** All research must pass Captain (user) review before validation.

**Reviewer:** Captain SpectraSynq (project owner)
- Reviews methodology for reproducibility
- Questions assumptions and challenges conclusions
- Checks for conflicts with existing research
- Provides APPROVE / NEEDS_REVISION / REJECT decision

**Why:** Ensures quality control and maintains project coherence with single authority.

### 5. Evidence-Based Only
**Rule:** Every decision must cite evidence (measurement, citation, or validated research).

**Accepted Evidence Types:**
- `[MEASUREMENT]` - Direct hardware measurement
- `[VALIDATED]` - Peer-reviewed research document
- `[CITATION]` - External authoritative source (ESP-IDF docs, datasheets)
- `[PRODUCTION]` - Production data from deployed devices

**Rejected:**
- Opinions without data
- "Best practices" without source
- Assumptions without validation

**Why:** Ensures decisions are defensible and reproducible.

---

## Research Lifecycle

### State Diagram

```
┌────────────┐
│  PROPOSED  │ ← Research created, awaiting review
└─────┬──────┘
      │
      ▼
┌────────────┐
│ IN_REVIEW  │ ← Captain review in progress
└─────┬──────┘
      │
      ├─────► [Captain approves] ──┐
      │                             │
      └─────► [Captain rejects] ────┤
                              ▼
              ┌───────────────────────┐
              │  VALIDATED / REJECTED │
              └───────────────────────┘
```

### State Transitions

#### 1. PROPOSED → IN_REVIEW
**Trigger:** Submit for Captain review
**Requirements:**
- Research document follows template
- Methodology section complete
- Question clearly stated
- Raw data provided (if applicable)

**Location:** `.taskmaster/research/[PROPOSED]/`

#### 2. IN_REVIEW → VALIDATED
**Trigger:** Captain approves
**Requirements:**
- Methodology reproducible
- Assumptions stated
- Limitations acknowledged
- No conflicts with existing research
- Captain approval documented

**Location:** `.taskmaster/research/[VALIDATED]/`

#### 3. IN_REVIEW → REJECTED
**Trigger:** Captain rejects
**Requirements:**
- Rejection reasons documented
- Author notified
- Research archived with rationale
- Captain feedback documented

**Location:** `.taskmaster/research/[REJECTED]/`

#### 4. VALIDATED → ADR
**Trigger:** Decision needed based on research
**Process:**
1. Run `./scripts/create-adr.sh`
2. Select validated research as source
3. Complete ADR template
4. Get Captain approval
5. Update status to APPROVED
6. Run `./scripts/generate-canon.sh`

---

## Decision-Making Process

### Step-by-Step Flow

#### Phase 1: Question Identification
```
Input: Technical question or conflict
Output: Research assignment

Example: "What WebSocket buffer size should we use?"

Actions:
1. Document the question precisely
2. Identify constraints (memory, performance, reliability)
3. Define success criteria
4. Assign research agent
```

#### Phase 2: Research Execution
```
Input: Research assignment
Output: Research document in [PROPOSED]

Actions:
1. Create research file: research/[PROPOSED]/topic_research.md
2. Add YAML frontmatter with metadata
3. Document methodology
4. Execute tests/measurements
5. Collect and document data
6. Draw conclusions
7. Submit for review
```

**Research Template:**
```yaml
---
title: Research Topic
status: PROPOSED
author: Agent Name
date: YYYY-MM-DD
category: [MEASUREMENT | ANALYTICAL | SECURITY | PRODUCTION]
question: What specific question does this answer?
methodology: |
  Detailed methodology here
  Tools used, test procedures, duration, etc.
impact: [LOW | MEDIUM | HIGH | CRITICAL]
---

# Research Content

## Executive Summary
[1-2 paragraphs: question, approach, findings]

## Methodology
[Detailed enough for reproduction]

## Data
[Raw data, measurements, observations]

## Analysis
[Interpretation of data]

## Conclusions
[Direct answers to the question]

## Limitations
[What this research does NOT cover]

## Recommendations
[Actionable next steps]
```

#### Phase 3: Captain Review
```
Input: Research in [PROPOSED]
Output: Research in [VALIDATED] or [REJECTED]

Reviewer: Captain SpectraSynq (project owner)

Process:
1. Captain reviews research document
2. Captain checks:
   - Methodology is reproducible
   - Assumptions are clearly stated
   - Limitations acknowledged
   - No conflicts with existing research
   - Conclusions supported by data
3. Captain provides decision: APPROVE / NEEDS_REVISION / REJECT
4. If NEEDS_REVISION: Author updates, restart review
5. If APPROVE: Move to [VALIDATED]
6. If REJECT: Move to [REJECTED] with reasons
```

**Review Checklist:** See VALIDATION_GUIDE.md

#### Phase 4: ADR Creation
```
Input: Validated research + decision needed
Output: ADR in decisions/ folder

Process:
1. Run: ./scripts/create-adr.sh
2. Enter decision title
3. Select validated research as source
4. Complete all ADR sections:
   - Context (why decision needed)
   - Research Evidence (cite validated research)
   - Decision (clear, unambiguous)
   - Alternatives Considered (with pros/cons)
   - Consequences (positive/negative/neutral)
   - Validation Criteria
   - Implementation details
5. Get Captain approval
6. Update status to APPROVED
7. ADR becomes immutable
```

**ADR Template:** See ADR_GUIDE.md

#### Phase 5: CANON Generation
```
Input: Approved ADR(s)
Output: Updated CANON.md

Process:
1. Run: ./scripts/generate-canon.sh
2. Script extracts all APPROVED ADRs
3. Script generates CANON.md with:
   - Table of contents
   - Decision specifications
   - Machine-readable YAML/JSON
   - Change history
   - Document signature
4. CANON.md becomes new source of truth
```

#### Phase 6: Code Synchronization
```
Input: Updated CANON.md
Output: Code matching CANON

Process:
1. Run: ./scripts/sync-code-to-canon.sh
2. Script generates prism_config.h from CANON
3. Script updates sdkconfig.defaults if needed
4. Run: ./scripts/validate-canon.sh
5. Fix any validation failures
6. Commit with message: "sync: Update code from CANON"
```

---

## Validation & Enforcement

### Pre-Commit Validation
**Enforced by:** `.githooks/pre-commit`

**Blocks:**
- Manual edits to CANON.md
- Commits without [AUTO] marker if CANON changed

**Reminds:**
- To regenerate CANON after ADR changes

### Pre-Push Validation
**Enforced by:** `.githooks/pre-push`

**Blocks:**
- Pushing code that doesn't match CANON
- Specification drift

**Requires:**
- Full CANON validation passes
- All constants match
- Partition table correct

### CI/CD Validation
**Enforced by:** `.github/workflows/canon-validation.yml`

**Runs on:** Every push/PR to main/develop

**Validates:**
1. CANON freshness (matches ADRs)
2. No manual CANON edits
3. Code constants match CANON
4. ADR format compliance
5. Firmware builds only if validation passes

---

## Conflict Resolution

### When Specifications Conflict

**Process:**
1. **Stop immediately** - Do not implement conflicting spec
2. **Document conflict:**
   ```markdown
   ## Specification Conflict

   **Source A:** PRD line 115 says X
   **Source B:** AUTHORITATIVE_SPEC says Y
   **Code:** Currently implements Z

   **Impact:** [BLOCKING | HIGH | MEDIUM | LOW]
   ```

3. **Research both positions:**
   - Why does each spec say what it says?
   - What evidence supports each?
   - What are the constraints?

4. **Conduct validation research:**
   - Test both approaches if possible
   - Measure actual behavior
   - Document findings

5. **Create conflict resolution ADR:**
   - Context: Document the conflict
   - Evidence: Present research findings
   - Decision: Choose ONE specification
   - Rationale: WHY this spec was chosen
   - Supersedes: Mark conflicting specs as superseded

6. **Update CANON:**
   - Generate new CANON from ADR
   - CANON becomes new truth
   - Old specs marked as superseded

### When Research Conflicts

**Process:**
1. **Identify discrepancy:**
   - Research A says X
   - Research B says Y

2. **Determine cause:**
   - Different methodologies?
   - Different constraints?
   - One supersedes the other?
   - Both valid in different contexts?

3. **Resolution paths:**

   **Path A: Different contexts (both valid)**
   ```
   - Create ADR noting context-dependent nature
   - Example: "Use 4KB for reliability, 8KB for max throughput"
   ```

   **Path B: One supersedes (methodology improved)**
   ```
   - Move older research to [ARCHIVED]
   - Add superseded_by metadata
   - Use newer research
   ```

   **Path C: Conflict needs new research**
   ```
   - Design experiment to resolve conflict
   - Execute validation research
   - Create ADR from validated result
   ```

---

## Quality Standards

### Research Quality Criteria

#### Methodology (MANDATORY)
- [ ] Question clearly stated
- [ ] Methodology reproducible
- [ ] Tools/equipment listed
- [ ] Test duration specified
- [ ] Sample size documented
- [ ] Variables controlled
- [ ] Assumptions stated explicitly

#### Data (MANDATORY)
- [ ] Raw data provided
- [ ] Measurements timestamped
- [ ] Units specified
- [ ] Uncertainties acknowledged
- [ ] Outliers explained

#### Analysis (MANDATORY)
- [ ] Calculations shown
- [ ] Statistical methods appropriate
- [ ] Conclusions supported by data
- [ ] Alternative explanations considered
- [ ] Limitations acknowledged

#### Documentation (MANDATORY)
- [ ] Executive summary (1-2 paragraphs)
- [ ] Methodology section complete
- [ ] Data section with tables/graphs
- [ ] Analysis section with interpretation
- [ ] Conclusions section
- [ ] Limitations section
- [ ] Recommendations section

### ADR Quality Criteria

#### Structure (MANDATORY)
- [ ] All template sections present
- [ ] Context explains why decision needed
- [ ] Decision is clear and unambiguous
- [ ] At least 2 alternatives considered
- [ ] Pros/cons listed for each alternative
- [ ] Consequences documented (positive/negative/neutral)
- [ ] Validation criteria defined
- [ ] Implementation details provided

#### Evidence (MANDATORY)
- [ ] Cites validated research OR
- [ ] Cites measurements with methodology OR
- [ ] Cites authoritative external sources
- [ ] NO opinions without evidence
- [ ] NO "best practices" without source

#### Traceability (MANDATORY)
- [ ] Research sources linked
- [ ] Supersedes/Superseded By specified
- [ ] Audit trail complete
- [ ] Date and author documented

---

## Tools & Automation

### Core Scripts

**generate-canon.sh**
- Scans `.taskmaster/decisions/` for APPROVED ADRs
- Extracts decision sections
- Generates CANON.md with TOC, specs, validation status
- Usage: `./scripts/generate-canon.sh`

**create-adr.sh**
- Interactive wizard for new ADRs
- Auto-numbers ADRs
- Lists validated research for citation
- Pre-populates template
- Usage: `./scripts/create-adr.sh`

**validate-canon.sh**
- 5-step validation: freshness, constants, config, partitions, headers
- Compares code to CANON specifications
- Reports mismatches with fix instructions
- Usage: `./scripts/validate-canon.sh`

**sync-code-to-canon.sh**
- Generates prism_config.h from CANON
- Updates sdkconfig.defaults if needed
- Ensures code matches specifications
- Usage: `./scripts/sync-code-to-canon.sh`

**extract-constants.sh**
- Parses CANON.md for machine-readable YAML
- Outputs JSON with all constants
- Used by validation and generation scripts
- Usage: `./scripts/extract-constants.sh`

### Git Hooks

**pre-commit**
- Prevents manual CANON.md edits
- Requires [AUTO] marker if CANON changes
- Reminds to regenerate after ADR changes

**pre-push**
- Runs full CANON validation
- Blocks push if code doesn't match CANON
- Can bypass with `--no-verify` (NOT recommended)

### CI/CD Pipeline

**.github/workflows/canon-validation.yml**
- Runs on every push/PR
- 4-step validation process
- Only builds firmware if validation passes
- Prevents specification drift in main branch

---

## Best Practices

### DO

✅ **Always cite sources** - Every claim needs evidence
✅ **Document methodology** - Make research reproducible
✅ **Seek Captain review** - Maintain quality and coherence
✅ **Update CANON after ADR** - Keep single source of truth current
✅ **Validate before push** - Use pre-push hook
✅ **Ask when uncertain** - Escalate to Captain if unclear

### DON'T

❌ **Never edit CANON manually** - Always regenerate from ADRs
❌ **Never skip Captain review** - Quality depends on validation
❌ **Never implement conflicting specs** - Resolve conflicts first
❌ **Never make decisions without evidence** - Research first
❌ **Never bypass validation** - Hooks exist for a reason
❌ **Never assume specifications** - Check CANON

---

## Success Metrics

System is working when:

1. **Zero Ambiguity:** Any question has ONE clear answer in CANON
2. **Zero Drift:** Code always matches CANON (automated enforcement)
3. **Zero Conflicts:** Impossible to have competing specs
4. **Full Traceability:** Every constant traces to specific ADR
5. **Immutable History:** Every decision timestamped and auditable

---

## Troubleshooting

### "I found a specification conflict"
1. Stop immediately
2. Document both specifications
3. Follow conflict resolution process above
4. Create conflict resolution ADR
5. Update CANON

### "Research was rejected"
1. Read reviewer feedback carefully
2. Identify methodology gaps
3. Address concerns
4. Resubmit for review
5. If still rejected, escalate to Captain

### "CANON validation failing"
1. Run `./scripts/validate-canon.sh` to see specifics
2. Check which constants don't match
3. Either: Update code to match CANON, OR
4. Create new ADR if CANON needs changing

### "Need to change approved ADR"
1. Cannot edit approved ADR (immutable)
2. Create NEW ADR
3. Reference original ADR in "Supersedes" field
4. Document why change needed
5. Get approval
6. Regenerate CANON

---

## References

- **ADR Guide:** [ADR_GUIDE.md](ADR_GUIDE.md)
- **Validation Guide:** [VALIDATION_GUIDE.md](VALIDATION_GUIDE.md)
- **Agent Rules:** [agent-rules.yml](agent-rules.yml)
- **Implementation Brief:** [continuity/IMPLEMENTATION_BRIEF.md](continuity/IMPLEMENTATION_BRIEF.md)

---

**Document Status:** ACTIVE
**Last Review:** 2025-10-15
**Next Review:** Quarterly or when process changes needed
**Owner:** Captain SpectraSynq
