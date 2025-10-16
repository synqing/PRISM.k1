# Deprecated Task Set (Pre-CANON Validation)

**Deprecated Date:** 2025-10-15
**Reason:** Regenerated with CANON corrections and full ADR metadata
**Replaced By:** `.taskmaster/tasks/` (CANON-validated task set)

## Why These Were Deprecated

These 56 tasks were generated before the Knowledge Fortress CANON system was established. They contain specification conflicts with approved ADRs:

### Critical Conflicts Found
1. **Task 002** - Wrong partition table (1MB vs 1.5MB)
2. **Task 004** - Wrong storage paths (`/prism` vs `/littlefs`)
3. **Task 017** - Wrong client limit (5 vs 2)
4. **Task 018** - Wrong LED count (150 vs 320)
5. **Task 012** - Wrong pattern size (200KB vs 256KB)

### Full Validation Report
See `.taskmaster/audit/TASK_VALIDATION_REPORT.md` for complete analysis.

## DO NOT USE

These tasks are preserved for historical reference only. All new work must use the regenerated task set in `.taskmaster/tasks/` which includes:
- Full CANON alignment with 6 ADRs
- Machine-readable ADR references
- Validation metadata
- Zero specification ambiguity
