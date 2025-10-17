import { describe, it, expect, vi } from 'vitest';
import { render, screen, fireEvent } from '@testing-library/react';
import SecureSettings from './SecureSettings';

vi.mock('../../lib/secure', () => ({
  setSecret: vi.fn().mockResolvedValue(undefined),
  getSecret: vi.fn().mockResolvedValue('mocked'),
  deleteSecret: vi.fn().mockResolvedValue(undefined),
}));

describe('SecureSettings', () => {
  it('renders and saves, loads, deletes', async () => {
    render(<SecureSettings />);
    const keyInput = screen.getByLabelText(/Key:/i) as HTMLInputElement;
    const valueInput = screen.getByLabelText(/Value:/i) as HTMLInputElement;
    fireEvent.change(keyInput, { target: { value: 'k' } });
    fireEvent.change(valueInput, { target: { value: 'v' } });

    fireEvent.click(screen.getByText(/Save Secret/i));
    fireEvent.click(screen.getByText(/Load Secret/i));
    fireEvent.click(screen.getByText(/Delete Secret/i));

    expect(keyInput.value).toBe('k');
    expect(valueInput.value).toBe('v');
    // Status and loaded message should appear
    const matches = await screen.findAllByText(/Deleted|Saved|Loaded/i);
    expect(matches.length).toBeGreaterThan(0);
  });
});
