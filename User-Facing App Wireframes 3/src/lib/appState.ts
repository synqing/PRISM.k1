/**
 * Global App State Manager
 * Centralized state for PRISM Studio
 */

import { create } from 'zustand';
import { AnimationPattern, AnimationConfig, PATTERN_PRESETS } from '../components/prism/K1AnimationEngine';

// Types
export interface Device {
  id: string;
  name: string;
  ip: string;
  ledCount: number;
  storageUsed: number;
  storageTotal: number;
  status: 'online' | 'offline' | 'syncing';
  lastSeen: Date;
}

export interface Pattern {
  id: string;
  name: string;
  description: string;
  thumbnail: string;
  duration: number;
  size: number;
  tags: string[];
  favorite: boolean;
  createdAt: Date;
  config: AnimationConfig;
}

export interface TimelineTrack {
  id: string;
  name: string;
  color: string;
  muted: boolean;
  solo: boolean;
  height: number;
  segments: TimelineSegment[];
}

export interface TimelineSegment {
  id: string;
  patternId: string;
  startTime: number;
  duration: number;
  color: string;
}

export interface Keyframe {
  time: number;
  value: number;
  easing: 'linear' | 'easeIn' | 'easeOut' | 'easeInOut';
}

export interface AutomationCurve {
  id: string;
  parameter: string;
  keyframes: Keyframe[];
}

interface AppState {
  // Playback State
  isPlaying: boolean;
  currentTime: number;
  playbackSpeed: number;
  loop: boolean;
  loopIn: number;
  loopOut: number;
  
  // Timeline State
  tracks: TimelineTrack[];
  automationCurves: AutomationCurve[];
  selectedSegmentIds: string[];
  timelineZoom: number;
  timelineScroll: number;
  
  // Pattern State
  patterns: Pattern[];
  currentPattern: Pattern | null;
  
  // Device State
  devices: Device[];
  selectedDeviceId: string | null;
  
  // UI State
  showShortcuts: boolean;
  quality: 'HQ' | 'LQ';
  theme: 'light' | 'dark';
  
  // Actions
  setPlaying: (playing: boolean) => void;
  setCurrentTime: (time: number) => void;
  setPlaybackSpeed: (speed: number) => void;
  toggleLoop: () => void;
  setLoopPoints: (inPoint: number, outPoint: number) => void;
  
  addTrack: (track: TimelineTrack) => void;
  removeTrack: (trackId: string) => void;
  updateTrack: (trackId: string, updates: Partial<TimelineTrack>) => void;
  
  addSegment: (trackId: string, segment: TimelineSegment) => void;
  removeSegment: (segmentId: string) => void;
  updateSegment: (segmentId: string, updates: Partial<TimelineSegment>) => void;
  selectSegments: (segmentIds: string[]) => void;
  
  addPattern: (pattern: Pattern) => void;
  removePattern: (patternId: string) => void;
  updatePattern: (patternId: string, updates: Partial<Pattern>) => void;
  setCurrentPattern: (pattern: Pattern | null) => void;
  togglePatternFavorite: (patternId: string) => void;
  
  addDevice: (device: Device) => void;
  removeDevice: (deviceId: string) => void;
  updateDevice: (deviceId: string, updates: Partial<Device>) => void;
  selectDevice: (deviceId: string | null) => void;
  
  setShowShortcuts: (show: boolean) => void;
  setQuality: (quality: 'HQ' | 'LQ') => void;
  toggleTheme: () => void;
  
  setTimelineZoom: (zoom: number) => void;
  setTimelineScroll: (scroll: number) => void;
}

// Generate mock patterns
const generateMockPatterns = (): Pattern[] => {
  const patternTypes: AnimationPattern[] = [
    'radialBloom',
    'risingWave',
    'horizontalChase',
    'triangleShape',
    'dualZone',
    'breathe'
  ];
  
  return patternTypes.map((type, i) => ({
    id: `pattern-${i}`,
    name: type.replace(/([A-Z])/g, ' $1').trim().replace(/^./, str => str.toUpperCase()),
    description: `${type} animation pattern with temporal sequencing`,
    thumbnail: '', // Will be generated
    duration: 4000,
    size: 42000 + Math.random() * 10000,
    tags: ['animated', 'lgp', type],
    favorite: i === 0,
    createdAt: new Date(Date.now() - i * 86400000),
    config: PATTERN_PRESETS[type]
  }));
};

// Generate mock devices
const generateMockDevices = (): Device[] => [
  {
    id: 'device-1',
    name: 'K1-Studio',
    ip: '192.168.1.100',
    ledCount: 320,
    storageUsed: 128000,
    storageTotal: 256000,
    status: 'online',
    lastSeen: new Date()
  },
  {
    id: 'device-2',
    name: 'K1-Desktop',
    ip: '192.168.1.101',
    ledCount: 320,
    storageUsed: 64000,
    storageTotal: 256000,
    status: 'online',
    lastSeen: new Date(Date.now() - 30000)
  }
];

// Generate initial timeline tracks
const generateInitialTracks = (): TimelineTrack[] => [
  {
    id: 'track-1',
    name: 'Main Pattern',
    color: '#5B8CFF',
    muted: false,
    solo: false,
    height: 64,
    segments: [
      {
        id: 'segment-1',
        patternId: 'pattern-0',
        startTime: 0,
        duration: 4000,
        color: '#5B8CFF'
      },
      {
        id: 'segment-2',
        patternId: 'pattern-1',
        startTime: 4000,
        duration: 4000,
        color: '#FF7A59'
      }
    ]
  },
  {
    id: 'track-2',
    name: 'Accent',
    color: '#FF7A59',
    muted: false,
    solo: false,
    height: 64,
    segments: [
      {
        id: 'segment-3',
        patternId: 'pattern-2',
        startTime: 2000,
        duration: 3000,
        color: '#FF7A59'
      }
    ]
  }
];

export const useAppState = create<AppState>((set, get) => ({
  // Initial State
  isPlaying: false,
  currentTime: 0,
  playbackSpeed: 1,
  loop: false,
  loopIn: 0,
  loopOut: 10000,
  
  tracks: generateInitialTracks(),
  automationCurves: [],
  selectedSegmentIds: [],
  timelineZoom: 1,
  timelineScroll: 0,
  
  patterns: generateMockPatterns(),
  currentPattern: null,
  
  devices: generateMockDevices(),
  selectedDeviceId: 'device-1',
  
  showShortcuts: false,
  quality: 'HQ',
  theme: 'dark',
  
  // Playback Actions
  setPlaying: (playing) => set({ isPlaying: playing }),
  
  setCurrentTime: (time) => {
    const { loop, loopIn, loopOut } = get();
    let newTime = time;
    
    if (loop && newTime >= loopOut) {
      newTime = loopIn;
    }
    
    set({ currentTime: newTime });
  },
  
  setPlaybackSpeed: (speed) => set({ playbackSpeed: speed }),
  toggleLoop: () => set((state) => ({ loop: !state.loop })),
  setLoopPoints: (inPoint, outPoint) => set({ loopIn: inPoint, loopOut: outPoint }),
  
  // Track Actions
  addTrack: (track) => set((state) => ({
    tracks: [...state.tracks, track]
  })),
  
  removeTrack: (trackId) => set((state) => ({
    tracks: state.tracks.filter(t => t.id !== trackId)
  })),
  
  updateTrack: (trackId, updates) => set((state) => ({
    tracks: state.tracks.map(t => 
      t.id === trackId ? { ...t, ...updates } : t
    )
  })),
  
  // Segment Actions
  addSegment: (trackId, segment) => set((state) => ({
    tracks: state.tracks.map(t =>
      t.id === trackId
        ? { ...t, segments: [...t.segments, segment] }
        : t
    )
  })),
  
  removeSegment: (segmentId) => set((state) => ({
    tracks: state.tracks.map(t => ({
      ...t,
      segments: t.segments.filter(s => s.id !== segmentId)
    })),
    selectedSegmentIds: state.selectedSegmentIds.filter(id => id !== segmentId)
  })),
  
  updateSegment: (segmentId, updates) => set((state) => ({
    tracks: state.tracks.map(t => ({
      ...t,
      segments: t.segments.map(s =>
        s.id === segmentId ? { ...s, ...updates } : s
      )
    }))
  })),
  
  selectSegments: (segmentIds) => set({ selectedSegmentIds: segmentIds }),
  
  // Pattern Actions
  addPattern: (pattern) => set((state) => ({
    patterns: [...state.patterns, pattern]
  })),
  
  removePattern: (patternId) => set((state) => ({
    patterns: state.patterns.filter(p => p.id !== patternId)
  })),
  
  updatePattern: (patternId, updates) => set((state) => ({
    patterns: state.patterns.map(p =>
      p.id === patternId ? { ...p, ...updates } : p
    )
  })),
  
  setCurrentPattern: (pattern) => set({ currentPattern: pattern }),
  
  togglePatternFavorite: (patternId) => set((state) => ({
    patterns: state.patterns.map(p =>
      p.id === patternId ? { ...p, favorite: !p.favorite } : p
    )
  })),
  
  // Device Actions
  addDevice: (device) => set((state) => ({
    devices: [...state.devices, device]
  })),
  
  removeDevice: (deviceId) => set((state) => ({
    devices: state.devices.filter(d => d.id !== deviceId),
    selectedDeviceId: state.selectedDeviceId === deviceId ? null : state.selectedDeviceId
  })),
  
  updateDevice: (deviceId, updates) => set((state) => ({
    devices: state.devices.map(d =>
      d.id === deviceId ? { ...d, ...updates } : d
    )
  })),
  
  selectDevice: (deviceId) => set({ selectedDeviceId: deviceId }),
  
  // UI Actions
  setShowShortcuts: (show) => set({ showShortcuts: show }),
  setQuality: (quality) => set({ quality }),
  toggleTheme: () => set((state) => ({ 
    theme: state.theme === 'dark' ? 'light' : 'dark' 
  })),
  
  setTimelineZoom: (zoom) => set({ timelineZoom: zoom }),
  setTimelineScroll: (scroll) => set({ timelineScroll: scroll }),
}));
