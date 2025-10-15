# PRISM K1 - Component Tests

**Task 1 Subtask 4:** Scaffolding smoke tests and build verification

## Test Files

- `test_component_init.c` - Unity smoke tests for all component `*_init()` functions

## Running Tests

### Build with Tests

```bash
cd firmware
idf.py build
```

The tests component is automatically included in the build.

### Run Unit Tests (ESP-IDF Test Runner)

```bash
# Flash firmware with tests
idf.py flash

# Run tests and monitor output
idf.py monitor
```

### Test Menu

When firmware boots, you can run specific tests:

```
Press ENTER to see menu
[*] Run all tests
[task1] Run Task 1 tests only
[network] Test network component
[storage] Test storage component
[playback] Test playback component
[templates] Test templates component
```

## Expected Boot Log (Task 1 Complete)

When flashed to hardware, the boot log should show:

```
I (xxx) PRISM-K1: ========================================
I (xxx) PRISM-K1: PRISM K1 LED Controller
I (xxx) PRISM-K1: Firmware: v1.0.0
I (xxx) PRISM-K1: Build: Oct 16 2025 01:30:00
I (xxx) PRISM-K1: ========================================
I (xxx) PRISM-K1: Chip: ESP32-S3
I (xxx) PRISM-K1: Cores: 2
I (xxx) PRISM-K1: Features: WiFi BLE
I (xxx) PRISM-K1: Flash size: 8 MB (embedded)
I (xxx) PRISM-K1: Free heap: ~400000 bytes
I (xxx) PRISM-K1: ========================================
I (xxx) PRISM-K1: Initializing memory pools...
I (xxx) heap_monitor: Heap monitor initialized
I (xxx) PRISM-K1: System initialized
I (xxx) PRISM-K1: Initializing firmware components...
I (xxx) network: Initializing network subsystem...
I (xxx) network: Network subsystem initialized (stub)
I (xxx) storage: Initializing storage subsystem...
I (xxx) storage: Storage subsystem initialized (stub)
I (xxx) playback: Initializing playback subsystem...
I (xxx) playback: Playback subsystem initialized (stub)
I (xxx) templates: Initializing template subsystem...
I (xxx) templates: Template subsystem initialized (stub)
I (xxx) PRISM-K1: All components initialized
I (xxx) PRISM-K1: Creating FreeRTOS tasks...
I (xxx) PRISM-K1: All tasks created
I (xxx) PRISM-K1: PRISM K1 started successfully!
I (xxx) playback: Playback task started on core 0 (HIGHEST priority)
I (xxx) network: Network task started on core 1
I (xxx) storage: Storage task started on core 0
I (xxx) templates: Templates task started on core 0
```

## Success Criteria (Subtask 4)

✅ **Build succeeds** - `idf.py build` completes without errors
✅ **Tests compile** - Unity tests included in build
✅ **No watchdog resets** - Firmware boots without WDT timeout
✅ **All init stages complete** - All 4 components initialize successfully
✅ **Tasks start** - All 4 FreeRTOS tasks begin execution
✅ **Correct core assignment** - Playback/storage on Core 0, network on Core 1
✅ **Correct priorities** - Playback (10), Network (5), Storage (4), Templates (3)

## Test Coverage

**Unit Tests:**
- ✅ `network_init()` returns ESP_OK
- ✅ `storage_init()` returns ESP_OK
- ✅ `playback_init()` returns ESP_OK
- ✅ `templates_init()` returns ESP_OK
- ✅ Sequential initialization (integration)
- ✅ Clean deinitialization
- ✅ Repeated init/deinit cycles (robustness)

**Integration Tests:**
- ✅ All components initialize in sequence (as in main.c)
- ✅ Components deinitialize cleanly
- ✅ 3 init/deinit cycles without errors

## Hardware Testing (When Available)

When hardware is available:

```bash
# Flash firmware
idf.py -p /dev/ttyUSB0 flash

# Monitor boot log
idf.py -p /dev/ttyUSB0 monitor
```

**Expected behavior:**
- Firmware boots within 2 seconds
- No watchdog resets (WDT timeout)
- All tasks start and run their 1-second delay loops
- Heap remains stable (~400KB free)
- No memory leaks after 1 minute

## Notes

- Tests use Unity framework (built into ESP-IDF)
- Each test is tagged: `[task1]`, `[network]`, `[storage]`, etc.
- Tests can be run individually or in groups
- Hardware testing validates real-world behavior
- Stub implementations pass all tests (as expected for Task 1)

## Next Steps

After Task 1 complete:
- Task 2 will add real WiFi initialization (network component)
- Task 3 will add WebSocket server (network component)
- Tasks 5-10 will fill in the other component stubs

These tests will continue to pass as long as the `*_init()` functions don't fail.
