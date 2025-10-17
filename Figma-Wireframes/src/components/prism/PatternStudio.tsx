import { useState, useEffect, useRef } from 'react';
import { K1DeviceAnimated } from './K1DeviceAnimated';
import { AnimationPlayer } from './AnimationPlayer';
import { LivePatternEditor } from './LivePatternEditor';
import { TopBar } from './TopBar';
import { AnimationPattern, AnimationConfig, PATTERN_PRESETS } from './K1AnimationEngine';
import { Button } from '../ui/button';
import { Badge } from '../ui/badge';
import { Download, Save, Share2 } from 'lucide-react';
import { toast } from 'sonner@2.0.3';

export function PatternStudio() {
  const [pattern, setPattern] = useState<AnimationPattern>('radialBloom');
  const [config, setConfig] = useState<AnimationConfig>(PATTERN_PRESETS['radialBloom']);
  const [isPlaying, setIsPlaying] = useState(true);
  const [currentTime, setCurrentTime] = useState(0);
  const animationFrameRef = useRef<number>();
  const lastTimeRef = useRef<number>(Date.now());
  
  const duration = 4000;

  const handlePatternChange = (newPattern: AnimationPattern) => {
    setPattern(newPattern);
    setConfig(PATTERN_PRESETS[newPattern]);
    setCurrentTime(0);
    lastTimeRef.current = Date.now();
  };

  const handleConfigChange = (updates: Partial<AnimationConfig>) => {
    setConfig({ ...config, ...updates });
  };
  
  // Animation loop
  useEffect(() => {
    if (!isPlaying) return;
    
    const animate = () => {
      const now = Date.now();
      const delta = now - lastTimeRef.current;
      lastTimeRef.current = now;
      
      setCurrentTime(prev => (prev + delta) % duration);
      animationFrameRef.current = requestAnimationFrame(animate);
    };
    
    lastTimeRef.current = Date.now();
    animationFrameRef.current = requestAnimationFrame(animate);
    
    return () => {
      if (animationFrameRef.current) {
        cancelAnimationFrame(animationFrameRef.current);
      }
    };
  }, [isPlaying, duration]);

  const handleExport = () => {
    toast.success('Pattern exported to .prism format');
  };

  const handleSave = () => {
    toast.success('Pattern saved to library');
  };

  const handleShare = () => {
    toast.success('Share link copied to clipboard');
  };

  return (
    <div className="h-screen flex flex-col bg-background">
      <TopBar connected={true} fps="Auto" quality="HQ" />
      
      <div className="flex-1 flex overflow-hidden">
        {/* Left Panel - Pattern Editor */}
        <div className="w-96 border-r border-border bg-card/50 backdrop-blur-sm overflow-y-auto">
          <div className="p-4 space-y-4">
            <LivePatternEditor
              pattern={pattern}
              config={config}
              onConfigChange={handleConfigChange}
            />
          </div>
        </div>

        {/* Center - Preview Canvas */}
        <div className="flex-1 flex flex-col">
          {/* Preview Area */}
          <div className="flex-1 bg-[#0D0F14] relative flex items-center justify-center">
            {/* Background Grid */}
            <div className="absolute inset-0 opacity-[0.02]" style={{
              backgroundImage: 'linear-gradient(rgba(255,255,255,0.05) 1px, transparent 1px), linear-gradient(90deg, rgba(255,255,255,0.05) 1px, transparent 1px)',
              backgroundSize: '40px 40px'
            }} />

            {/* K1 Device Preview */}
            <div 
              className="relative"
              style={{ transform: 'scale(0.5)' }}
            >
              <K1DeviceAnimated
                heightVariant="h60"
                finish="dark-frame"
                angle="threeQuarter"
                pattern={pattern}
                config={config}
                isPlaying={isPlaying}
                currentTime={currentTime}
              />
            </div>

            {/* Pattern Info Overlay */}
            <div className="absolute top-4 left-4 glass shadow-elevation-2 border border-border/50 rounded-lg px-4 py-3 text-sm backdrop-blur-xl">
              <div className="font-semibold mb-2">{pattern}</div>
              <div className="space-y-1 text-xs text-muted-foreground">
                <div>Sync: <Badge variant="outline" className="ml-1 text-xs">{config.syncMode}</Badge></div>
                <div>Speed: <Badge variant="outline" className="ml-1 text-xs font-mono">{config.speed}</Badge></div>
                <div>Offset: <Badge variant="outline" className="ml-1 text-xs font-mono">{config.temporalOffset}ms</Badge></div>
              </div>
            </div>

            {/* Device Specs */}
            <div className="absolute top-4 right-4 glass shadow-elevation-2 border border-border/50 rounded-lg px-4 py-3 text-xs backdrop-blur-xl">
              <div className="font-semibold mb-2">K1-Lightwave</div>
              <div className="space-y-1 text-muted-foreground">
                <div>330mm × 60mm</div>
                <div>WS2812B × 320</div>
                <div>Dual-Edge LGP</div>
                <div className="pt-2 border-t border-border text-primary">
                  {Math.round((currentTime / duration) * 60)}fps
                </div>
              </div>
            </div>
          </div>

          {/* Animation Player */}
          <div className="p-4 border-t border-border bg-card/50">
            <AnimationPlayer
              pattern={pattern}
              onPatternChange={handlePatternChange}
              isPlaying={isPlaying}
              onPlayPause={() => setIsPlaying(!isPlaying)}
              currentTime={currentTime}
              onTimeChange={setCurrentTime}
              duration={duration}
              speed={config.speed}
              onSpeedChange={(speed) => handleConfigChange({ speed })}
            />
          </div>
        </div>

        {/* Right Panel - Actions */}
        <div className="w-64 border-l border-border bg-card/50 backdrop-blur-sm p-4 space-y-4">
          <div>
            <h3 className="font-semibold mb-3">Export</h3>
            <div className="space-y-2">
              <Button 
                className="w-full gap-2 hover-lift glow-primary" 
                onClick={handleExport}
              >
                <Download className="w-4 h-4" />
                Export .prism
              </Button>
              <Button 
                variant="outline" 
                className="w-full gap-2 hover-lift"
                onClick={handleSave}
              >
                <Save className="w-4 h-4" />
                Save to Library
              </Button>
              <Button 
                variant="outline" 
                className="w-full gap-2 hover-lift"
                onClick={handleShare}
              >
                <Share2 className="w-4 h-4" />
                Share Link
              </Button>
            </div>
          </div>

          <div className="pt-4 border-t border-border">
            <h3 className="font-semibold mb-3">Pattern Info</h3>
            <div className="space-y-3 text-sm">
              <div>
                <div className="text-muted-foreground mb-1">CH1 Colors</div>
                <div className="flex gap-1">
                  {config.ch1Colors.map((color, i) => (
                    <div
                      key={i}
                      className="w-8 h-8 rounded border border-border shadow-sm"
                      style={{ backgroundColor: color }}
                      title={color}
                    />
                  ))}
                </div>
              </div>
              <div>
                <div className="text-muted-foreground mb-1">CH2 Colors</div>
                <div className="flex gap-1">
                  {config.ch2Colors.map((color, i) => (
                    <div
                      key={i}
                      className="w-8 h-8 rounded border border-border shadow-sm"
                      style={{ backgroundColor: color }}
                      title={color}
                    />
                  ))}
                </div>
              </div>
            </div>
          </div>

          <div className="pt-4 border-t border-border">
            <h3 className="font-semibold mb-3">Estimated Size</h3>
            <div className="space-y-2 text-sm">
              <div className="flex justify-between">
                <span className="text-muted-foreground">Duration:</span>
                <span className="font-mono">{(duration / 1000).toFixed(1)}s</span>
              </div>
              <div className="flex justify-between">
                <span className="text-muted-foreground">FPS:</span>
                <span className="font-mono">60</span>
              </div>
              <div className="flex justify-between">
                <span className="text-muted-foreground">Frames:</span>
                <span className="font-mono">{Math.floor((duration / 1000) * 60)}</span>
              </div>
              <div className="flex justify-between pt-2 border-t border-border font-semibold">
                <span className="text-muted-foreground">Est. Size:</span>
                <span className="font-mono">~42 KB</span>
              </div>
            </div>
          </div>
        </div>
      </div>
    </div>
  );
}
