import { create } from 'zustand';
import { devtools } from 'zustand/middleware';
import { immer } from 'zustand/middleware/immer';
import { temporal } from 'zundo';
import { invoke } from '@tauri-apps/api/core';

export type DeviceInfo = {
  name: string;
  host: string;
  port: number;
};

type DeviceState = {
  devices: DeviceInfo[];
  selected: DeviceInfo | null;
  lastError: string | null;
  discover: () => Promise<void>;
  connect: (d: DeviceInfo) => Promise<void>;
  fetchStatus: () => Promise<any>;
  listPatterns: () => Promise<any>;
  deletePattern: (name: string) => Promise<boolean>;
};

export const useDeviceStore = create<DeviceState>()(
  devtools(
    temporal(
      immer((set, get) => ({
        devices: [],
        selected: null,
        lastError: null,
      discover: async () => {
        try {
          const list = await invoke<DeviceInfo[]>('device_discover');
          set((s) => {
            s.devices = list;
            s.lastError = null;
          }, false, 'device/discover');
        } catch (e: any) {
          set((s) => { s.lastError = String(e); }, false, 'device/error');
        }
      },
      connect: async (d: DeviceInfo) => {
        try {
          await invoke('device_connect', { host: d.host, port: d.port });
          set((s) => {
            s.selected = d;
            s.lastError = null;
          }, false, 'device/connect');
        } catch (e: any) {
          set((s) => { s.lastError = String(e); }, false, 'device/error');
        }
      },
      fetchStatus: async () => {
        const d = get().selected; if (!d) throw new Error('NO_DEVICE');
        return await invoke('device_status', { host: d.host });
      },
      listPatterns: async () => {
        const d = get().selected; if (!d) throw new Error('NO_DEVICE');
        return await invoke('device_list', { host: d.host });
      },
      deletePattern: async (name: string) => {
        const d = get().selected; if (!d) throw new Error('NO_DEVICE');
        return await invoke<boolean>('device_delete', { host: d.host, name });
      },
      }))
      ,
      {
        limit: 50,
        // Track only relevant keys in history
        partialize: (state) => ({ selected: state.selected, devices: state.devices }),
        equality: (a, b) => (a as any).selected === (b as any).selected && (a as any).devices === (b as any).devices,
      }
    )
  )
);

export const { undo: deviceUndo, redo: deviceRedo, clear: deviceHistoryClear } =
  (useDeviceStore as any).temporal.getState();
