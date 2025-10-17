import { useState } from "react";
import { Tabs, TabsContent, TabsList, TabsTrigger } from "../ui/tabs";
import { Label } from "../ui/label";
import { Slider } from "../ui/slider";
import { Input } from "../ui/input";
import { Select, SelectContent, SelectItem, SelectTrigger, SelectValue } from "../ui/select";
import { Button } from "../ui/button";
import { ScrollArea } from "../ui/scroll-area";
import { Sliders, TrendingUp, Palette, Plus } from "lucide-react";

interface InspectorProps {
  selectedClip?: string;
}

const MOCK_PARAMS = [
  { name: 'Speed', value: 50, min: 0, max: 100 },
  { name: 'Intensity', value: 75, min: 0, max: 100 },
  { name: 'Hue Shift', value: 0, min: -180, max: 180 }
];

const MOCK_KEYFRAMES = [
  { time: '0.0s', value: 0 },
  { time: '2.5s', value: 100 },
  { time: '5.0s', value: 50 }
];

export function Inspector({ selectedClip = 'Ocean Sunrise' }: InspectorProps) {
  const [tab, setTab] = useState('params');

  return (
    <div className="w-[320px] border-l border-border bg-card/50 backdrop-blur-sm h-full flex flex-col">
      <div className="h-12 border-b border-border px-4 flex items-center glass">
        <h3 className="font-semibold">Inspector</h3>
      </div>

      <Tabs value={tab} onValueChange={setTab} className="flex-1 flex flex-col">
        <TabsList className="w-full grid grid-cols-3 rounded-none border-b border-border h-10 bg-muted/30">
          <TabsTrigger value="params" className="gap-1.5 text-xs data-[state=active]:shadow-sm">
            <Sliders className="w-3.5 h-3.5" />
            Params
          </TabsTrigger>
          <TabsTrigger value="easing" className="gap-1.5 text-xs data-[state=active]:shadow-sm">
            <TrendingUp className="w-3.5 h-3.5" />
            Easing
          </TabsTrigger>
          <TabsTrigger value="palette" className="gap-1.5 text-xs data-[state=active]:shadow-sm">
            <Palette className="w-3.5 h-3.5" />
            Palette
          </TabsTrigger>
        </TabsList>

        <ScrollArea className="flex-1">
          <TabsContent value="params" className="p-4 space-y-4 m-0">
            <div>
              <Label className="text-xs text-muted-foreground">Effect</Label>
              <div className="font-medium mt-1">{selectedClip}</div>
            </div>

            {MOCK_PARAMS.map((param) => (
              <div key={param.name} className="space-y-2">
                <div className="flex items-center justify-between">
                  <Label>{param.name}</Label>
                  <Input
                    type="number"
                    value={param.value}
                    className="w-16 h-7 text-right"
                  />
                </div>
                <Slider
                  value={[param.value]}
                  min={param.min}
                  max={param.max}
                  step={1}
                  className="w-full"
                />
              </div>
            ))}
          </TabsContent>

          <TabsContent value="easing" className="p-4 space-y-4 m-0">
            <div className="space-y-2">
              <Label>Easing Function</Label>
              <Select defaultValue="ease-in-out">
                <SelectTrigger className="hover-lift">
                  <SelectValue />
                </SelectTrigger>
                <SelectContent className="glass shadow-elevation-2">
                  <SelectItem value="linear">Linear</SelectItem>
                  <SelectItem value="ease">Ease</SelectItem>
                  <SelectItem value="ease-in">Ease In</SelectItem>
                  <SelectItem value="ease-out">Ease Out</SelectItem>
                  <SelectItem value="ease-in-out">Ease In-Out</SelectItem>
                  <SelectItem value="bezier">Custom Bezier</SelectItem>
                </SelectContent>
              </Select>
            </div>

            <div className="aspect-square bg-gradient-to-br from-muted/50 to-muted/30 rounded-lg flex items-center justify-center text-muted-foreground text-sm border border-border relative overflow-hidden">
              {/* Easing Curve Visualization */}
              <svg className="absolute inset-0 w-full h-full p-4" viewBox="0 0 100 100" preserveAspectRatio="none">
                <path
                  d="M 10,90 C 20,90 40,10 50,10 C 60,10 80,90 90,10"
                  stroke="currentColor"
                  strokeWidth="2"
                  fill="none"
                  className="text-primary opacity-60"
                />
              </svg>
              <span className="relative z-10">Easing Curve Preview</span>
            </div>
          </TabsContent>

          <TabsContent value="palette" className="p-4 space-y-4 m-0">
            <div className="space-y-2">
              <Label>Palette Reference</Label>
              <Select defaultValue="sunset">
                <SelectTrigger>
                  <SelectValue />
                </SelectTrigger>
                <SelectContent>
                  <SelectItem value="sunset">Sunset</SelectItem>
                  <SelectItem value="ocean">Ocean</SelectItem>
                  <SelectItem value="forest">Forest</SelectItem>
                  <SelectItem value="neon">Neon</SelectItem>
                </SelectContent>
              </Select>
            </div>

            <div className="space-y-2">
              <Label>Color Stops</Label>
              <div className="space-y-2">
                {['#FF6B35', '#F7931E', '#FDC830'].map((color, i) => (
                  <div key={i} className="flex items-center gap-2">
                    <div
                      className="w-8 h-8 rounded border border-border"
                      style={{ backgroundColor: color }}
                    />
                    <Input value={color} className="flex-1 h-8" />
                  </div>
                ))}
              </div>
            </div>
          </TabsContent>
        </ScrollArea>
      </Tabs>

      {/* Automation Panel */}
      <div className="border-t border-border">
        <div className="p-4 space-y-3">
          <div className="flex items-center justify-between">
            <Label>Automation</Label>
            <Button variant="ghost" size="sm" className="h-7 w-7 p-0">
              <Plus className="w-4 h-4" />
            </Button>
          </div>
          <div className="space-y-1">
            {MOCK_KEYFRAMES.map((kf, i) => (
              <div
                key={i}
                className="flex items-center justify-between text-sm p-2 rounded hover:bg-accent/10"
              >
                <span className="text-muted-foreground font-mono">{kf.time}</span>
                <span>{kf.value}</span>
              </div>
            ))}
          </div>
        </div>
      </div>
    </div>
  );
}
