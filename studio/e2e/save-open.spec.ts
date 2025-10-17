import { test, expect } from '@playwright/test';

test('save/open roundtrip via dev helpers', async ({ page }) => {
  await page.goto('/');

  // Ensure helpers exist
  await page.waitForFunction(() => {
    // @ts-ignore
    return typeof window.__e2e !== 'undefined';
  });

  const fname = 'roundtrip_e2e.prismproj';

  // Set a unique name and save to a known path
  await page.evaluate(() => {
    // @ts-ignore
    window.__e2e.setName('Roundtrip E2E');
  });
  await page.evaluate((f) => {
    // @ts-ignore
    return window.__e2e.saveTo(f);
  }, fname);

  // Change name in-memory to prove open replaces it
  await page.evaluate(() => {
    // @ts-ignore
    window.__e2e.setName('Modified');
  });

  // Open from path and assert name restored
  const name = await page.evaluate((f) => {
    // @ts-ignore
    return window.__e2e.openFrom(f).then(() => window.__e2e.getProjectName());
  }, fname);
  expect(name).toBe('Roundtrip E2E');
});

