import { describe, it, expect, beforeEach } from 'vitest';
import { render, screen, fireEvent } from '@testing-library/react';
import DevicePanel from './DevicePanel';

describe('DevicePanel patterns + status (fake mode)', () => {
  beforeEach(() => {
    (window as any).__E2E_FORCE_FAKE__ = true;
    (window as any).__TAURI_INTERNALS__ = undefined;
  });

  it('shows status and patterns after fake connect', async () => {
    render(<DevicePanel />);
    // Should show fallback device
    await screen.findByText('prism-k1.local');
    // Connect
    fireEvent.click(screen.getByText('Connect'));
    // Status is synthesized in fake mode
    expect(await screen.findByText(/Version:/i)).toBeDefined();
    // Patterns table rendered
    expect(await screen.findByText('Patterns')).toBeDefined();
  });
});
