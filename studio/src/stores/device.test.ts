import { describe, it, expect, vi, beforeEach } from 'vitest';
import { useDeviceStore, type DeviceInfo } from './device';

vi.mock('@tauri-apps/api/core', () => {
  return {
    invoke: vi.fn(),
  };
});

import { invoke } from '@tauri-apps/api/core';

describe('Device store (immer drafts)', () => {
  beforeEach(() => {
    // reset store between tests
    const { getState, setState } = useDeviceStore;
    setState({ ...getState(), devices: [], selected: null, lastError: null });
    (invoke as any).mockReset();
  });

  it('discovers devices and updates state with immer mutation', async () => {
    const devices: DeviceInfo[] = [
      { name: 'prism-k1', host: 'prism-k1.local.', port: 80 },
    ];
    (invoke as any).mockResolvedValueOnce(devices);

    await useDeviceStore.getState().discover();

    expect(useDeviceStore.getState().devices).toEqual(devices);
    expect(useDeviceStore.getState().lastError).toBeNull();
  });

  it('connect sets selected and clears error', async () => {
    const d: DeviceInfo = { name: 'prism-k1', host: 'prism-k1.local.', port: 80 };
    (invoke as any).mockResolvedValueOnce('CONNECTED');

    await useDeviceStore.getState().connect(d);

    expect(useDeviceStore.getState().selected).toEqual(d);
    expect(useDeviceStore.getState().lastError).toBeNull();
  });

  it('undo/redo device selection with zundo temporal', async () => {
    const a: DeviceInfo = { name: 'A', host: 'a.local.', port: 80 };
    const b: DeviceInfo = { name: 'B', host: 'b.local.', port: 80 };
    (invoke as any).mockResolvedValue('CONNECTED');

    await useDeviceStore.getState().connect(a);
    await useDeviceStore.getState().connect(b);

    // Undo should go back to A
    (useDeviceStore as any).temporal.getState().undo();
    expect(useDeviceStore.getState().selected?.name).toBe('A');

    // Redo returns to B
    (useDeviceStore as any).temporal.getState().redo();
    expect(useDeviceStore.getState().selected?.name).toBe('B');
  });
});
