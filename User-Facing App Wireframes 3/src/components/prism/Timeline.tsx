import { useAppState } from "../../lib/appState";
import { Button } from "../ui/button";
import { Volume2, VolumeX, Music, Plus } from "lucide-react";
import { toast } from "sonner@2.0.3";

export function Timeline() {
  const { 
    tracks, 
    currentTime, 
    setCurrentTime, 
    selectedSegmentIds, 
    selectSegments,
    patterns,
    isPlaying,
    addTrack,
    updateTrack
  } = useAppState();
  
  const totalDuration = 10000; // 10 seconds in ms
  const playheadPosition = (currentTime / totalDuration) * 100;
  
  const handleAddTrack = () => {
    const newTrack = {
      id: `track-${Date.now()}`,
      name: `Track ${tracks.length + 1}`,
      color: tracks.length % 2 === 0 ? '#5B8CFF' : '#FF7A59',
      muted: false,
      solo: false,
      height: 64,
      segments: []
    };
    addTrack(newTrack);
    toast.success(`Added ${newTrack.name}`);
  };
  
  const handleToggleMute = (trackId: string) => {
    const track = tracks.find(t => t.id === trackId);
    if (track) {
      updateTrack(trackId, { muted: !track.muted });
      toast.info(`${track.name} ${!track.muted ? 'muted' : 'unmuted'}`);
    }
  };
  
  const handleToggleSolo = (trackId: string) => {
    const track = tracks.find(t => t.id === trackId);
    if (track) {
      updateTrack(trackId, { solo: !track.solo });
      toast.info(`${track.name} solo ${!track.solo ? 'enabled' : 'disabled'}`);
    }
  };

  const msToPercent = (ms: number) => {
    return (ms / totalDuration) * 100;
  };
  
  const handleTimelineClick = (e: React.MouseEvent<HTMLDivElement>) => {
    const rect = e.currentTarget.getBoundingClientRect();
    const x = e.clientX - rect.left - 128; // Subtract track label width
    const percent = (x / (rect.width - 128)) * 100;
    const newTime = Math.max(0, Math.min(totalDuration, (percent / 100) * totalDuration));
    setCurrentTime(newTime);
  };
  
  const handleSegmentClick = (segmentId: string, e: React.MouseEvent) => {
    e.stopPropagation();
    if (e.metaKey || e.ctrlKey) {
      // Multi-select
      if (selectedSegmentIds.includes(segmentId)) {
        selectSegments(selectedSegmentIds.filter(id => id !== segmentId));
      } else {
        selectSegments([...selectedSegmentIds, segmentId]);
      }
    } else {
      // Single select
      selectSegments([segmentId]);
    }
  };
  
  const getPatternName = (patternId: string) => {
    const pattern = patterns.find(p => p.id === patternId);
    return pattern?.name || 'Unknown Pattern';
  };

  return (
    <div className="h-full bg-background flex flex-col relative">
      {/* Timeline Header */}
      <div className="h-10 border-b border-border bg-muted/30 backdrop-blur-sm flex relative z-10">
        <div className="w-48 border-r border-border flex items-center justify-between px-3 bg-card/50">
          <span className="text-xs font-medium text-muted-foreground uppercase tracking-wide">Tracks</span>
          <Button
            variant="ghost"
            size="sm"
            className="h-6 w-6 p-0 hover-lift"
            onClick={handleAddTrack}
          >
            <Plus className="w-3.5 h-3.5" />
          </Button>
        </div>
        <div className="flex-1 relative cursor-pointer" onClick={handleTimelineClick}>
          {/* Time Ruler */}
          <div className="absolute inset-0 flex">
            {Array.from({ length: 11 }).map((_, i) => (
              <div key={i} className="flex-1 border-l border-border/40 relative">
                <span className="absolute left-2 top-2 text-xs text-muted-foreground font-mono">
                  {i}s
                </span>
              </div>
            ))}
          </div>
        </div>
      </div>

      {/* Timeline Tracks */}
      <div className="flex-1 overflow-auto">
        {tracks.map((track, trackIndex) => (
          <div 
            key={track.id} 
            className={`h-20 border-b border-border flex hover:bg-accent/5 transition-colors group ${
              track.muted ? 'opacity-50' : ''
            }`}
            style={{ backgroundColor: trackIndex % 2 === 0 ? 'transparent' : 'rgba(0,0,0,0.02)' }}
          >
            {/* Track Label & Controls */}
            <div className="w-48 border-r border-border flex items-center gap-2 px-3 bg-card/30">
              <div className="flex flex-col gap-1">
                <Button
                  variant="ghost"
                  size="sm"
                  className={`h-6 w-6 p-0 hover-lift ${track.solo ? 'bg-accent text-accent-foreground' : ''}`}
                  onClick={() => handleToggleSolo(track.id)}
                  title="Solo"
                >
                  <Music className="w-3 h-3" />
                </Button>
                <Button
                  variant="ghost"
                  size="sm"
                  className={`h-6 w-6 p-0 hover-lift ${track.muted ? 'text-destructive' : ''}`}
                  onClick={() => handleToggleMute(track.id)}
                  title="Mute"
                >
                  {track.muted ? <VolumeX className="w-3 h-3" /> : <Volume2 className="w-3 h-3" />}
                </Button>
              </div>
              <div className="flex-1 space-y-1">
                <span className="text-sm font-medium block truncate">{track.name}</span>
                <div 
                  className="h-1 rounded-full"
                  style={{ backgroundColor: track.color, width: '100%', opacity: 0.5 }}
                />
              </div>
            </div>

            {/* Track Content */}
            <div className="flex-1 relative p-2" onClick={handleTimelineClick}>
              {/* Grid Lines */}
              <div className="absolute inset-0 flex pointer-events-none">
                {Array.from({ length: 11 }).map((_, i) => (
                  <div key={i} className="flex-1 border-l border-border/20" />
                ))}
              </div>

              {/* Segments */}
              {track.segments.map((segment) => {
                const isSelected = selectedSegmentIds.includes(segment.id);
                return (
                  <div
                    key={segment.id}
                    onClick={(e) => handleSegmentClick(segment.id, e)}
                    className={`absolute top-3 bottom-3 rounded-lg px-3 flex items-center cursor-pointer transition-all duration-200 overflow-hidden group/clip ${
                      isSelected
                        ? 'bg-gradient-to-r from-primary to-primary/90 text-primary-foreground ring-2 ring-primary ring-offset-2 ring-offset-background shadow-elevation-2 glow-primary'
                        : 'bg-gradient-to-br from-muted to-muted/70 hover:from-accent/30 hover:to-accent/20 border border-border hover:border-primary/50 shadow-sm'
                    }`}
                    style={{
                      left: `${msToPercent(segment.startTime)}%`,
                      width: `${msToPercent(segment.duration)}%`,
                      borderLeft: `3px solid ${segment.color}`
                    }}
                  >
                    {/* Clip Background Pattern */}
                    <div className="absolute inset-0 opacity-10" style={{
                      backgroundImage: 'repeating-linear-gradient(90deg, transparent, transparent 10px, currentColor 10px, currentColor 11px)'
                    }} />
                    
                    <span className="text-sm font-medium truncate relative z-10">
                      {getPatternName(segment.patternId)}
                    </span>
                    
                    {/* Edge Handles */}
                    {isSelected && (
                      <>
                        <div className="absolute left-0 top-0 bottom-0 w-1.5 bg-primary-foreground/40 cursor-ew-resize hover:bg-primary-foreground/60 transition-colors" />
                        <div className="absolute right-0 top-0 bottom-0 w-1.5 bg-primary-foreground/40 cursor-ew-resize hover:bg-primary-foreground/60 transition-colors" />
                      </>
                    )}
                    
                    {/* Hover indicator */}
                    <div className="absolute inset-0 bg-white/5 opacity-0 group-hover/clip:opacity-100 transition-opacity pointer-events-none" />
                  </div>
                );
              })}

              {/* Automation Curve */}
              {track.name === 'Automation' && track.segments.length === 0 && (
                <svg className="absolute inset-0 pointer-events-none" style={{ height: '100%', width: '100%' }}>
                  <defs>
                    <linearGradient id="curveGradient" x1="0%" y1="0%" x2="100%" y2="0%">
                      <stop offset="0%" stopColor="var(--prism-brand-500)" stopOpacity="0.6" />
                      <stop offset="50%" stopColor="var(--prism-accent-500)" stopOpacity="0.6" />
                      <stop offset="100%" stopColor="var(--prism-brand-500)" stopOpacity="0.6" />
                    </linearGradient>
                  </defs>
                  
                  <path
                    d="M 0,50 Q 150,20 300,50 T 600,50 L 100%,30"
                    stroke="url(#curveGradient)"
                    strokeWidth="2.5"
                    fill="none"
                    strokeLinecap="round"
                  />
                  
                  {/* Keyframe dots with glow */}
                  {[
                    { cx: 0, cy: 50 },
                    { cx: 300, cy: 50 },
                    { cx: 600, cy: 50 }
                  ].map((point, i) => (
                    <g key={i}>
                      <circle 
                        cx={point.cx} 
                        cy={point.cy} 
                        r="6" 
                        className="fill-primary/20"
                      />
                      <circle 
                        cx={point.cx} 
                        cy={point.cy} 
                        r="4" 
                        className="fill-primary cursor-pointer hover:r-5 transition-all"
                      />
                    </g>
                  ))}
                </svg>
              )}
            </div>
          </div>
        ))}
      </div>

      {/* Playhead with glow */}
      <div
        className="absolute top-10 bottom-0 w-0.5 bg-accent pointer-events-none z-20 shadow-[0_0_8px_rgba(255,122,89,0.6)]"
        style={{ left: `calc(12rem + ${playheadPosition}%)` }}
      >
        <div className="absolute top-0 left-1/2 -translate-x-1/2 -translate-y-full">
          <div className="w-0 h-0 border-l-[6px] border-r-[6px] border-t-[6px] border-transparent border-t-accent shadow-elevation-1" />
        </div>
      </div>
    </div>
  );
}
