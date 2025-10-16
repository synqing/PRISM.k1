# PRISM K1 Firmware Changelog

## [1.1.0] - 2025-10-16

### Added
- WAVE mode implemented with LUT-based `sin8` waveform (Task 17)
- Triangle and sawtooth waveform helpers (fixed-point)
- Profiling framework via `CONFIG_PRISM_PROFILE_TEMPORAL` (Task 18)
- Periodic profiling logs and DRAM placement for LUT
- Migration CLI `tools/migrate_prism.py` v1.0 → v1.1 (Task 20)
- User manual for temporal sequencing (docs/user-manual.md)
- Basic unit tests for wave LUT helpers

### Changed
- Playback engine effect `EFFECT_WAVE_SINGLE` now uses sine LUT

### Notes
- Use `idf.py menuconfig` to enable profiling when validating performance
- See `docs/phase3/wave_performance.md` for profiling guidance

---

## [1.0.0] - 2025-10-15
- Initial ADR-003 scaffolding and LED playback engine

