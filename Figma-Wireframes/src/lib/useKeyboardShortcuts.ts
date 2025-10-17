/**
 * Keyboard Shortcuts Hook
 * Video editor-style shortcuts (Space, J/K/L, I/O, etc.)
 */

import { useEffect } from 'react';
import { useAppState } from './appState';
import { toast } from 'sonner@2.0.3';

export function useKeyboardShortcuts() {
  const {
    isPlaying,
    setPlaying,
    currentTime,
    setCurrentTime,
    playbackSpeed,
    setPlaybackSpeed,
    setLoopPoints,
    loopIn,
    loopOut,
    toggleLoop,
    setShowShortcuts,
    selectedSegmentIds,
    removeSegment,
  } = useAppState();

  useEffect(() => {
    const handleKeyDown = (e: KeyboardEvent) => {
      // Ignore if typing in input/textarea
      const target = e.target as HTMLElement;
      if (
        target.tagName === 'INPUT' ||
        target.tagName === 'TEXTAREA' ||
        target.isContentEditable
      ) {
        return;
      }

      // Prevent default for handled shortcuts
      const preventDefault = () => {
        e.preventDefault();
        e.stopPropagation();
      };

      switch (e.key.toLowerCase()) {
        // SPACE - Play/Pause
        case ' ':
          preventDefault();
          setPlaying(!isPlaying);
          toast.success(isPlaying ? 'Paused' : 'Playing');
          break;

        // J - Reverse (slow)
        case 'j':
          preventDefault();
          setPlaybackSpeed(-1);
          setPlaying(true);
          toast.success('Reverse playback');
          break;

        // K - Pause
        case 'k':
          preventDefault();
          setPlaying(false);
          setPlaybackSpeed(1);
          toast.success('Paused');
          break;

        // L - Forward (fast)
        case 'l':
          preventDefault();
          if (playbackSpeed === 1) {
            setPlaybackSpeed(2);
            toast.success('2x speed');
          } else if (playbackSpeed === 2) {
            setPlaybackSpeed(4);
            toast.success('4x speed');
          } else {
            setPlaybackSpeed(1);
            toast.success('Normal speed');
          }
          setPlaying(true);
          break;

        // I - Set In Point
        case 'i':
          preventDefault();
          setLoopPoints(currentTime, loopOut);
          toast.success(`In point: ${(currentTime / 1000).toFixed(2)}s`);
          break;

        // O - Set Out Point
        case 'o':
          preventDefault();
          setLoopPoints(loopIn, currentTime);
          toast.success(`Out point: ${(currentTime / 1000).toFixed(2)}s`);
          break;

        // Left Arrow - Step back 1 frame (16.67ms @ 60fps)
        case 'arrowleft':
          preventDefault();
          setCurrentTime(Math.max(0, currentTime - 16.67));
          break;

        // Right Arrow - Step forward 1 frame
        case 'arrowright':
          preventDefault();
          setCurrentTime(currentTime + 16.67);
          break;

        // Shift + Left - Jump back 1 second
        case 'arrowleft':
          if (e.shiftKey) {
            preventDefault();
            setCurrentTime(Math.max(0, currentTime - 1000));
          }
          break;

        // Shift + Right - Jump forward 1 second
        case 'arrowright':
          if (e.shiftKey) {
            preventDefault();
            setCurrentTime(currentTime + 1000);
          }
          break;

        // Home - Jump to start
        case 'home':
          preventDefault();
          setCurrentTime(0);
          toast.success('Jumped to start');
          break;

        // End - Jump to end
        case 'end':
          preventDefault();
          setCurrentTime(loopOut);
          toast.success('Jumped to end');
          break;

        // M - Toggle loop
        case 'm':
          preventDefault();
          toggleLoop();
          toast.success('Loop toggled');
          break;

        // Delete/Backspace - Delete selected segments
        case 'delete':
        case 'backspace':
          if (selectedSegmentIds.length > 0) {
            preventDefault();
            selectedSegmentIds.forEach(id => removeSegment(id));
            toast.success(`Deleted ${selectedSegmentIds.length} segment(s)`);
          }
          break;

        // ? - Show shortcuts
        case '?':
          if (e.shiftKey) {
            preventDefault();
            setShowShortcuts(true);
          }
          break;

        // Escape - Hide shortcuts
        case 'escape':
          preventDefault();
          setShowShortcuts(false);
          break;

        // Cmd/Ctrl + S - Save (mock)
        case 's':
          if (e.metaKey || e.ctrlKey) {
            preventDefault();
            toast.success('Pattern saved to library');
          }
          break;

        // Cmd/Ctrl + E - Export (mock)
        case 'e':
          if (e.metaKey || e.ctrlKey) {
            preventDefault();
            toast.success('Exported to .prism format');
          }
          break;

        // Cmd/Ctrl + D - Duplicate selected (mock)
        case 'd':
          if (e.metaKey || e.ctrlKey) {
            preventDefault();
            if (selectedSegmentIds.length > 0) {
              toast.success('Segment duplicated');
            }
          }
          break;

        default:
          break;
      }
    };

    window.addEventListener('keydown', handleKeyDown);
    return () => window.removeEventListener('keydown', handleKeyDown);
  }, [
    isPlaying,
    setPlaying,
    currentTime,
    setCurrentTime,
    playbackSpeed,
    setPlaybackSpeed,
    setLoopPoints,
    loopIn,
    loopOut,
    toggleLoop,
    setShowShortcuts,
    selectedSegmentIds,
    removeSegment,
  ]);
}
