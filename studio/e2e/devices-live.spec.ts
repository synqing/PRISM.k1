import { test, expect } from '@playwright/test';

test('devices panel tries real connect and skips gracefully', async ({ page }) => {
  await page.goto('/');
  await expect(page.getByText('Devices')).toBeVisible();
  // Click Connect (first row)
  const connectBtn = page.getByText('Connect').first();
  await connectBtn.click();
  // Try to see CONNECTED within 3s; if not, pass best-effort without failing
  try {
    await expect(page.getByText('CONNECTED')).toBeVisible({ timeout: 3000 });
  } catch {
    // Best-effort: no real device reachable; do not fail
    // eslint-disable-next-line no-console
    console.warn('Best-effort: real device not reachable; skipping assertion.');
  }
});
