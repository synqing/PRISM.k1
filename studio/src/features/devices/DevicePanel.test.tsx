import { describe, it, expect, vi, beforeEach } from 'vitest';
import { render, screen, fireEvent, waitFor } from '@testing-library/react';
import DevicePanel from './DevicePanel';

vi.mock('@tauri-apps/api/core', () => ({
  invoke: vi.fn(async (cmd: string) => {
    if (cmd === 'device_discover') {
      return [{ name: 'mock-k1', host: 'prism-k1.local', port: 80 }];
    }
    if (cmd === 'device_connect') {
      return 'CONNECTED';
    }
    if (cmd === 'device_status') {
      return { status: 'ok' };
    }
    return null;
  })
}));

describe('DevicePanel', () => {
  beforeEach(() => {
    // Ensure we think we are in Tauri in this test
    (window as any).__TAURI_INTERNALS__ = {};
  });

  it('scans and lists devices', async () => {
    render(<DevicePanel />);
    expect(await screen.findByText('mock-k1')).toBeDefined();
  });

  it('connects to a device', async () => {
    render(<DevicePanel />);
    await screen.findByText('mock-k1');
    fireEvent.click(screen.getByText('Connect'));
    await waitFor(() => expect(screen.getByText('CONNECTED')).toBeDefined());
  });
});

