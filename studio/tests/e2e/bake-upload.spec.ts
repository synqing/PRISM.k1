import { test, expect } from '@playwright/test';

test('DevicePanel renders and has Bake & Upload', async ({ page }) => {
  // This is a placeholder smoke test; full E2E requires Tauri runtime.
  await page.goto('http://localhost:5173/');
  await expect(page.getByText('PRISM Studio')).toBeVisible();
  await expect(page.getByText('Bake & Upload')).toBeVisible();
});

