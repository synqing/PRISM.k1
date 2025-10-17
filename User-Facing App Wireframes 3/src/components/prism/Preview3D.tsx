import { useState } from "react";
import { Toggle } from "../ui/toggle";
import { K1DeviceAnimated } from "./K1DeviceAnimated";
import { AnimationPlayer } from "./AnimationPlayer";
import { AnimationPattern } from "./K1AnimationEngine";
import { ChevronDown, ChevronUp } from "lucide-react";
import { Button } from "../ui/button";

interface Preview3DProps {
  quality?: 'HQ' | 'LQ';
  onQualityToggle?: () => void;
}

export function Preview3D({ quality = 'HQ', onQualityToggle }: Preview3DProps) {
  const [pattern, setPattern] = useState<AnimationPattern>('radialBloom');
  const [viewAngle, setViewAngle] = useState<'front' | 'threeQuarter'>('threeQuarter');
  const [isPlaying, setIsPlaying] = useState(true);
  const [currentTime, setCurrentTime] = useState(0);
  const [speed, setSpeed] = useState(100);
  const [showControls, setShowControls] = useState(true);
  
  const duration = 4000; // 4 second loop

  const handleTimeUpdate = (time: number) => {
    if (isPlaying) {
      setCurrentTime(time % duration);
    }
  };

  return (
    <div className="h-full bg-[#0D0F14] relative flex flex-col overflow-hidden">
      {/* Subtle Background Grid */}
      <div className="absolute inset-0 opacity-[0.02]" style={{
        backgroundImage: 'linear-gradient(rgba(255,255,255,0.05) 1px, transparent 1px), linear-gradient(90deg, rgba(255,255,255,0.05) 1px, transparent 1px)',
        backgroundSize: '40px 40px'
      }} />

      {/* Controls Bar */}
      <div className="absolute top-4 left-4 right-4 z-20 flex items-center justify-between">
        <div className="flex items-center gap-3">
          <Button
            variant="outline"
            size="sm"
            onClick={() => setViewAngle(viewAngle === 'front' ? 'threeQuarter' : 'front')}
            className="glass shadow-elevation-1 hover-lift h-9 px-3"
          >
            {viewAngle === 'front' ? 'Front View' : '3/4 View'}
          </Button>
          
          <Button
            variant="outline"
            size="sm"
            onClick={() => setShowControls(!showControls)}
            className="glass shadow-elevation-1 hover-lift h-9 w-9 p-0"
          >
            {showControls ? <ChevronUp className="w-4 h-4" /> : <ChevronDown className="w-4 h-4" />}
          </Button>
        </div>

        <Toggle 
          pressed={quality === 'HQ'} 
          onClick={onQualityToggle} 
          className="glass shadow-elevation-1 border-border/50 font-mono hover-lift active-press data-[state=on]:glow-primary h-9"
        >
          {quality}
        </Toggle>
      </div>

      {/* K1 Device Viewport */}
      <div className="flex-1 flex items-center justify-center p-8">
        <div 
          className="relative"
          style={{
            // Scale down to fit viewport while maintaining 1px = 0.25mm
            transform: 'scale(0.42)',
          }}
        >
          <K1DeviceAnimated
            heightVariant="h60"
            finish="dark-frame"
            angle={viewAngle}
            pattern={pattern}
            config={{ speed }}
            isPlaying={isPlaying}
            currentTime={currentTime}
            onTimeUpdate={handleTimeUpdate}
          />
        </div>
      </div>
      
      {/* Animation Player */}
      {showControls && (
        <div className="absolute bottom-4 left-4 right-4 z-20">
          <AnimationPlayer
            pattern={pattern}
            onPatternChange={setPattern}
            isPlaying={isPlaying}
            onPlayPause={() => setIsPlaying(!isPlaying)}
            currentTime={currentTime}
            onTimeChange={setCurrentTime}
            duration={duration}
            speed={speed}
            onSpeedChange={setSpeed}
          />
        </div>
      )}

      {/* Environment Elements for 3/4 view */}
      {viewAngle === 'threeQuarter' && !showControls && (
        <>
          {/* Desk Surface Hint */}
          <div
            className="absolute inset-x-0 bottom-0 h-1/4 pointer-events-none"
            style={{
              background: 'linear-gradient(to top, rgba(13,15,20,0.6) 0%, transparent 100%)',
            }}
          />
        </>
      )}

      {/* Stats Overlay - Only show when controls hidden */}
      {!showControls && (
        <>
          <div className="absolute bottom-4 right-4 glass shadow-elevation-2 border border-border/50 rounded-lg px-4 py-3 text-xs font-mono space-y-2 backdrop-blur-xl z-10">
            <div className="flex justify-between gap-6">
              <span className="text-muted-foreground">FPS:</span>
              <span className="text-primary font-semibold">60</span>
            </div>
            <div className="flex justify-between gap-6">
              <span className="text-muted-foreground">LEDs:</span>
              <span className="font-semibold">320</span>
            </div>
            <div className="flex justify-between gap-6">
              <span className="text-muted-foreground">Frame:</span>
              <span className="font-semibold">{Math.floor((currentTime / duration) * 240)}/240</span>
            </div>
            <div className="flex justify-between gap-6">
              <span className="text-muted-foreground">Pattern:</span>
              <span className="font-semibold text-xs">{pattern}</span>
            </div>
          </div>

          {/* Device Info Overlay */}
          <div className="absolute bottom-4 left-4 glass shadow-elevation-2 border border-border/50 rounded-lg px-4 py-3 text-xs space-y-1 backdrop-blur-xl z-10">
            <div className="font-semibold mb-2">K1-Lightwave</div>
            <div className="text-muted-foreground">330mm × 60mm</div>
            <div className="text-muted-foreground">Dual-Edge LGP</div>
            <div className="text-muted-foreground">WS2812B × 320</div>
          </div>
        </>
      )}
    </div>
  );
}
