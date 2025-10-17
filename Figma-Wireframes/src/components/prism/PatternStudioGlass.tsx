import { useState, useEffect, useRef } from 'react';
import { K1DeviceAnimated } from './K1DeviceAnimated';
import { AnimationPlayer } from './AnimationPlayer';
import { TopBar } from './TopBar';
import { 
  AnimationPattern, 
  AnimationConfig, 
  MotionDirection, 
  SyncMode,
  WaveformType,
  PATTERN_PRESETS 
} from './K1AnimationEngine';
import { Button } from '../ui/button';
import { Badge } from '../ui/badge';
import { Card } from '../ui/card';
import { Label } from '../ui/label';
import { Slider } from '../ui/slider';
import { Input } from '../ui/input';
import { Switch } from '../ui/switch';
import { Tabs, TabsContent, TabsList, TabsTrigger } from '../ui/tabs';
import { 
  Download, 
  Save, 
  Share2, 
  Palette,
  ArrowLeft,
  ArrowRight,
  CircleDot,
  Minimize2,
  Minus,
  Zap,
  Waves,
  Settings2,
  Sparkles,
  Plus,
  X
} from 'lucide-react';
import { toast } from 'sonner@2.0.3';

export function PatternStudioGlass() {
  const [pattern, setPattern] = useState<AnimationPattern>('radialBloom');
  const [config, setConfig] = useState<AnimationConfig>(PATTERN_PRESETS['radialBloom']);
  const [isPlaying, setIsPlaying] = useState(true);
  const [currentTime, setCurrentTime] = useState(0);
  const [expertMode, setExpertMode] = useState(false);
  const animationFrameRef = useRef<number>();
  const lastTimeRef = useRef<number>(Date.now());
  
  const duration = 4000;

  const handlePatternChange = (newPattern: AnimationPattern) => {
    setPattern(newPattern);
    const presetConfig = PATTERN_PRESETS[newPattern];
    setConfig(presetConfig);
    setCurrentTime(0);
    lastTimeRef.current = Date.now();
  };

  const handleConfigChange = (updates: Partial<AnimationConfig>) => {
    setConfig(prev => ({ ...prev, ...updates }));
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
    toast.success('Pattern exported to .prism v1.1 format');
  };

  const handleSave = () => {
    toast.success('Pattern saved to library');
  };

  const handleShare = () => {
    toast.success('Share link copied to clipboard');
  };

  const handleCh1ColorChange = (index: number, color: string) => {
    const newColors = [...config.ch1Colors];
    newColors[index] = color;
    handleConfigChange({ ch1Colors: newColors });
  };

  const handleCh2ColorChange = (index: number, color: string) => {
    const newColors = [...config.ch2Colors];
    newColors[index] = color;
    handleConfigChange({ ch2Colors: newColors });
  };

  // Motion direction button config: icon and label
  const motionDirections: Array<{ value: MotionDirection; icon: React.ReactNode; label: string }> = [
    { value: 'left', icon: <ArrowLeft className="w-4 h-4" />, label: 'Left' },
    { value: 'center', icon: <CircleDot className="w-4 h-4" />, label: 'Center' },
    { value: 'right', icon: <ArrowRight className="w-4 h-4" />, label: 'Right' },
    { value: 'edge', icon: <Minimize2 className="w-4 h-4" />, label: 'Edge' },
    { value: 'static', icon: <Minus className="w-4 h-4" />, label: 'Static' },
  ];

  return (
    <div className="h-screen flex flex-col bg-[#0A0B0F] relative overflow-hidden">
      {/* Wavy Textured Background */}
      <div className="wavy-bg" />
      
      {/* Content */}
      <div className="relative z-10 flex flex-col h-full">
        <TopBar connected={true} fps="120" quality="HQ" />
        
        <div className="flex-1 flex overflow-hidden">
          {/* Left Panel - Pattern Controls */}
          <div className="w-[420px] overflow-y-auto overflow-x-hidden">
            <div className="p-6 space-y-6">
              {/* Header */}
              <div className="space-y-4">
                <div className="flex items-center justify-between">
                  <div>
                    <h2 className="flex items-center gap-2 text-2xl">
                      <Sparkles className="w-6 h-6 text-primary" />
                      Pattern <span className="text-accent">Studio</span>
                    </h2>
                    <p className="text-sm text-muted-foreground mt-1">
                      LGP Temporal Sequencing Engine
                    </p>
                  </div>
                  <div className="flex items-center gap-2">
                    <Label htmlFor="expert-mode" className="text-xs text-muted-foreground">
                      Expert
                    </Label>
                    <Switch
                      id="expert-mode"
                      checked={expertMode}
                      onCheckedChange={setExpertMode}
                    />
                  </div>
                </div>
                <div className="glass-heavy glass-texture rounded-xl p-4 shadow-elevation-2">
                  <div className="grid grid-cols-3 gap-3 text-xs">
                    <div>
                      <div className="text-muted-foreground mb-1">LEDs</div>
                      <div className="font-mono">320 (2×160)</div>
                    </div>
                    <div>
                      <div className="text-muted-foreground mb-1">Target FPS</div>
                      <div className="font-mono text-primary">120 fps</div>
                    </div>
                    <div>
                      <div className="text-muted-foreground mb-1">Type</div>
                      <div className="font-mono">WS2812B</div>
                    </div>
                  </div>
                </div>
              </div>

              {/* Motion Direction */}
              <Card className="glass-card glass-texture shadow-elevation-2 border-border/50 p-5">
                <div className="flex items-center gap-3 mb-4">
                  <div className="w-9 h-9 rounded-lg bg-primary/10 flex items-center justify-center">
                    <Zap className="w-5 h-5 text-primary" />
                  </div>
                  <div>
                    <h3 className="text-base">Motion Direction</h3>
                    <p className="text-xs text-muted-foreground">Horizontal propagation along 160-LED edges</p>
                  </div>
                </div>
                <div className="grid grid-cols-5 gap-2">
                  {motionDirections.map(({ value, icon, label }) => (
                    <Button
                      key={value}
                      variant={config.motionDirection === value ? 'default' : 'outline'}
                      size="sm"
                      onClick={() => handleConfigChange({ motionDirection: value })}
                      className="flex flex-col gap-1.5 h-[60px] px-1 hover-lift"
                    >
                      <span className="flex items-center justify-center">{icon}</span>
                      <span className="text-[10px]">{label}</span>
                    </Button>
                  ))}
                </div>
              </Card>

              {/* Sync Mode */}
              <Card className="glass-card glass-texture shadow-elevation-2 border-border/50 p-5">
                <div className="flex items-center gap-3 mb-4">
                  <div className="w-9 h-9 rounded-lg bg-accent/10 flex items-center justify-center">
                    <Waves className="w-5 h-5 text-accent" />
                  </div>
                  <div>
                    <h3 className="text-base">Sync Mode</h3>
                    <p className="text-xs text-muted-foreground">Temporal coordination between edges</p>
                  </div>
                </div>
                
                <Tabs value={config.syncMode} onValueChange={(v) => handleConfigChange({ syncMode: v as SyncMode })}>
                  <TabsList className="grid w-full mb-4" style={{ gridTemplateColumns: expertMode ? '1fr 1fr 1fr 1fr 1fr' : '1fr 1fr 1fr' }}>
                    <TabsTrigger value="sync" className="text-xs px-2">Sync</TabsTrigger>
                    <TabsTrigger value="offset" className="text-xs px-2">Offset</TabsTrigger>
                    <TabsTrigger value="progressive" className="text-xs px-2">Progress</TabsTrigger>
                    {expertMode && <TabsTrigger value="wave" className="text-xs px-2">Wave</TabsTrigger>}
                    {expertMode && <TabsTrigger value="custom" className="text-xs px-2">Custom</TabsTrigger>}
                  </TabsList>

                  <TabsContent value="sync" className="space-y-3 mt-0">
                    <div className="p-4 bg-primary/5 border border-primary/20 rounded-lg space-y-2">
                      <div className="flex items-center gap-2">
                        <Badge variant="outline" className="bg-primary/10 text-primary border-primary/30">
                          Optimized
                        </Badge>
                        <span className="text-xs">50% CPU savings</span>
                      </div>
                      <p className="text-xs text-muted-foreground">
                        Both edges fire simultaneously. Creates unified bright surface. Recommended for 80% of users.
                      </p>
                    </div>
                  </TabsContent>

                  <TabsContent value="offset" className="space-y-3 mt-0">
                    <div className="space-y-2">
                      <div className="flex items-center justify-between">
                        <Label className="text-xs">Delay (CH2)</Label>
                        <Badge variant="outline" className="font-mono text-xs">{config.offsetDelayMs}ms</Badge>
                      </div>
                      <Slider
                        value={[config.offsetDelayMs]}
                        onValueChange={(v) => handleConfigChange({ offsetDelayMs: v[0] })}
                        min={0}
                        max={500}
                        step={10}
                        className="w-full"
                      />
                      <div className="flex justify-between text-[10px] text-muted-foreground">
                        <span>0ms (sync)</span>
                        <span className={config.offsetDelayMs >= 60 && config.offsetDelayMs <= 300 ? 'text-success' : ''}>
                          Phi: 60-300ms
                        </span>
                        <span>500ms</span>
                      </div>
                    </div>
                    <p className="text-xs text-muted-foreground p-3 bg-muted/20 rounded-lg">
                      Creates uniform diagonal sweep. Best for rising/falling effects and depth illusion.
                    </p>
                  </TabsContent>

                  <TabsContent value="progressive" className="space-y-3 mt-0">
                    <div className="space-y-3">
                      <div className="space-y-2">
                        <div className="flex items-center justify-between">
                          <Label className="text-xs">Start Delay (LED 0)</Label>
                          <Badge variant="outline" className="font-mono text-xs">{config.progressiveStartMs}ms</Badge>
                        </div>
                        <Slider
                          value={[config.progressiveStartMs]}
                          onValueChange={(v) => handleConfigChange({ progressiveStartMs: v[0] })}
                          min={0}
                          max={500}
                          step={10}
                          className="w-full"
                        />
                      </div>
                      <div className="space-y-2">
                        <div className="flex items-center justify-between">
                          <Label className="text-xs">End Delay (LED 159)</Label>
                          <Badge variant="outline" className="font-mono text-xs">{config.progressiveEndMs}ms</Badge>
                        </div>
                        <Slider
                          value={[config.progressiveEndMs]}
                          onValueChange={(v) => handleConfigChange({ progressiveEndMs: v[0] })}
                          min={0}
                          max={500}
                          step={10}
                          className="w-full"
                        />
                      </div>
                    </div>
                    <p className="text-xs text-muted-foreground p-3 bg-muted/20 rounded-lg">
                      Delay varies linearly. Creates triangular/wedge shapes and diagonal ramps.
                    </p>
                    <div className="grid grid-cols-3 gap-2">
                      <Button
                        variant="outline"
                        size="sm"
                        onClick={() => handleConfigChange({ progressiveStartMs: 200, progressiveEndMs: 0 })}
                        className="text-[10px] hover-lift"
                      >
                        Right △
                      </Button>
                      <Button
                        variant="outline"
                        size="sm"
                        onClick={() => handleConfigChange({ progressiveStartMs: 0, progressiveEndMs: 200 })}
                        className="text-[10px] hover-lift"
                      >
                        Left △
                      </Button>
                      <Button
                        variant="outline"
                        size="sm"
                        onClick={() => handleConfigChange({ progressiveStartMs: 0, progressiveEndMs: 0 })}
                        className="text-[10px] hover-lift"
                      >
                        Diamond ◇
                      </Button>
                    </div>
                  </TabsContent>

                  {expertMode && (
                    <TabsContent value="wave" className="space-y-3 mt-0">
                      <div className="space-y-3">
                        <div className="space-y-2">
                          <div className="flex items-center justify-between">
                            <Label className="text-xs">Amplitude</Label>
                            <Badge variant="outline" className="font-mono text-xs">{config.waveAmplitudeMs}ms</Badge>
                          </div>
                          <Slider
                            value={[config.waveAmplitudeMs]}
                            onValueChange={(v) => handleConfigChange({ waveAmplitudeMs: v[0] })}
                            min={0}
                            max={500}
                            step={10}
                            className="w-full"
                          />
                        </div>
                        <div className="space-y-2">
                          <div className="flex items-center justify-between">
                            <Label className="text-xs">Period (cycles)</Label>
                            <Badge variant="outline" className="font-mono text-xs">{config.wavePeriod.toFixed(1)}</Badge>
                          </div>
                          <Slider
                            value={[config.wavePeriod * 10]}
                            onValueChange={(v) => handleConfigChange({ wavePeriod: v[0] / 10 })}
                            min={5}
                            max={40}
                            step={1}
                            className="w-full"
                          />
                        </div>
                        <div className="space-y-2">
                          <div className="flex items-center justify-between">
                            <Label className="text-xs">Phase</Label>
                            <Badge variant="outline" className="font-mono text-xs">{config.wavePhaseDeg}°</Badge>
                          </div>
                          <Slider
                            value={[config.wavePhaseDeg]}
                            onValueChange={(v) => handleConfigChange({ wavePhaseDeg: v[0] })}
                            min={0}
                            max={359}
                            step={1}
                            className="w-full"
                          />
                        </div>
                        <div className="space-y-2">
                          <Label className="text-xs">Waveform</Label>
                          <div className="grid grid-cols-3 gap-2">
                            {(['sin', 'triangle', 'sawtooth'] as WaveformType[]).map((wf) => (
                              <Button
                                key={wf}
                                variant={config.waveform === wf ? 'default' : 'outline'}
                                size="sm"
                                onClick={() => handleConfigChange({ waveform: wf })}
                                className="text-xs capitalize hover-lift"
                              >
                                {wf}
                              </Button>
                            ))}
                          </div>
                        </div>
                      </div>
                      <p className="text-xs text-muted-foreground p-3 bg-muted/20 rounded-lg">
                        Sinusoidal delay pattern. Creates organic wave motion, ripples, and spirals.
                      </p>
                    </TabsContent>
                  )}

                  {expertMode && (
                    <TabsContent value="custom" className="space-y-3 mt-0">
                      <div className="p-4 bg-accent/5 border border-accent/20 rounded-lg space-y-2">
                        <div className="flex items-center gap-2">
                          <Badge variant="outline" className="bg-accent/10 text-accent border-accent/30">
                            Advanced
                          </Badge>
                          <span className="text-xs">320 bytes</span>
                        </div>
                        <p className="text-xs text-muted-foreground">
                          Per-LED timing control with 160 independent delay values. Requires external timing map upload.
                        </p>
                      </div>
                      <Button 
                        variant="outline" 
                        className="w-full hover-lift" 
                        size="sm"
                        onClick={() => {
                          toast.info('Custom delay map upload coming soon');
                        }}
                      >
                        <Download className="w-3 h-3 mr-2" />
                        Load Delay Map (.csv)
                      </Button>
                    </TabsContent>
                  )}
                </Tabs>
              </Card>

              {/* Colors */}
              <Card className="glass-card glass-texture shadow-elevation-2 border-border/50 p-5">
                <div className="flex items-center gap-3 mb-4">
                  <div className="w-9 h-9 rounded-lg bg-gradient-to-br from-orange-500 to-purple-500 flex items-center justify-center">
                    <Palette className="w-5 h-5 text-white" />
                  </div>
                  <div>
                    <h3 className="text-base">Edge Colors</h3>
                    <p className="text-xs text-muted-foreground">Dual-channel LGP illumination</p>
                  </div>
                </div>

                <div className="space-y-4">
                  {/* CH1 */}
                  <div className="space-y-2">
                    <div className="flex items-center justify-between">
                      <Label className="text-xs">
                        CH1 <span className="text-muted-foreground">(Bottom, 160 LEDs)</span>
                      </Label>
                      <Button 
                        variant="ghost" 
                        size="sm"
                        onClick={() => handleConfigChange({ ch1Colors: [...config.ch1Colors, '#FF0000'] })}
                        className="h-6 gap-1 text-xs hover-lift"
                      >
                        <Plus className="w-3 h-3" />
                        Add
                      </Button>
                    </div>
                    <div className="flex gap-2 flex-wrap">
                      {config.ch1Colors.map((color, i) => (
                        <div key={i} className="relative group">
                          <Input
                            type="color"
                            value={color}
                            onChange={(e) => handleCh1ColorChange(i, e.target.value)}
                            className="w-12 h-12 p-1 cursor-pointer rounded-lg border-2 border-border"
                          />
                          {config.ch1Colors.length > 1 && (
                            <button
                              onClick={() => {
                                const newColors = config.ch1Colors.filter((_, idx) => idx !== i);
                                handleConfigChange({ ch1Colors: newColors });
                              }}
                              className="absolute -top-1 -right-1 w-5 h-5 bg-destructive text-white rounded-full flex items-center justify-center opacity-0 group-hover:opacity-100 transition-opacity"
                            >
                              <X className="w-3 h-3" />
                            </button>
                          )}
                        </div>
                      ))}
                    </div>
                  </div>

                  {/* CH2 */}
                  <div className="space-y-2">
                    <div className="flex items-center justify-between">
                      <Label className="text-xs">
                        CH2 <span className="text-muted-foreground">(Top, 160 LEDs)</span>
                      </Label>
                      <Button 
                        variant="ghost" 
                        size="sm"
                        onClick={() => handleConfigChange({ ch2Colors: [...config.ch2Colors, '#0000FF'] })}
                        className="h-6 gap-1 text-xs hover-lift"
                      >
                        <Plus className="w-3 h-3" />
                        Add
                      </Button>
                    </div>
                    <div className="flex gap-2 flex-wrap">
                      {config.ch2Colors.map((color, i) => (
                        <div key={i} className="relative group">
                          <Input
                            type="color"
                            value={color}
                            onChange={(e) => handleCh2ColorChange(i, e.target.value)}
                            className="w-12 h-12 p-1 cursor-pointer rounded-lg border-2 border-border"
                          />
                          {config.ch2Colors.length > 1 && (
                            <button
                              onClick={() => {
                                const newColors = config.ch2Colors.filter((_, idx) => idx !== i);
                                handleConfigChange({ ch2Colors: newColors });
                              }}
                              className="absolute -top-1 -right-1 w-5 h-5 bg-destructive text-white rounded-full flex items-center justify-center opacity-0 group-hover:opacity-100 transition-opacity"
                            >
                              <X className="w-3 h-3" />
                            </button>
                          )}
                        </div>
                      ))}
                    </div>
                  </div>

                  {/* Preview */}
                  <div className="space-y-2">
                    <Label className="text-xs">Preview</Label>
                    <div className="h-16 rounded-lg overflow-hidden border border-border shadow-inner">
                      <div className="h-1/2 flex">
                        {config.ch2Colors.map((color, i) => (
                          <div key={i} className="flex-1" style={{ backgroundColor: color }} />
                        ))}
                      </div>
                      <div className="h-1/2 flex">
                        {config.ch1Colors.map((color, i) => (
                          <div key={i} className="flex-1" style={{ backgroundColor: color }} />
                        ))}
                      </div>
                    </div>
                  </div>
                </div>
              </Card>

              {/* Current Pattern & Device - Side by Side */}
              <div className="grid grid-cols-2 gap-4">
                {/* Current Pattern Info */}
                <Card className="glass-card glass-texture shadow-elevation-2 border-border/50 p-4">
                  <div className="mb-3">
                    <span className="text-muted-foreground text-xs">Current Pattern</span>
                    <h3 className="capitalize mt-1 text-sm">{pattern}</h3>
                  </div>
                  <div className="space-y-2 text-xs">
                    <div className="flex items-center justify-between">
                      <span className="text-muted-foreground">Motion:</span>
                      <Badge variant="outline" className="text-[10px] capitalize bg-primary/10 border-primary/30">
                        {config.motionDirection}
                      </Badge>
                    </div>
                    <div className="flex items-center justify-between">
                      <span className="text-muted-foreground">Sync:</span>
                      <Badge variant="outline" className="text-[10px] capitalize bg-accent/10 border-accent/30">
                        {config.syncMode}
                      </Badge>
                    </div>
                    <div className="flex items-center justify-between">
                      <span className="text-muted-foreground">Speed:</span>
                      <Badge variant="outline" className="text-[10px] font-mono">{config.speed}</Badge>
                    </div>
                    {config.syncMode === 'offset' && (
                      <div className="flex items-center justify-between">
                        <span className="text-muted-foreground">Delay:</span>
                        <Badge variant="outline" className={`text-[10px] font-mono ${
                          config.offsetDelayMs >= 60 && config.offsetDelayMs <= 300 
                            ? 'bg-success/10 border-success/30 text-success' 
                            : ''
                        }`}>
                          {config.offsetDelayMs}ms
                        </Badge>
                      </div>
                    )}
                  </div>
                </Card>

                {/* Device Specs */}
                <Card className="glass-card glass-texture shadow-elevation-2 border-border/50 p-4">
                  <div className="mb-3">
                    <span className="text-muted-foreground text-xs">Device</span>
                    <h4 className="mt-1 text-sm">K1-Lightwave</h4>
                  </div>
                  <div className="space-y-1.5 text-xs text-muted-foreground">
                    <div>330mm × 60mm</div>
                    <div>WS2812B × 320</div>
                    <div>Dual-Edge LGP</div>
                    <div className="pt-2 border-t border-border text-primary font-mono">
                      {Math.round((currentTime / duration) * 120)} fps
                    </div>
                  </div>
                </Card>
              </div>
            </div>
          </div>

          {/* Center - Preview Canvas */}
          <div className="flex-1 flex flex-col justify-between">
            {/* Preview Area */}
            <div className="flex-1 relative flex items-center justify-center">
              {/* Background Grid */}
              <div className="absolute inset-0 opacity-[0.015]" style={{
                backgroundImage: 'linear-gradient(rgba(255,255,255,0.1) 1px, transparent 1px), linear-gradient(90deg, rgba(255,255,255,0.1) 1px, transparent 1px)',
                backgroundSize: '40px 40px'
              }} />

              {/* Color Splash - Left */}
              <div 
                className="absolute top-[20%] left-[10%] w-[380px] h-[380px] rounded-full opacity-25 pointer-events-none"
                style={{
                  background: 'radial-gradient(circle, rgba(91, 140, 255, 0.5), transparent 65%)',
                  filter: 'blur(90px)'
                }}
              />

              {/* Color Splash - Right */}
              <div 
                className="absolute top-[20%] right-[10%] w-[320px] h-[320px] rounded-full opacity-25 pointer-events-none"
                style={{
                  background: 'radial-gradient(circle, rgba(255, 122, 89, 0.5), transparent 65%)',
                  filter: 'blur(90px)'
                }}
              />

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

              {/* Phi Phenomenon Indicator */}
              {config.syncMode === 'offset' && config.offsetDelayMs >= 60 && config.offsetDelayMs <= 300 && (
                <div className="absolute bottom-[15%] left-1/2 -translate-x-1/2 glass-heavy glass-texture shadow-elevation-2 border border-success/30 rounded-lg px-4 py-2 text-xs">
                  <div className="flex items-center gap-2 text-success">
                    <Sparkles className="w-3.5 h-3.5" />
                    <span>Phi Phenomenon Active (optimal geometric shapes)</span>
                  </div>
                </div>
              )}
            </div>

            {/* Animation Player */}
            <div className="p-6 flex justify-center">
              <div className="w-[60%]">
                <div className="mb-3">
                  <h3 className="text-base">Animation Player</h3>
                </div>
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
          </div>

          {/* Right Panel - Actions & Export */}
          <div className="w-[300px] overflow-y-auto overflow-x-hidden">
            <div className="p-6 space-y-6">
              {/* Export Actions */}
              <Card className="glass-card glass-texture shadow-elevation-2 border-border/50 p-5">
                <h3 className="text-base mb-4">Export & Share</h3>
                <div className="space-y-2">
                  <Button 
                    className="w-full gap-2 hover-lift glow-primary" 
                    onClick={handleExport}
                  >
                    <Download className="w-4 h-4" />
                    Export .prism v1.1
                  </Button>
                  <Button 
                    variant="outline" 
                    className="w-full gap-2 hover-lift glass"
                    onClick={handleSave}
                  >
                    <Save className="w-4 h-4" />
                    Save to Library
                  </Button>
                  <Button 
                    variant="outline" 
                    className="w-full gap-2 hover-lift glass"
                    onClick={handleShare}
                  >
                    <Share2 className="w-4 h-4" />
                    Share Link
                  </Button>
                </div>
              </Card>

              {/* Pattern Presets */}
              <Card className="glass-card glass-texture shadow-elevation-2 border-border/50 p-5">
                <h3 className="text-base mb-4">Pattern Presets</h3>
                <div className="grid grid-cols-2 gap-2">
                  {(Object.keys(PATTERN_PRESETS) as AnimationPattern[]).map((presetPattern) => (
                    <Button
                      key={presetPattern}
                      variant={pattern === presetPattern ? 'default' : 'outline'}
                      size="sm"
                      onClick={() => handlePatternChange(presetPattern)}
                      className="text-xs capitalize hover-lift"
                    >
                      {presetPattern.replace(/([A-Z])/g, ' $1').trim()}
                    </Button>
                  ))}
                </div>
              </Card>

              {/* Estimated File Size */}
              <Card className="glass-card glass-texture shadow-elevation-2 border-border/50 p-5">
                <h3 className="text-base mb-4">Estimated Output</h3>
                <div className="space-y-3 text-sm">
                  <div className="flex justify-between">
                    <span className="text-muted-foreground">Duration:</span>
                    <span className="font-mono">{(duration / 1000).toFixed(1)}s</span>
                  </div>
                  <div className="flex justify-between">
                    <span className="text-muted-foreground">Target FPS:</span>
                    <span className="font-mono">120</span>
                  </div>
                  <div className="flex justify-between">
                    <span className="text-muted-foreground">Frames:</span>
                    <span className="font-mono">{Math.floor((duration / 1000) * 120)}</span>
                  </div>
                  <div className="flex justify-between">
                    <span className="text-muted-foreground">LEDs:</span>
                    <span className="font-mono">320 (2×160)</span>
                  </div>
                  <div className="pt-3 border-t border-border/50">
                    <div className="flex justify-between">
                      <span className="text-muted-foreground">Header:</span>
                      <span className="font-mono text-xs">70 B</span>
                    </div>
                    <div className="flex justify-between mt-1">
                      <span className="text-muted-foreground">Frame Data:</span>
                      <span className="font-mono text-xs">~{Math.floor((duration / 1000) * 120 * 0.32)} KB</span>
                    </div>
                  </div>
                  <div className="pt-3 border-t border-border/50 flex justify-between items-center">
                    <span className="font-medium">Est. Size:</span>
                    <Badge className="font-mono bg-primary/20 text-primary border-primary/40">
                      ~{Math.floor(((duration / 1000) * 120 * 0.32) + 0.07)} KB
                    </Badge>
                  </div>
                </div>
              </Card>
            </div>
          </div>
        </div>
      </div>
    </div>
  );
}
