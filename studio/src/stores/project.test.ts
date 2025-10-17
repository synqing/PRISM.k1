import { describe, it, expect } from 'vitest';
import { ProjectSchema, createEmptyProject, MotionEnum, SyncModeEnum } from '../lib/projectSchema';
import { useProjectStore, serializeProject, deserializeProject } from './project';

describe('Project schema', () => {
  it('creates a valid empty project', () => {
    const p = createEmptyProject('Test');
    expect(() => ProjectSchema.parse(p)).not.toThrow();
    expect(p.name).toBe('Test');
    expect(p.tracks).toEqual([]);
    expect(p.device.ledCount).toBeGreaterThan(0);
    expect(p.motion).toBe('STATIC');
    expect(p.sync.mode).toBe('SYNC');
  });
});

describe('Project store', () => {
  it('updates project name and timestamps', () => {
    const { getState, setState } = useProjectStore;
    // Reset store for a clean test
    setState({ ...getState(), project: createEmptyProject('Untitled') });
    const before = getState().project.updatedAt;
    getState().setName('Renamed');
    const after = getState().project.updatedAt;
    expect(getState().project.name).toBe('Renamed');
    expect(new Date(after).getTime()).toBeGreaterThanOrEqual(new Date(before).getTime());
  });

  it('serializes and deserializes project', () => {
    const p = createEmptyProject('Serialize Test');
    p.motion = MotionEnum.enum.LEFT;
    p.sync = { mode: SyncModeEnum.enum.OFFSET, offsetMs: 80 } as any;
    const json = serializeProject(p);
    const parsed = deserializeProject(json);
    expect(parsed.name).toBe('Serialize Test');
    expect(parsed.version).toBe(1);
    expect(parsed.motion).toBe('LEFT');
    expect(parsed.sync.mode).toBe('OFFSET');
  });
});

describe('Undo/Redo', () => {
  it('supports undo/redo', () => {
    const { getState, setState } = useProjectStore;
    // reset
    setState({ ...getState(), project: createEmptyProject('Untitled') });
    getState().setName('First');
    getState().setName('Second');

    useProjectStore.temporal.getState().undo();
    expect(getState().project.name).toBe('First');

    useProjectStore.temporal.getState().redo();
    expect(getState().project.name).toBe('Second');
  });
});
