# OTA Upgrade / Rollback Checklist (v1.1)

Use this checklist before publishing the v1.1 firmware to ensure OTA upgrade and rollback behaviour is verified on all soak devices.

## Pre-requisites
- Firmware artifacts: `firmware/build/prism-k1.bin`, bootloader, partition table.
- Previous stable build (v1.0) available for rollback testing.
- Device list aligned with `logs/soak_devices.yaml`.
- Network credentials and OTA endpoint (e.g., `http://<device>/api/ota`).

## Upgrade Procedure
1. Record baseline firmware version (`GET /metrics` or serial log).
2. Trigger OTA upload:
   - `curl -X POST http://<device>/api/ota --data-binary @firmware/build/prism-k1.bin`
   - or use provided dashboard upload tool.
3. Wait for reboot and confirm v1.1 boot banner via serial or metrics endpoint.
4. Verify:
   - Device rejoins network automatically.
   - `GET /metrics` reports new firmware version.
   - Load at least one preset from `out/presets/` to confirm playback.

## Rollback Procedure
1. Trigger rollback (hold firmware button or invoke `/api/ota/rollback` if supported).
2. Confirm device reverts to previous firmware (check metrics banner).
3. Re-apply v1.1 OTA and ensure second upgrade succeeds.

## Evidence to Capture
- OTA request logs (curl output, dashboard screenshots).
- Serial logs showing upgrade stages and final firmware tag.
- Metrics snapshots before/after upgrade.
- Notes on any required recovery actions.

## Sign-off
- ✅ dev1 upgrade + rollback validated (operator, timestamp)
- ✅ dev2 upgrade + rollback validated (operator, timestamp)
- ✅ dev3 upgrade + rollback validated (operator, timestamp)
