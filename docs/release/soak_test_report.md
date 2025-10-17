# PRISM K1 Firmware v1.1 Soak Test Report

Use this document to capture evidence for the 24-hour validation run described in `docs/release/soak_test_runbook.md`. Keep raw telemetry, summaries, and photos under `logs/` (or an equivalent tracked location) and link them in the sections below.

## Overview
- **Test window:** <!-- YYYY-MM-DD HH:MM → YYYY-MM-DD HH:MM (local time) -->
- **Firmware build/tag:** <!-- e.g., firmware-v1.1 -->
- **Preset bundle:** <!-- e.g., out/presets bundle hash -->
- **Operator:** <!-- Name -->
- **Location/environment:** <!-- Ambient temperature, airflow notes -->

## Hardware Matrix

| Device ID | HW revision | PSU / cabling | Notes |
| --- | --- | --- | --- |
| dev1 | <!-- e.g., rev-B --> | <!-- 5V/3A USB-C --> | <!-- serial, special configs --> |
| dev2 |  |  |  |
| dev3 |  |  |  |

## Telemetry Summary

Populate using `python3 -m tools.validation.soak_telemetry --input logs/soak_raw.csv --summary logs/soak_summary.json`.

| Device | Samples | Max Fragmentation (%) | Frame p99 (ms) | Frame Budget (ms) | Max Temp (°C) | Errors |
| --- | --- | --- | --- | --- | --- | --- |
| dev1 |  |  |  |  |  |  |
| dev2 |  |  |  |  |  |  |
| dev3 |  |  |  |  |  |  |

- **Fragmentation limit:** 5 %
- **Frame budget margin:** 1.1×
- **Temperature limit:** 65 °C

## Playlist & Automation Details
- **Preset manifest path:** <!-- out/presets/manifest.json -->
- **Playback cadence:** <!-- e.g., rotate every 5 minutes -->
- **Playlist schedule CSV:** <!-- logs/soak_playlist.csv -->
- **Automation script:** <!-- command used -->
- **Telemetry cadence:** <!-- 60 s, etc. -->
- **Device config:** <!-- logs/soak_devices.yaml -->

## Observations
- ✅ <!-- Example: No frame drops observed -->
- ⚠️ <!-- Example: Device dev2 reached 62 °C at hour 20 -->

## Incidents & Mitigations
| Timestamp | Device | Preset | Description | Mitigation | Status |
| --- | --- | --- | --- | --- | --- |
|  |  |  |  |  |  |

## Evidence Attachments
- `logs/soak_raw.csv`
- `logs/soak_summary.json`
- `logs/soak_playlist.csv`
- `docs/release/templates/soak_log_template.csv` (completed)
- Hardware photos / clips: <!-- links -->
- Additional notes: <!-- e.g., Grafana dashboard -->

## Sign-off
- **QA reviewer:** <!-- Name + date -->
- **Firmware lead:** <!-- Name + date -->
- **Release owner:** <!-- Name + date -->

> Archive a copy of this report alongside the release notes when publishing the v1.1 release.
