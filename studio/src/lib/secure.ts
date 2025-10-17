// Secure credential storage helpers
// Tries Stronghold first; falls back to Store (encrypted at rest may not be guaranteed)

async function ensureStrongholdPassword(): Promise<boolean> {
  try {
    const stronghold: any = await import('@tauri-apps/plugin-stronghold');
    // If running in Tauri and stronghold is present
    if ((window as any).__TAURI_INTERNALS__ && stronghold?.setPassword) {
      const { invoke } = await import('@tauri-apps/api/core');
      const derived: string = await invoke('get_or_create_master_key');
      await stronghold.setPassword(derived);
      return true;
    }
  } catch { /* noop */ }
  return false;
}

export async function setSecret(key: string, value: string): Promise<void> {
  try {
    const stronghold: any = await import('@tauri-apps/plugin-stronghold');
    if (stronghold?.set) {
      await ensureStrongholdPassword();
      await stronghold.set(key, value);
      return;
    }
  } catch { /* noop */ }
  try {
    const storeMod: any = await import('@tauri-apps/plugin-store');
    const store = await storeMod.Store?.load?.('secrets.bin');
    if (store) {
      await store.set(key, value);
      await store.save();
    }
  } catch { /* noop */ }
}

export async function getSecret(key: string): Promise<string | null> {
  try {
    const stronghold: any = await import('@tauri-apps/plugin-stronghold');
    if (stronghold?.get) {
      await ensureStrongholdPassword();
      return (await stronghold.get(key)) ?? null;
    }
  } catch { /* noop */ }
  try {
    const storeMod: any = await import('@tauri-apps/plugin-store');
    const store = await storeMod.Store?.load?.('secrets.bin');
    if (store) return (await store.get(key)) ?? null;
  } catch { /* noop */ }
  return null;
}

export async function deleteSecret(key: string): Promise<void> {
  try {
    const stronghold: any = await import('@tauri-apps/plugin-stronghold');
    if (stronghold?.delete) {
      await ensureStrongholdPassword();
      await stronghold.delete(key);
      return;
    }
  } catch { /* noop */ }
  try {
    const storeMod: any = await import('@tauri-apps/plugin-store');
    const store = await storeMod.Store?.load?.('secrets.bin');
    if (store) {
      await store.delete(key);
      await store.save();
    }
  } catch { /* noop */ }
}
