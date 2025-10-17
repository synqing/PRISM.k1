import { test, expect } from '@playwright/test';

test('devices panel shows fallback and connects (best-effort)', async ({ page }) => {
  await page.addInitScript(() => {
    // @ts-ignore
    window.__E2E_FORCE_FAKE__ = true;
  });
  await page.goto('/');
  await expect(page.getByText('Devices')).toBeVisible();
  await expect(page.getByText('prism-k1.local')).toBeVisible();
  await page.getByText('Connect').first().click();
  await expect(page.getByText('CONNECTED')).toBeVisible();
});

