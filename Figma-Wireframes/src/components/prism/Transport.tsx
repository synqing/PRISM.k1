import { Button } from "../ui/button";
import { Input } from "../ui/input";
import { Toggle } from "../ui/toggle";
import { Separator } from "../ui/separator";
import { Slider } from "../ui/slider";
import { useAppState } from "../../lib/appState";
import { 
  Play, 
  Pause, 
  Square, 
  Repeat, 
  ZoomIn, 
  ZoomOut, 
  Maximize2,
  Grid3x3,
  Keyboard
} from "lucide-react";
import { toast } from "sonner@2.0.3";

interface TransportProps {
  onShowShortcuts?: () => void;
}

export function Transport({ onShowShortcuts }: TransportProps) {
  const { 
    isPlaying, 
    setPlaying, 
    currentTime, 
    setCurrentTime,
    loop, 
    toggleLoop,
    loopOut,
    timelineZoom,
    setTimelineZoom
  } = useAppState();
  
  const formatTime = (ms: number) => {
    const totalSeconds = ms / 1000;
    const minutes = Math.floor(totalSeconds / 60);
    const seconds = (totalSeconds % 60).toFixed(1);
    return `${minutes.toString().padStart(2, '0')}:${seconds.padStart(4, '0')}`;
  };
  
  const handleStop = () => {
    setPlaying(false);
    setCurrentTime(0);
    toast.success('Stopped');
  };
  
  const handleZoomChange = (value: number[]) => {
    setTimelineZoom(value[0] / 50); // Convert slider 0-100 to zoom 0-2
  };
  return (
    <div className="h-14 border-t border-border glass backdrop-blur-xl px-4 flex items-center gap-4 relative z-10">
      {/* Playback Controls */}
      <div className="flex items-center gap-1.5">
        <Button
          variant={isPlaying ? "default" : "ghost"}
          size="sm"
          className={`h-9 w-9 p-0 hover-lift active-press ${isPlaying ? 'glow-primary' : ''}`}
          onClick={() => setPlaying(!isPlaying)}
        >
          {isPlaying ? <Pause className="w-4 h-4" /> : <Play className="w-4 h-4 ml-0.5" />}
        </Button>
        <Button 
          variant="ghost" 
          size="sm" 
          className="h-9 w-9 p-0 hover-lift active-press"
          onClick={handleStop}
        >
          <Square className="w-3.5 h-3.5" />
        </Button>
        <Toggle 
          pressed={loop} 
          onClick={toggleLoop}
          className="h-9 w-9 p-0 hover-lift data-[state=on]:bg-primary/10 data-[state=on]:text-primary"
        >
          <Repeat className="w-4 h-4" />
        </Toggle>
      </div>

      <Separator orientation="vertical" className="h-6" />

      {/* Timecode */}
      <div className="flex items-center gap-2.5 px-2 py-1.5 rounded-lg bg-muted/50">
        <Input
          value={formatTime(currentTime)}
          className="w-20 h-7 text-center font-mono border-0 bg-transparent focus-visible:ring-1 focus-visible:ring-primary"
          readOnly
        />
        <span className="text-muted-foreground font-mono text-sm">/</span>
        <span className="font-mono text-sm text-muted-foreground">{formatTime(loopOut)}</span>
      </div>

      <Separator orientation="vertical" className="h-6" />

      {/* Grid Snap */}
      <Toggle 
        defaultPressed={true}
        className="h-8 px-3 gap-2 hover-lift data-[state=on]:bg-primary/10 data-[state=on]:text-primary data-[state=on]:border-primary/30"
      >
        <Grid3x3 className="w-3.5 h-3.5" />
        <span className="text-sm">Snap</span>
      </Toggle>

      {/* BPM */}
      <div className="flex items-center gap-2 px-2 py-1.5 rounded-lg bg-muted/50">
        <span className="text-xs text-muted-foreground uppercase tracking-wider">BPM</span>
        <Input
          type="number"
          value="120"
          className="w-14 h-7 text-center font-mono border-0 bg-transparent focus-visible:ring-1 focus-visible:ring-primary"
        />
      </div>

      <div className="flex-1" />

      {/* Zoom Controls */}
      <div className="flex items-center gap-2 px-2 py-1.5 rounded-lg bg-muted/50">
        <Button 
          variant="ghost" 
          size="sm" 
          className="h-7 w-7 p-0 hover-lift active-press"
          onClick={() => setTimelineZoom(Math.max(0.1, timelineZoom - 0.1))}
        >
          <ZoomOut className="w-3.5 h-3.5" />
        </Button>
        <Slider
          value={[timelineZoom * 50]}
          onValueChange={handleZoomChange}
          max={100}
          step={1}
          className="w-28"
        />
        <Button 
          variant="ghost" 
          size="sm" 
          className="h-7 w-7 p-0 hover-lift active-press"
          onClick={() => setTimelineZoom(Math.min(2, timelineZoom + 0.1))}
        >
          <ZoomIn className="w-3.5 h-3.5" />
        </Button>
        <Separator orientation="vertical" className="h-4 mx-1" />
        <Button variant="ghost" size="sm" className="h-7 w-7 p-0 hover-lift active-press">
          <Maximize2 className="w-3.5 h-3.5" />
        </Button>
      </div>

      <Separator orientation="vertical" className="h-6" />

      {/* Shortcuts */}
      <Button 
        variant="ghost" 
        size="sm" 
        className="h-8 gap-2 hover-lift active-press"
        onClick={onShowShortcuts}
      >
        <Keyboard className="w-3.5 h-3.5" />
        <span className="text-sm">Shortcuts</span>
      </Button>
    </div>
  );
}
