# PRISM K1 Validation Guide

**Version:** 1.0.0
**Last Updated:** 2025-10-15
**Status:** ACTIVE
**Reviewer:** Captain SpectraSynq

---

## Purpose

This guide defines validation procedures for the PRISM K1 Knowledge Fortress system. It provides checklists, review criteria, and workflows for ensuring research and architectural decisions meet quality standards.

**Target Audience:**
- Captain SpectraSynq (primary reviewer)
- Future agents conducting research
- Contributors creating ADRs

---

## Table of Contents

1. [Captain Review Process](#captain-review-process)
2. [Research Validation](#research-validation)
3. [ADR Validation](#adr-validation)
4. [Code Validation](#code-validation)
5. [Common Validation Failures](#common-validation-failures)
6. [Validation Workflows](#validation-workflows)
7. [Quality Metrics](#quality-metrics)

---

## Captain Review Process

### Overview

**Captain SpectraSynq** is the sole reviewer for all research and architectural decisions. This centralized review ensures:
- Consistent quality standards
- Single authoritative voice
- Rapid decision-making
- Clear accountability

### Captain Review Responsibilities

**For Research Documents:**
- ✅ Verify methodology is reproducible
- ✅ Challenge assumptions and conclusions
- ✅ Check for conflicts with existing validated research
- ✅ Assess evidence quality and sufficiency
- ✅ Provide APPROVE / NEEDS_REVISION / REJECT decision

**For Architecture Decision Records:**
- ✅ Verify all required sections present
- ✅ Ensure decision cites validated evidence
- ✅ Check alternatives were genuinely considered
- ✅ Assess consequences are realistic
- ✅ Verify implementation details are complete
- ✅ Provide APPROVE / NEEDS_REVISION / REJECT decision

### Review Outcomes

**APPROVE:**
- Research: Move to `.taskmaster/research/[VALIDATED]/`
- ADR: Update status to `APPROVED`, becomes immutable
- Agent: Proceed with implementation or next phase

**NEEDS_REVISION:**
- Document specific concerns in review comments
- List required changes clearly
- Set expectations for revision scope
- Author revises and resubmits
- Restart review process

**REJECT:**
- Document rejection rationale thoroughly
- Move research to `.taskmaster/research/[REJECTED]/`
- Archive with lessons learned
- Provide guidance for future attempts

### Review Timeline

**Target Response Times:**
- Simple research (< 5 pages): 24 hours
- Complex research (5-20 pages): 48 hours
- ADRs: 24 hours
- Emergency decisions: 4 hours

**Escalation:** If review delayed beyond targets, agent should:
1. Check if reviewer notified
2. Verify document is in correct location
3. Send reminder after 2x target time
4. Escalate if no response after 3x target time

---

## Research Validation

### Phase 1: Self-Validation (Agent)

Before submitting for Captain review, agent MUST complete this checklist:

#### Document Structure
- [ ] YAML frontmatter complete with all required fields
- [ ] Title clearly states research topic
- [ ] Status set to `PROPOSED`
- [ ] Author identified
- [ ] Date in YYYY-MM-DD format
- [ ] Category appropriate (MEASUREMENT | ANALYTICAL | SECURITY | PRODUCTION)
- [ ] Question explicitly stated
- [ ] Methodology section present
- [ ] Impact level assigned (LOW | MEDIUM | HIGH | CRITICAL)

#### Content Completeness
- [ ] Executive Summary (1-2 paragraphs maximum)
- [ ] Methodology section (reproducible detail level)
- [ ] Data section with raw measurements/observations
- [ ] Analysis section with interpretation
- [ ] Conclusions section answering the question
- [ ] Limitations section acknowledging boundaries
- [ ] Recommendations section with actionable steps

#### Methodology Quality
- [ ] Research question explicitly stated
- [ ] Approach described in sufficient detail for reproduction
- [ ] Tools and equipment listed with versions
- [ ] Test duration specified
- [ ] Sample size documented (if applicable)
- [ ] Variables controlled or acknowledged
- [ ] Assumptions stated explicitly
- [ ] Uncertainties quantified or acknowledged

#### Data Quality
- [ ] Raw data provided (not just summaries)
- [ ] Measurements include units
- [ ] Timestamps included where relevant
- [ ] Outliers identified and explained
- [ ] Data presentation clear (tables/graphs if appropriate)
- [ ] Source data traceable

#### Analysis Quality
- [ ] Calculations shown or referenced
- [ ] Statistical methods appropriate for data type
- [ ] Conclusions supported by presented data
- [ ] Alternative explanations considered
- [ ] Bias sources identified
- [ ] Confidence levels stated (if applicable)

#### Formatting & Style
- [ ] Markdown formatting correct
- [ ] Code blocks properly formatted with language tags
- [ ] Tables use proper markdown syntax
- [ ] Links functional
- [ ] No spelling errors in critical sections
- [ ] Technical terms defined on first use

### Phase 2: Captain Validation

Captain uses this checklist when reviewing:

#### Initial Assessment (5 minutes)
- [ ] Document structure complete
- [ ] Question is clear and answerable
- [ ] Scope is reasonable for research investment
- [ ] Category assignment correct
- [ ] No immediate red flags (conflicts, ethical issues)

#### Methodology Review (15-30 minutes)
- [ ] **Reproducibility Test:** Could another agent reproduce this?
  - Are steps explicit enough?
  - Are tools/versions specified?
  - Are parameters documented?
  - Are conditions described?

- [ ] **Rigor Assessment:** Is the approach sound?
  - Sample size adequate?
  - Control variables identified?
  - Confounding factors addressed?
  - Measurement error quantified?

- [ ] **Assumption Validation:** Are assumptions reasonable?
  - Explicitly stated?
  - Justified with evidence or rationale?
  - Limitations acknowledged?
  - Bias sources identified?

#### Data Review (10-20 minutes)
- [ ] **Completeness:** All relevant data provided?
- [ ] **Transparency:** Raw data, not just processed?
- [ ] **Consistency:** Data matches methodology?
- [ ] **Anomalies:** Outliers explained?
- [ ] **Presentation:** Clear and accessible?

#### Analysis Review (15-30 minutes)
- [ ] **Logic:** Analysis follows from data?
- [ ] **Methods:** Statistical approaches appropriate?
- [ ] **Alternatives:** Other interpretations considered?
- [ ] **Limitations:** Acknowledged and explained?
- [ ] **Conclusions:** Supported by evidence?

#### Conflict Check (10 minutes)
- [ ] **Existing Research:** Conflicts with validated research?
- [ ] **ADRs:** Contradicts approved decisions?
- [ ] **CANON:** Aligns with current specifications?
- [ ] **PRD:** Supports project requirements?

#### Final Decision
- [ ] **APPROVE:** All criteria met, ready for [VALIDATED]
- [ ] **NEEDS_REVISION:** Specific issues documented below
- [ ] **REJECT:** Fundamental flaws, archive with rationale

**Review Notes Template:**
```markdown
## Captain Review - [APPROVE | NEEDS_REVISION | REJECT]

**Reviewer:** Captain SpectraSynq
**Date:** YYYY-MM-DD
**Review Duration:** X minutes

### Strengths
- [What was done well]
- [Particularly strong aspects]

### Concerns
- [Issues requiring attention]
- [Questions needing answers]

### Required Changes (if NEEDS_REVISION)
1. [Specific change #1]
2. [Specific change #2]

### Decision Rationale
[Why this decision was made]

### Next Steps
[What agent should do next]
```

### Phase 3: Post-Approval

After Captain approves:
- [ ] Move research to `.taskmaster/research/[VALIDATED]/`
- [ ] Add `validated_by` and `validated_date` to frontmatter
- [ ] Update research index if maintained
- [ ] Notify requesting agent/task
- [ ] Research now citable in ADRs

---

## ADR Validation

### Phase 1: Self-Validation (Agent)

Before submitting ADR for Captain approval:

#### Structure Validation
- [ ] All 9 required sections present (see ADR_GUIDE.md)
- [ ] YAML frontmatter complete
- [ ] Status set to `PROPOSED`
- [ ] Unique ADR number assigned
- [ ] Title clear and descriptive
- [ ] Date and author documented
- [ ] Category assigned

#### Content Validation

**1. Context Section:**
- [ ] Explains WHY decision is needed
- [ ] Describes problem or question
- [ ] Outlines constraints and requirements
- [ ] Provides background for readers unfamiliar with issue

**2. Research Evidence Section:**
- [ ] Cites validated research OR
- [ ] Cites measurements with methodology OR
- [ ] Cites authoritative external sources (ESP-IDF docs, datasheets)
- [ ] NO unsupported opinions
- [ ] NO "best practices" without source
- [ ] Evidence directly relevant to decision

**3. Decision Section:**
- [ ] Decision statement is clear and unambiguous
- [ ] Machine-readable YAML included if introducing constants
- [ ] Rationale explains WHY this choice
- [ ] Decision is actionable (implementable)

**4. Alternatives Considered:**
- [ ] At least 2 alternatives documented
- [ ] Each alternative has pros listed
- [ ] Each alternative has cons listed
- [ ] Explains why alternatives were rejected
- [ ] Genuine consideration (not strawman arguments)

**5. Consequences Section:**
- [ ] Positive consequences identified
- [ ] Negative consequences acknowledged
- [ ] Neutral/informational items noted
- [ ] Realistic assessment (not overly optimistic)
- [ ] Long-term implications considered

**6. Validation Criteria:**
- [ ] Measurable success criteria defined
- [ ] Failure conditions identified
- [ ] Testing approach outlined
- [ ] Monitoring requirements specified

**7. Implementation Section:**
- [ ] Affected components listed
- [ ] Configuration changes specified
- [ ] Code changes outlined
- [ ] Migration steps documented (if applicable)

**8. References:**
- [ ] All research sources linked
- [ ] External citations complete
- [ ] Related ADRs referenced
- [ ] No broken links

**9. Metadata:**
- [ ] Supersedes field populated (if replacing ADR)
- [ ] Superseded By left empty (populated later if replaced)
- [ ] Dependencies documented
- [ ] Tags appropriate

#### Quality Checks
- [ ] Writing is clear and professional
- [ ] Technical accuracy verified
- [ ] No ambiguous statements
- [ ] Examples provided where helpful
- [ ] Consistent terminology with CANON

### Phase 2: Captain Validation

Captain's ADR review checklist:

#### Quick Check (5 minutes)
- [ ] All sections present
- [ ] Evidence section not empty
- [ ] At least 2 alternatives considered
- [ ] Decision statement exists and is clear

#### Evidence Review (10-15 minutes)
- [ ] **Source Verification:**
  - Research cited is in [VALIDATED] folder?
  - External sources are authoritative?
  - Measurements include methodology?
  - Citations are accessible?

- [ ] **Relevance Check:**
  - Evidence directly supports decision?
  - No logical leaps?
  - Quantity of evidence sufficient?

- [ ] **Conflicts:**
  - Check against existing ADRs
  - Check against CANON
  - Check against PRD
  - Check against validated research

#### Decision Quality (10 minutes)
- [ ] **Clarity:** Decision unambiguous?
- [ ] **Completeness:** All parameters specified?
- [ ] **Implementability:** Can be coded without interpretation?
- [ ] **YAML Correctness:** Syntax valid? Values reasonable?

#### Alternatives Assessment (10 minutes)
- [ ] **Genuine Consideration:** Not strawman arguments?
- [ ] **Completeness:** Major alternatives covered?
- [ ] **Analysis:** Pros/cons are realistic?
- [ ] **Justification:** Clear why chosen over alternatives?

#### Consequences Review (10 minutes)
- [ ] **Honesty:** Negative consequences acknowledged?
- [ ] **Realism:** Not overly optimistic?
- [ ] **Completeness:** Long-term implications considered?
- [ ] **Trade-offs:** Clearly articulated?

#### Implementation Review (10 minutes)
- [ ] **Specificity:** Clear what needs to change?
- [ ] **Feasibility:** Implementation is realistic?
- [ ] **Dependencies:** All identified?
- [ ] **Migration:** If needed, path is clear?

#### Final Decision
- [ ] **APPROVE:** Ready to become immutable
- [ ] **NEEDS_REVISION:** Issues documented
- [ ] **REJECT:** Fundamental problems, start over

**ADR Review Template:**
```markdown
## Captain ADR Review - [APPROVE | NEEDS_REVISION | REJECT]

**ADR Number:** ADR-XXX
**Title:** [ADR Title]
**Reviewer:** Captain SpectraSynq
**Date:** YYYY-MM-DD

### Evidence Quality: [EXCELLENT | GOOD | ACCEPTABLE | INSUFFICIENT]
[Comments on research citations and evidence]

### Decision Clarity: [EXCELLENT | GOOD | ACCEPTABLE | UNCLEAR]
[Comments on decision statement and specifications]

### Alternatives Analysis: [THOROUGH | ADEQUATE | WEAK]
[Comments on alternatives consideration]

### Implementation: [DETAILED | ADEQUATE | INCOMPLETE]
[Comments on implementation section]

### Issues Requiring Attention:
1. [Issue #1]
2. [Issue #2]

### Decision: [APPROVE | NEEDS_REVISION | REJECT]

**Rationale:**
[Why this decision was made]

**Next Steps:**
[What agent should do]
```

### Phase 3: Post-Approval

After Captain approves ADR:
- [ ] Update ADR status to `APPROVED`
- [ ] Add `approved_by` and `approved_date` to frontmatter
- [ ] ADR becomes **IMMUTABLE** (no further edits allowed)
- [ ] Run `./scripts/generate-canon.sh` to update CANON.md
- [ ] Run `./scripts/sync-code-to-canon.sh` to generate code
- [ ] Validate with `./scripts/validate-canon.sh`
- [ ] Commit with message: `adr: Add ADR-XXX [ADR title]`

---

## Code Validation

### Automated Validation

**Pre-Commit Hook** (`.githooks/pre-commit`):
```bash
# Prevents manual CANON.md edits
# Blocks commits without [AUTO] marker if CANON changed
# Validates ADR format if ADRs modified
```

**Pre-Push Hook** (`.githooks/pre-push`):
```bash
# Runs ./scripts/validate-canon.sh
# Blocks push if validation fails
# Can bypass with --no-verify (NOT recommended)
```

**CI/CD Pipeline** (`.github/workflows/canon-validation.yml`):
- Validates on every push/PR to main/develop
- 5-step validation process
- Firmware build only if validation passes

### Manual Validation

**Run validation manually:**
```bash
cd /Users/spectrasynq/Workspace_Management/Software/PRISM.k1/.taskmaster
./scripts/validate-canon.sh
```

**Expected output if passing:**
```
✓ Step 1: CANON freshness check - PASS
✓ Step 2: Constants validation - PASS
✓ Step 3: sdkconfig validation - PASS
✓ Step 4: Partition table validation - PASS
✓ Step 5: Header generation - PASS

All validation checks passed!
```

**If validation fails:**
1. Read error message carefully
2. Identify which constant/config doesn't match
3. Either:
   - **Option A:** Update code to match CANON (usual case)
   - **Option B:** Create new ADR if CANON needs changing
4. Re-run validation
5. Don't bypass validation unless emergency

### Code Review Checklist

When implementing from CANON:

- [ ] **Constants Match:**
  - All `#define` values match CANON YAML
  - No hardcoded magic numbers
  - Use generated `prism_config.h`

- [ ] **Configuration Match:**
  - `sdkconfig.defaults` matches CANON specifications
  - No undocumented config changes
  - Component configs aligned

- [ ] **Partition Table:**
  - `partitions.csv` matches CANON exactly
  - No size/offset changes without ADR
  - Labels match specifications

- [ ] **Documentation:**
  - Code comments reference ADR numbers
  - Complex logic explained
  - Assumptions documented

- [ ] **Error Handling:**
  - Error codes from CANON specifications
  - Failures handled gracefully
  - Logging follows standards

---

## Common Validation Failures

### Research Validation Failures

**Failure: "Methodology not reproducible"**
- **Symptom:** Captain cannot determine how to reproduce research
- **Fix:** Add explicit step-by-step procedure, tool versions, parameters
- **Example:**
  ```markdown
  ❌ "Tested buffer sizes"
  ✅ "Tested buffer sizes 1KB, 2KB, 4KB, 8KB using ESP-IDF v5.2.1
      on ESP32-S3-DevKitC-1, with 100 iterations per size,
      measuring allocation success rate over 24 hours"
  ```

**Failure: "Assumptions not stated"**
- **Symptom:** Research makes implicit assumptions
- **Fix:** Add explicit "Assumptions" section
- **Example:**
  ```markdown
  ❌ "Buffer size affects performance"
  ✅ "**Assumption:** WiFi network latency < 50ms. Performance
      results may differ on high-latency networks."
  ```

**Failure: "Conclusions not supported by data"**
- **Symptom:** Conclusions go beyond what data shows
- **Fix:** Limit conclusions to what data actually demonstrates
- **Example:**
  ```markdown
  ❌ "4KB is the optimal buffer size for all use cases"
  ✅ "4KB provides best balance for tested scenarios (2-client
      websocket with 320 LED patterns). Other use cases may differ."
  ```

**Failure: "Conflicts with existing research"**
- **Symptom:** Research contradicts validated findings
- **Fix:** Either explain discrepancy or conduct new research to resolve
- **Example:**
  ```markdown
  **Conflict Resolution:**
  Research-042 found 8KB optimal, but that was under different
  conditions (single client, short duration). This research (2 clients,
  24 hour duration) shows fragmentation issues with 8KB. Both are
  valid in their contexts. Recommend ADR specifying context-dependent
  sizing.
  ```

### ADR Validation Failures

**Failure: "No evidence cited"**
- **Symptom:** Decision based on opinion
- **Fix:** Add research citations or conduct research first
- **Example:**
  ```markdown
  ❌ "We should use 4KB because it's a good size"
  ✅ "Set buffer to 4KB based on [VALIDATED] research-042
      showing 98% allocation success over 24h vs 85% for 8KB"
  ```

**Failure: "Alternatives are strawmen"**
- **Symptom:** Alternatives obviously bad, not genuine consideration
- **Fix:** Present realistic alternatives with honest pros/cons
- **Example:**
  ```markdown
  ❌ Alternative: 1MB buffer
      Cons: Way too large, would crash immediately

  ✅ Alternative: 8KB buffer
      Pros: Higher throughput (15% faster in benchmarks)
      Cons: Memory fragmentation after 12h (research-042)
      Rejected because: Reliability more critical than throughput
  ```

**Failure: "Decision ambiguous"**
- **Symptom:** Decision can be interpreted multiple ways
- **Fix:** Make decision crystal clear with YAML specs
- **Example:**
  ```markdown
  ❌ "Use reasonable buffer size"
  ✅ "Set WebSocket frame buffer to exactly 4096 bytes
      ```yaml
      ws_buffer_size: 4096
      ```"
  ```

**Failure: "Implementation details missing"**
- **Symptom:** Unclear how to implement decision
- **Fix:** Add specific code changes, files affected, steps
- **Example:**
  ```markdown
  ❌ "Update the buffer configuration"
  ✅ "Update configuration:
      1. Add to prism_config.h: #define WS_BUFFER_SIZE 4096
      2. Modify ws_handler.c: Use WS_BUFFER_SIZE in malloc()
      3. Update sdkconfig.defaults: CONFIG_WS_BUFFER=4096"
  ```

### Code Validation Failures

**Failure: "Constant doesn't match CANON"**
```
❌ Code has: #define WS_BUFFER_SIZE 8192
✅ CANON specifies: ws_buffer_size: 4096

Fix: Update code to match CANON, or create new ADR if CANON needs changing
```

**Failure: "Manual CANON edit detected"**
```
❌ Direct edit to CANON.md
✅ Must regenerate from ADRs using ./scripts/generate-canon.sh

Fix: Revert CANON.md changes, update ADR instead, regenerate
```

**Failure: "Partition table mismatch"**
```
❌ partitions.csv has: littlefs, data, , 0x310000, 0x0F0000
✅ CANON specifies: 0x300000, 0x100000

Fix: Update partitions.csv to match CANON exactly
```

---

## Validation Workflows

### Workflow 1: Research Validation

```
┌─────────────────────────────────────────────┐
│ Agent completes research document          │
│ Location: research/[PROPOSED]/             │
└───────────────┬─────────────────────────────┘
                │
                ▼
┌─────────────────────────────────────────────┐
│ Agent self-validates using checklist       │
│ (Phase 1: Self-Validation)                 │
└───────────────┬─────────────────────────────┘
                │
                ▼
┌─────────────────────────────────────────────┐
│ Agent notifies Captain: "Research ready"   │
│ Status: PROPOSED → IN_REVIEW               │
└───────────────┬─────────────────────────────┘
                │
                ▼
┌─────────────────────────────────────────────┐
│ Captain reviews using validation checklist │
│ Timeline: 24-48h depending on complexity   │
└───────────────┬─────────────────────────────┘
                │
        ┌───────┴───────┐
        │               │
        ▼               ▼
   [APPROVE]      [NEEDS_REVISION]        [REJECT]
        │               │                     │
        │               ▼                     │
        │    ┌─────────────────────────┐     │
        │    │ Agent revises per       │     │
        │    │ Captain feedback        │     │
        │    └──────────┬──────────────┘     │
        │               │                     │
        │               └─────► (back to Captain review)
        │                                     │
        ▼                                     ▼
┌────────────────┐                  ┌──────────────────┐
│ Move to        │                  │ Move to          │
│ [VALIDATED]/   │                  │ [REJECTED]/      │
│ Update status  │                  │ Archive with     │
│ Add metadata   │                  │ rationale        │
└────────────────┘                  └──────────────────┘
```

### Workflow 2: ADR Validation

```
┌─────────────────────────────────────────────┐
│ Agent creates ADR using create-adr.sh      │
│ Status: PROPOSED                           │
└───────────────┬─────────────────────────────┘
                │
                ▼
┌─────────────────────────────────────────────┐
│ Agent completes all 9 required sections   │
│ Self-validates using checklist            │
└───────────────┬─────────────────────────────┘
                │
                ▼
┌─────────────────────────────────────────────┐
│ Agent notifies Captain: "ADR ready"        │
│ Status: PROPOSED                           │
└───────────────┬─────────────────────────────┘
                │
                ▼
┌─────────────────────────────────────────────┐
│ Captain reviews using ADR checklist        │
│ Timeline: 24h standard                     │
└───────────────┬─────────────────────────────┘
                │
        ┌───────┴───────┐
        │               │
        ▼               ▼
   [APPROVE]      [NEEDS_REVISION]        [REJECT]
        │               │                     │
        │               ▼                     │
        │    ┌─────────────────────────┐     │
        │    │ Agent revises per       │     │
        │    │ Captain feedback        │     │
        │    └──────────┬──────────────┘     │
        │               │                     │
        │               └─────► (back to Captain review)
        │                                     │
        ▼                                     ▼
┌────────────────┐                  ┌──────────────────┐
│ Update to      │                  │ Archive          │
│ APPROVED       │                  │ Start over with  │
│ Now IMMUTABLE  │                  │ new approach     │
└───────┬────────┘                  └──────────────────┘
        │
        ▼
┌─────────────────────────────────────────────┐
│ Run generate-canon.sh to update CANON      │
└───────────────┬─────────────────────────────┘
                │
                ▼
┌─────────────────────────────────────────────┐
│ Run sync-code-to-canon.sh to generate code│
└───────────────┬─────────────────────────────┘
                │
                ▼
┌─────────────────────────────────────────────┐
│ Run validate-canon.sh to verify           │
└───────────────┬─────────────────────────────┘
                │
                ▼
┌─────────────────────────────────────────────┐
│ Commit with message: "adr: Add ADR-XXX"   │
└─────────────────────────────────────────────┘
```

### Workflow 3: Code Validation

```
┌─────────────────────────────────────────────┐
│ Developer writes/modifies code             │
└───────────────┬─────────────────────────────┘
                │
                ▼
┌─────────────────────────────────────────────┐
│ git add <files>                            │
└───────────────┬─────────────────────────────┘
                │
                ▼
┌─────────────────────────────────────────────┐
│ git commit -m "message"                    │
└───────────────┬─────────────────────────────┘
                │
                ▼
┌─────────────────────────────────────────────┐
│ PRE-COMMIT HOOK runs                       │
│ - Checks for manual CANON.md edits        │
│ - Validates ADR format if changed          │
└───────────────┬─────────────────────────────┘
                │
        ┌───────┴───────┐
        │               │
        ▼               ▼
      PASS            FAIL
        │               │
        │               ▼
        │    ┌─────────────────────────┐
        │    │ Fix issues              │
        │    │ Retry commit            │
        │    └──────────┬──────────────┘
        │               │
        │               └─────► (back to commit)
        │
        ▼
┌─────────────────────────────────────────────┐
│ git push origin <branch>                   │
└───────────────┬─────────────────────────────┘
                │
                ▼
┌─────────────────────────────────────────────┐
│ PRE-PUSH HOOK runs                         │
│ - Executes validate-canon.sh              │
│ - Checks constants, config, partitions     │
└───────────────┬─────────────────────────────┘
                │
        ┌───────┴───────┐
        │               │
        ▼               ▼
      PASS            FAIL
        │               │
        │               ▼
        │    ┌─────────────────────────┐
        │    │ Fix mismatches          │
        │    │ Update code or CANON    │
        │    └──────────┬──────────────┘
        │               │
        │               └─────► (back to push)
        │
        ▼
┌─────────────────────────────────────────────┐
│ Push succeeds                              │
└───────────────┬─────────────────────────────┘
                │
                ▼
┌─────────────────────────────────────────────┐
│ CI/CD VALIDATION runs                      │
│ - Re-validates everything                  │
│ - Builds firmware if validation passes     │
└─────────────────────────────────────────────┘
```

---

## Quality Metrics

### Research Quality Score

**Excellent (9-10/10):**
- Methodology perfectly reproducible
- Data complete with raw measurements
- Analysis thorough with alternatives considered
- Limitations clearly acknowledged
- Professional presentation
- Ready for publication-level review

**Good (7-8/10):**
- Methodology mostly reproducible (minor gaps)
- Data complete, some raw data missing
- Analysis sound, few alternatives explored
- Most limitations acknowledged
- Clean presentation
- Ready for approval with minor revisions

**Acceptable (5-6/10):**
- Methodology somewhat reproducible
- Data present but incomplete
- Analysis basic, conclusions supported
- Some limitations acknowledged
- Adequate presentation
- Needs revision before approval

**Insufficient (<5/10):**
- Methodology not reproducible
- Data missing or inadequate
- Analysis weak or unsupported
- Limitations not acknowledged
- Poor presentation
- Requires significant rework

### ADR Quality Score

**Excellent (9-10/10):**
- All sections complete and thorough
- Evidence comprehensive and validated
- Decision crystal clear with YAML specs
- Alternatives genuinely considered
- Consequences realistic and complete
- Implementation detailed and actionable
- Professional writing
- Zero ambiguity

**Good (7-8/10):**
- All sections present
- Evidence adequate and cited
- Decision clear with minor ambiguities
- Alternatives reasonable
- Consequences mostly complete
- Implementation adequate
- Good writing
- Minimal clarification needed

**Acceptable (5-6/10):**
- All sections present but some thin
- Evidence present but limited
- Decision mostly clear
- Alternatives considered but weak analysis
- Consequences incomplete
- Implementation needs more detail
- Adequate writing
- Needs revision

**Insufficient (<5/10):**
- Sections missing or incomplete
- Evidence weak or absent
- Decision ambiguous
- Alternatives strawmen or missing
- Consequences unrealistic
- Implementation vague
- Poor writing
- Major rework required

### Code Compliance Score

**Perfect Compliance (100%):**
- All constants match CANON exactly
- All configurations match specifications
- Partition table exact match
- Headers generated correctly
- No validation errors
- Full traceability (ADR comments)

**Good Compliance (90-99%):**
- Minor constant mismatches (1-2)
- Configurations mostly aligned
- Partition table correct
- Headers generated
- Few validation warnings
- Most code has ADR references

**Acceptable Compliance (70-89%):**
- Several constant mismatches
- Some configuration drift
- Partition table needs updates
- Headers need regeneration
- Multiple validation failures
- Incomplete ADR references

**Non-Compliant (<70%):**
- Many constants don't match
- Significant configuration drift
- Partition table wrong
- Headers outdated
- Validation blocked
- No ADR traceability

---

## Appendix: Quick Reference

### Captain Decision Matrix

| Document Type | Review Time | Key Criteria | Common Issues |
|--------------|-------------|--------------|---------------|
| Simple Research | 24h | Reproducible, data complete | Methodology vague |
| Complex Research | 48h | Rigorous analysis, limitations noted | Assumptions hidden |
| Standard ADR | 24h | Evidence cited, alternatives real | Strawman alternatives |
| Complex ADR | 48h | Implementation detailed, consequences honest | Ambiguous decisions |
| Emergency Decision | 4h | Minimum viable evidence, clear rationale | Rushed analysis |

### Validation Command Quick Reference

```bash
# Generate CANON from ADRs
./scripts/generate-canon.sh

# Create new ADR
./scripts/create-adr.sh

# Validate code matches CANON
./scripts/validate-canon.sh

# Generate code from CANON
./scripts/sync-code-to-canon.sh

# Extract constants as JSON
./scripts/extract-constants.sh
```

### Document Status Lifecycle

```
Research: PROPOSED → IN_REVIEW → VALIDATED | REJECTED
ADR: PROPOSED → APPROVED (immutable) | REJECTED
CANON: [AUTO] generated, never manual edits
Code: Must match CANON (enforced by hooks)
```

---

**Document Status:** ACTIVE
**Last Review:** 2025-10-15
**Next Review:** Quarterly or when validation process changes
**Owner:** Captain SpectraSynq

**Related Documents:**
- [METHODOLOGY.md](METHODOLOGY.md) - Research-first process
- [ADR_GUIDE.md](ADR_GUIDE.md) - How to write ADRs
- [CANON.md](CANON.md) - Single source of truth (auto-generated)
