import { useState, useEffect } from 'react';
import { Button } from '../ui/button';
import { Slider } from '../ui/slider';
import { Select, SelectContent, SelectItem, SelectTrigger, SelectValue } from '../ui/select';
import { Badge } from '../ui/badge';
import { 
  Play, 
  Pause, 
  RotateCcw, 
  SkipBack,
  SkipForward,
  Settings
} from 'lucide-react';
import { AnimationPattern, PATTERN_PRESETS } from './K1AnimationEngine';

interface AnimationPlayerProps {
  pattern: AnimationPattern;
  onPatternChange: (pattern: AnimationPattern) => void;
  isPlaying: boolean;
  onPlayPause: () => void;
  currentTime: number;
  onTimeChange: (time: number) => void;
  duration: number; // Total loop duration in ms
  speed: number;
  onSpeedChange: (speed: number) => void;
}

export function AnimationPlayer({
  pattern,
  onPatternChange,
  isPlaying,
  onPlayPause,
  currentTime,
  onTimeChange,
  duration,
  speed,
  onSpeedChange,
}: AnimationPlayerProps) {
  const [isScrubbing, setIsScrubbing] = useState(false);
  
  const formatTime = (ms: number) => {
    const seconds = Math.floor(ms / 1000);
    const milliseconds = Math.floor((ms % 1000) / 10);
    return `${seconds}.${milliseconds.toString().padStart(2, '0')}s`;
  };

  const handleTimelineChange = (value: number[]) => {
    setIsScrubbing(true);
    onTimeChange(value[0]);
  };

  const handleTimelineCommit = () => {
    setIsScrubbing(false);
  };

  const handleReset = () => {
    onTimeChange(0);
  };

  const handleSkipBack = () => {
    onTimeChange(Math.max(0, currentTime - 500));
  };

  const handleSkipForward = () => {
    onTimeChange(Math.min(duration, currentTime + 500));
  };

  const patterns: { value: AnimationPattern; label: string; description: string }[] = [
    { 
      value: 'radialBloom', 
      label: 'Radial Bloom', 
      description: 'Center origin, synchronized'
    },
    { 
      value: 'risingWave', 
      label: 'Rising Wave', 
      description: 'Temporal offset, L→R sweep'
    },
    { 
      value: 'horizontalChase', 
      label: 'Horizontal Chase', 
      description: 'Sequential lobes'
    },
    { 
      value: 'triangleShape', 
      label: 'Triangle Shape', 
      description: 'Progressive delay'
    },
    { 
      value: 'dualZone', 
      label: 'Dual Zone', 
      description: 'Independent patterns'
    },
    { 
      value: 'breathe', 
      label: 'Breathe', 
      description: 'Synchronized pulse'
    },
  ];

  const currentPattern = patterns.find(p => p.value === pattern);
  const config = PATTERN_PRESETS[pattern];

  return (
    <div className="w-full glass shadow-elevation-2 border border-border/50 rounded-lg p-4 space-y-4 backdrop-blur-xl">
      {/* Header */}
      <div className="flex items-center justify-between">
        <div className="flex items-center gap-3">
          <div className="w-10 h-10 rounded-lg bg-primary/10 flex items-center justify-center">
            <Settings className="w-5 h-5 text-primary" />
          </div>
          <div>
            <h3 className="font-semibold">Animation Player</h3>
            <p className="text-xs text-muted-foreground">Timeline-based LGP control</p>
          </div>
        </div>
        
        <div className="flex items-center gap-2">
          <Badge variant="outline" className="font-mono">
            {formatTime(currentTime)} / {formatTime(duration)}
          </Badge>
          <Badge variant="secondary">
            {Math.round((currentTime / duration) * 100)}%
          </Badge>
        </div>
      </div>

      {/* Pattern Selector */}
      <div className="grid grid-cols-2 gap-4">
        <div className="space-y-2">
          <label className="text-xs font-medium text-muted-foreground uppercase tracking-wider">
            Pattern
          </label>
          <Select value={pattern} onValueChange={(v) => onPatternChange(v as AnimationPattern)}>
            <SelectTrigger className="hover-lift">
              <SelectValue />
            </SelectTrigger>
            <SelectContent className="glass shadow-elevation-2">
              {patterns.map((p) => (
                <SelectItem key={p.value} value={p.value}>
                  <div>
                    <div className="font-medium">{p.label}</div>
                    <div className="text-xs text-muted-foreground">{p.description}</div>
                  </div>
                </SelectItem>
              ))}
            </SelectContent>
          </Select>
        </div>

        <div className="space-y-2">
          <label className="text-xs font-medium text-muted-foreground uppercase tracking-wider">
            Speed
          </label>
          <div className="flex items-center gap-3">
            <Slider
              value={[speed]}
              onValueChange={(v) => onSpeedChange(v[0])}
              min={50}
              max={255}
              step={1}
              className="flex-1"
            />
            <span className="text-sm font-mono w-12 text-right">{speed}</span>
          </div>
        </div>
      </div>

      {/* Timeline Scrubber */}
      <div className="space-y-2">
        <label className="text-xs font-medium text-muted-foreground uppercase tracking-wider">
          Timeline
        </label>
        <div className="relative">
          <Slider
            value={[currentTime]}
            onValueChange={handleTimelineChange}
            onValueCommit={handleTimelineCommit}
            min={0}
            max={duration}
            step={1}
            className="w-full"
          />
          {/* Loop markers */}
          <div className="absolute -bottom-4 left-0 right-0 flex justify-between text-xs text-muted-foreground font-mono">
            <span>0s</span>
            <span>{formatTime(duration)}</span>
          </div>
        </div>
      </div>

      {/* Transport Controls */}
      <div className="flex items-center justify-between pt-2 border-t border-border">
        <div className="flex items-center gap-1.5">
          <Button
            variant="outline"
            size="sm"
            onClick={handleReset}
            className="h-9 w-9 p-0 hover-lift active-press"
          >
            <RotateCcw className="w-4 h-4" />
          </Button>
          <Button
            variant="outline"
            size="sm"
            onClick={handleSkipBack}
            className="h-9 w-9 p-0 hover-lift active-press"
          >
            <SkipBack className="w-4 h-4" />
          </Button>
          <Button
            variant={isPlaying ? "default" : "outline"}
            size="sm"
            onClick={onPlayPause}
            className={`h-9 w-9 p-0 hover-lift active-press ${isPlaying ? 'glow-primary' : ''}`}
          >
            {isPlaying ? <Pause className="w-4 h-4" /> : <Play className="w-4 h-4 ml-0.5" />}
          </Button>
          <Button
            variant="outline"
            size="sm"
            onClick={handleSkipForward}
            className="h-9 w-9 p-0 hover-lift active-press"
          >
            <SkipForward className="w-4 h-4" />
          </Button>
        </div>

        {/* Pattern Info */}
        <div className="flex items-center gap-4 text-xs text-muted-foreground">
          <div className="flex items-center gap-2">
            <span>Sync:</span>
            <Badge variant="outline" className="text-xs">
              {config.syncMode}
            </Badge>
          </div>
          <div className="flex items-center gap-2">
            <span>Offset:</span>
            <Badge variant="outline" className="text-xs font-mono">
              {config.temporalOffset}ms
            </Badge>
          </div>
          <div className="flex items-center gap-2">
            <span>Origin:</span>
            <Badge variant="outline" className="text-xs">
              {config.motionOrigin}
            </Badge>
          </div>
        </div>
      </div>
    </div>
  );
}
