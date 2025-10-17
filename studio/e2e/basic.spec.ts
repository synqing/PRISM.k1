import { test, expect } from '@playwright/test';

test('app launches', async ({ page }) => {
  await page.goto('/');
  await expect(page).toHaveTitle(/PRISM Studio/i);
});

