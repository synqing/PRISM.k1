import { useState } from 'react';
import { Card } from '../ui/card';
import { Label } from '../ui/label';
import { Input } from '../ui/input';
import { Slider } from '../ui/slider';
import { Button } from '../ui/button';
import { Badge } from '../ui/badge';
import { Tabs, TabsContent, TabsList, TabsTrigger } from '../ui/tabs';
import { 
  AnimationConfig, 
  AnimationPattern,
  SyncMode,
  MotionOrigin,
  PATTERN_PRESETS 
} from './K1AnimationEngine';
import { Palette, Zap, Route, Plus, X } from 'lucide-react';

interface LivePatternEditorProps {
  pattern: AnimationPattern;
  config: AnimationConfig;
  onConfigChange: (config: Partial<AnimationConfig>) => void;
}

export function LivePatternEditor({
  pattern,
  config,
  onConfigChange,
}: LivePatternEditorProps) {
  const [activeTab, setActiveTab] = useState<'colors' | 'motion' | 'timing'>('colors');

  const handleCh1ColorChange = (index: number, color: string) => {
    const newColors = [...config.ch1Colors];
    newColors[index] = color;
    onConfigChange({ ch1Colors: newColors });
  };

  const handleCh2ColorChange = (index: number, color: string) => {
    const newColors = [...config.ch2Colors];
    newColors[index] = color;
    onConfigChange({ ch2Colors: newColors });
  };

  const handleAddCh1Color = () => {
    onConfigChange({ ch1Colors: [...config.ch1Colors, '#FFFFFF'] });
  };

  const handleAddCh2Color = () => {
    onConfigChange({ ch2Colors: [...config.ch2Colors, '#FFFFFF'] });
  };

  const handleRemoveCh1Color = (index: number) => {
    if (config.ch1Colors.length > 1) {
      const newColors = config.ch1Colors.filter((_, i) => i !== index);
      onConfigChange({ ch1Colors: newColors });
    }
  };

  const handleRemoveCh2Color = (index: number) => {
    if (config.ch2Colors.length > 1) {
      const newColors = config.ch2Colors.filter((_, i) => i !== index);
      onConfigChange({ ch2Colors: newColors });
    }
  };

  const handleResetToPreset = () => {
    const preset = PATTERN_PRESETS[pattern];
    onConfigChange(preset);
  };

  return (
    <Card className="p-6 glass shadow-elevation-2 border-border/50">
      <div className="flex items-center justify-between mb-6">
        <div className="flex items-center gap-3">
          <div className="w-10 h-10 rounded-lg bg-accent/10 flex items-center justify-center">
            <Palette className="w-5 h-5 text-accent" />
          </div>
          <div>
            <h3 className="font-semibold">Live Pattern Editor</h3>
            <p className="text-xs text-muted-foreground">Real-time LGP customization</p>
          </div>
        </div>
        <Button 
          variant="outline" 
          size="sm" 
          onClick={handleResetToPreset}
          className="hover-lift"
        >
          Reset to Preset
        </Button>
      </div>

      <Tabs value={activeTab} onValueChange={(v) => setActiveTab(v as any)}>
        <TabsList className="grid grid-cols-3 w-full mb-6">
          <TabsTrigger value="colors" className="gap-2">
            <Palette className="w-3.5 h-3.5" />
            Colors
          </TabsTrigger>
          <TabsTrigger value="motion" className="gap-2">
            <Route className="w-3.5 h-3.5" />
            Motion
          </TabsTrigger>
          <TabsTrigger value="timing" className="gap-2">
            <Zap className="w-3.5 h-3.5" />
            Timing
          </TabsTrigger>
        </TabsList>

        <TabsContent value="colors" className="space-y-6">
          {/* CH1 (Bottom) Colors */}
          <div className="space-y-3">
            <div className="flex items-center justify-between">
              <Label className="text-sm font-medium">
                CH1 (Bottom Edge)
                <Badge variant="outline" className="ml-2 text-xs">Warm</Badge>
              </Label>
              <Button
                variant="ghost"
                size="sm"
                onClick={handleAddCh1Color}
                className="h-7 gap-1.5 hover-lift"
              >
                <Plus className="w-3 h-3" />
                Add Color
              </Button>
            </div>
            <div className="grid grid-cols-2 gap-3">
              {config.ch1Colors.map((color, index) => (
                <div key={index} className="flex items-center gap-2 p-2 border border-border rounded-lg">
                  <Input
                    type="color"
                    value={color}
                    onChange={(e) => handleCh1ColorChange(index, e.target.value)}
                    className="w-12 h-8 p-1 cursor-pointer"
                  />
                  <Input
                    type="text"
                    value={color}
                    onChange={(e) => handleCh1ColorChange(index, e.target.value)}
                    className="flex-1 h-8 font-mono text-xs"
                  />
                  <Button
                    variant="ghost"
                    size="sm"
                    onClick={() => handleRemoveCh1Color(index)}
                    disabled={config.ch1Colors.length <= 1}
                    className="h-8 w-8 p-0"
                  >
                    <X className="w-3 h-3" />
                  </Button>
                </div>
              ))}
            </div>
          </div>

          {/* CH2 (Top) Colors */}
          <div className="space-y-3">
            <div className="flex items-center justify-between">
              <Label className="text-sm font-medium">
                CH2 (Top Edge)
                <Badge variant="outline" className="ml-2 text-xs">Cool</Badge>
              </Label>
              <Button
                variant="ghost"
                size="sm"
                onClick={handleAddCh2Color}
                className="h-7 gap-1.5 hover-lift"
              >
                <Plus className="w-3 h-3" />
                Add Color
              </Button>
            </div>
            <div className="grid grid-cols-2 gap-3">
              {config.ch2Colors.map((color, index) => (
                <div key={index} className="flex items-center gap-2 p-2 border border-border rounded-lg">
                  <Input
                    type="color"
                    value={color}
                    onChange={(e) => handleCh2ColorChange(index, e.target.value)}
                    className="w-12 h-8 p-1 cursor-pointer"
                  />
                  <Input
                    type="text"
                    value={color}
                    onChange={(e) => handleCh2ColorChange(index, e.target.value)}
                    className="flex-1 h-8 font-mono text-xs"
                  />
                  <Button
                    variant="ghost"
                    size="sm"
                    onClick={() => handleRemoveCh2Color(index)}
                    disabled={config.ch2Colors.length <= 1}
                    className="h-8 w-8 p-0"
                  >
                    <X className="w-3 h-3" />
                  </Button>
                </div>
              ))}
            </div>
          </div>

          {/* Color Preview */}
          <div className="space-y-2">
            <Label className="text-sm font-medium">Preview</Label>
            <div className="h-20 rounded-lg overflow-hidden border border-border">
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
        </TabsContent>

        <TabsContent value="motion" className="space-y-6">
          {/* Motion Origin */}
          <div className="space-y-2">
            <Label className="text-sm font-medium">Motion Origin</Label>
            <div className="grid grid-cols-3 gap-2">
              {(['left', 'center', 'right'] as MotionOrigin[]).map((origin) => (
                <Button
                  key={origin}
                  variant={config.motionOrigin === origin ? 'default' : 'outline'}
                  size="sm"
                  onClick={() => onConfigChange({ motionOrigin: origin })}
                  className="capitalize hover-lift"
                >
                  {origin}
                </Button>
              ))}
            </div>
          </div>

          {/* Sync Mode */}
          <div className="space-y-2">
            <Label className="text-sm font-medium">Sync Mode</Label>
            <div className="grid grid-cols-2 gap-2">
              {(['synchronized', 'offset', 'progressive', 'split'] as SyncMode[]).map((mode) => (
                <Button
                  key={mode}
                  variant={config.syncMode === mode ? 'default' : 'outline'}
                  size="sm"
                  onClick={() => onConfigChange({ syncMode: mode })}
                  className="capitalize hover-lift text-xs"
                >
                  {mode}
                </Button>
              ))}
            </div>
          </div>

          {/* Speed */}
          <div className="space-y-2">
            <div className="flex items-center justify-between">
              <Label className="text-sm font-medium">Speed</Label>
              <Badge variant="outline" className="font-mono">{config.speed}</Badge>
            </div>
            <Slider
              value={[config.speed]}
              onValueChange={(v) => onConfigChange({ speed: v[0] })}
              min={1}
              max={255}
              step={1}
              className="w-full"
            />
            <div className="flex justify-between text-xs text-muted-foreground">
              <span>Slow (1)</span>
              <span>Fast (255)</span>
            </div>
          </div>
        </TabsContent>

        <TabsContent value="timing" className="space-y-6">
          {/* Temporal Offset */}
          <div className="space-y-2">
            <div className="flex items-center justify-between">
              <Label className="text-sm font-medium">Temporal Offset (CH2 delay)</Label>
              <Badge variant="outline" className="font-mono">{config.temporalOffset}ms</Badge>
            </div>
            <Slider
              value={[config.temporalOffset]}
              onValueChange={(v) => onConfigChange({ temporalOffset: v[0] })}
              min={0}
              max={300}
              step={10}
              className="w-full"
            />
            <div className="flex justify-between text-xs text-muted-foreground">
              <span>0ms (sync)</span>
              <span>300ms (max)</span>
            </div>
          </div>

          {/* Timing Info */}
          <div className="p-4 bg-muted/30 rounded-lg space-y-2 text-sm">
            <div className="flex justify-between">
              <span className="text-muted-foreground">Optimal phi range:</span>
              <span className="font-mono">60-150ms</span>
            </div>
            <div className="flex justify-between">
              <span className="text-muted-foreground">Current offset:</span>
              <span className={`font-mono ${
                config.temporalOffset >= 60 && config.temporalOffset <= 150 
                  ? 'text-success' 
                  : 'text-muted-foreground'
              }`}>
                {config.temporalOffset}ms
              </span>
            </div>
            <div className="pt-2 border-t border-border text-xs text-muted-foreground">
              Phi phenomenon creates geometric shapes (triangles, diamonds) when delays are 60-150ms.
            </div>
          </div>

          {/* Quick Presets */}
          <div className="space-y-2">
            <Label className="text-sm font-medium">Quick Timing Presets</Label>
            <div className="grid grid-cols-3 gap-2">
              <Button
                variant="outline"
                size="sm"
                onClick={() => onConfigChange({ temporalOffset: 80 })}
                className="hover-lift"
              >
                Quick (80ms)
              </Button>
              <Button
                variant="outline"
                size="sm"
                onClick={() => onConfigChange({ temporalOffset: 120 })}
                className="hover-lift"
              >
                Medium (120ms)
              </Button>
              <Button
                variant="outline"
                size="sm"
                onClick={() => onConfigChange({ temporalOffset: 180 })}
                className="hover-lift"
              >
                Slow (180ms)
              </Button>
            </div>
          </div>
        </TabsContent>
      </Tabs>
    </Card>
  );
}
