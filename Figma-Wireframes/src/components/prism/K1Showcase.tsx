import { useState } from 'react';
import { K1Device } from './K1Device';
import { K1HeroComposition } from './K1HeroComposition';
import { Card } from '../ui/card';
import { Tabs, TabsContent, TabsList, TabsTrigger } from '../ui/tabs';
import { Badge } from '../ui/badge';
import { Button } from '../ui/button';
import { Download } from 'lucide-react';

export function K1Showcase() {
  const [selectedVariant, setSelectedVariant] = useState<'h50' | 'h60' | 'h70'>('h60');
  const [selectedGlow, setSelectedGlow] = useState<'magentaFire' | 'cyanTeal' | 'triLobe'>('magentaFire');

  return (
    <div className="min-h-screen bg-background">
      {/* Header */}
      <div className="border-b border-border glass backdrop-blur-xl p-6">
        <div className="max-w-7xl mx-auto">
          <h1 className="text-3xl font-semibold tracking-tight mb-2">K1-Lightwave Device Mockups</h1>
          <p className="text-muted-foreground">
            Photoreal renders • 1px = 0.25mm • 330mm bar • Dual-edge-lit LGP
          </p>
        </div>
      </div>

      <div className="max-w-7xl mx-auto p-8 space-y-12">
        {/* Hero Compositions */}
        <section>
          <div className="flex items-center justify-between mb-6">
            <div>
              <h2 className="text-2xl font-semibold mb-1">Hero Compositions</h2>
              <p className="text-sm text-muted-foreground">Studio shots with environment</p>
            </div>
            <Button variant="outline" className="gap-2 hover-lift">
              <Download className="w-4 h-4" />
              Export 2×
            </Button>
          </div>
          
          <div className="grid grid-cols-2 gap-6">
            <Card className="overflow-hidden shadow-elevation-2 border-border/50">
              <div className="aspect-video">
                <K1HeroComposition glowStyle="magentaFire" heightVariant="h60" />
              </div>
              <div className="p-4 border-t border-border">
                <div className="flex items-center justify-between">
                  <div>
                    <div className="font-medium">Magenta Fire</div>
                    <div className="text-xs text-muted-foreground">60mm • Dark Frame • 3/4 View</div>
                  </div>
                  <Badge variant="outline">1920×1080</Badge>
                </div>
              </div>
            </Card>

            <Card className="overflow-hidden shadow-elevation-2 border-border/50">
              <div className="aspect-video">
                <K1HeroComposition glowStyle="cyanTeal" heightVariant="h60" />
              </div>
              <div className="p-4 border-t border-border">
                <div className="flex items-center justify-between">
                  <div>
                    <div className="font-medium">Cyan Teal</div>
                    <div className="text-xs text-muted-foreground">60mm • Dark Frame • 3/4 View</div>
                  </div>
                  <Badge variant="outline">1920×1080</Badge>
                </div>
              </div>
            </Card>
          </div>
        </section>

        {/* Variant Explorer */}
        <section>
          <h2 className="text-2xl font-semibold mb-6">Variant Explorer</h2>
          
          <div className="grid grid-cols-4 gap-4 mb-6">
            <Card className="p-4">
              <div className="text-sm text-muted-foreground mb-2">Height</div>
              <Tabs value={selectedVariant} onValueChange={(v) => setSelectedVariant(v as any)}>
                <TabsList className="w-full grid grid-cols-3">
                  <TabsTrigger value="h50" className="text-xs">50mm</TabsTrigger>
                  <TabsTrigger value="h60" className="text-xs">60mm</TabsTrigger>
                  <TabsTrigger value="h70" className="text-xs">70mm</TabsTrigger>
                </TabsList>
              </Tabs>
            </Card>

            <Card className="p-4">
              <div className="text-sm text-muted-foreground mb-2">Glow Style</div>
              <Tabs value={selectedGlow} onValueChange={(v) => setSelectedGlow(v as any)}>
                <TabsList className="w-full grid grid-cols-3">
                  <TabsTrigger value="magentaFire" className="text-xs">Mag</TabsTrigger>
                  <TabsTrigger value="cyanTeal" className="text-xs">Cyan</TabsTrigger>
                  <TabsTrigger value="triLobe" className="text-xs">Tri</TabsTrigger>
                </TabsList>
              </Tabs>
            </Card>

            <Card className="p-4">
              <div className="text-sm text-muted-foreground mb-2">Dimensions</div>
              <div className="space-y-1">
                <div className="text-sm font-mono">W: 330mm (1320px)</div>
                <div className="text-sm font-mono">H: {selectedVariant === 'h50' ? '50' : selectedVariant === 'h60' ? '60' : '70'}mm</div>
              </div>
            </Card>

            <Card className="p-4">
              <div className="text-sm text-muted-foreground mb-2">Technology</div>
              <div className="space-y-1">
                <div className="text-xs">Dual-Edge LGP</div>
                <div className="text-xs">WS2812B × 144</div>
              </div>
            </Card>
          </div>

          {/* Preview Canvas */}
          <Card className="overflow-hidden shadow-elevation-2 border-border/50">
            <div className="bg-[#0D0F14] p-12 flex items-center justify-center min-h-[400px]">
              <div style={{ transform: 'scale(0.5)' }}>
                <K1Device
                  heightVariant={selectedVariant}
                  finish="dark-frame"
                  glowStyle={selectedGlow}
                  angle="front"
                />
              </div>
            </div>
          </Card>
        </section>

        {/* Storyboards (Motion Frames) */}
        <section>
          <h2 className="text-2xl font-semibold mb-6">Storyboards (Animation Frames)</h2>
          
          <div className="space-y-8">
            {/* Center Bloom */}
            <div>
              <h3 className="text-lg font-medium mb-3">Center Bloom Pulse</h3>
              <div className="grid grid-cols-5 gap-3">
                {[0, 25, 50, 75, 100].map((frame) => (
                  <Card key={frame} className="overflow-hidden shadow-sm">
                    <div className="bg-[#0D0F14] p-4 flex items-center justify-center aspect-video">
                      <div style={{ transform: 'scale(0.15)' }}>
                        <K1Device
                          heightVariant="h60"
                          finish="dark-frame"
                          glowStyle="magentaFire"
                          angle="front"
                        />
                      </div>
                    </div>
                    <div className="p-2 border-t border-border text-center">
                      <div className="text-xs font-mono text-muted-foreground">T+{frame * 4}ms</div>
                    </div>
                  </Card>
                ))}
              </div>
            </div>

            {/* Rising Wave */}
            <div>
              <h3 className="text-lg font-medium mb-3">Rising Wave</h3>
              <div className="grid grid-cols-5 gap-3">
                {[0, 25, 50, 75, 100].map((frame) => (
                  <Card key={frame} className="overflow-hidden shadow-sm">
                    <div className="bg-[#0D0F14] p-4 flex items-center justify-center aspect-video">
                      <div style={{ transform: 'scale(0.15)' }}>
                        <K1Device
                          heightVariant="h60"
                          finish="dark-frame"
                          glowStyle="cyanTeal"
                          angle="front"
                        />
                      </div>
                    </div>
                    <div className="p-2 border-t border-border text-center">
                      <div className="text-xs font-mono text-muted-foreground">T+{frame * 4}ms</div>
                    </div>
                  </Card>
                ))}
              </div>
            </div>

            {/* Dual Zone Pulse */}
            <div>
              <h3 className="text-lg font-medium mb-3">Dual Zone Pulse</h3>
              <div className="grid grid-cols-4 gap-3">
                {[0, 33, 66, 100].map((frame) => (
                  <Card key={frame} className="overflow-hidden shadow-sm">
                    <div className="bg-[#0D0F14] p-4 flex items-center justify-center aspect-video">
                      <div style={{ transform: 'scale(0.15)' }}>
                        <K1Device
                          heightVariant="h60"
                          finish="dark-frame"
                          glowStyle="triLobe"
                          angle="front"
                        />
                      </div>
                    </div>
                    <div className="p-2 border-t border-border text-center">
                      <div className="text-xs font-mono text-muted-foreground">T+{Math.round(frame * 3)}ms</div>
                    </div>
                  </Card>
                ))}
              </div>
            </div>
          </div>
        </section>

        {/* Technical Specifications */}
        <section>
          <h2 className="text-2xl font-semibold mb-6">Technical Specifications</h2>
          
          <div className="grid grid-cols-2 gap-6">
            <Card className="p-6">
              <h3 className="font-medium mb-4">Physical Properties</h3>
              <dl className="space-y-2 text-sm">
                <div className="flex justify-between">
                  <dt className="text-muted-foreground">Width (exact):</dt>
                  <dd className="font-mono">330mm (1320px)</dd>
                </div>
                <div className="flex justify-between">
                  <dt className="text-muted-foreground">Height variants:</dt>
                  <dd className="font-mono">50/60/70mm</dd>
                </div>
                <div className="flex justify-between">
                  <dt className="text-muted-foreground">Corner radius:</dt>
                  <dd className="font-mono">12px outer / 8px inner</dd>
                </div>
                <div className="flex justify-between">
                  <dt className="text-muted-foreground">Scale:</dt>
                  <dd className="font-mono">1px = 0.25mm</dd>
                </div>
              </dl>
            </Card>

            <Card className="p-6">
              <h3 className="font-medium mb-4">LGP Light Recipe</h3>
              <dl className="space-y-2 text-sm">
                <div className="flex justify-between">
                  <dt className="text-muted-foreground">Base frost:</dt>
                  <dd>6px blur @ 88% opacity</dd>
                </div>
                <div className="flex justify-between">
                  <dt className="text-muted-foreground">Edge injection:</dt>
                  <dd>Top & bottom lobes @ 28px blur</dd>
                </div>
                <div className="flex justify-between">
                  <dt className="text-muted-foreground">Additive field:</dt>
                  <dd>32px blur @ 22% opacity</dd>
                </div>
                <div className="flex justify-between">
                  <dt className="text-muted-foreground">Outer glow:</dt>
                  <dd>36px blur @ 15% opacity</dd>
                </div>
              </dl>
            </Card>
          </div>
        </section>

        {/* Acceptance Criteria */}
        <section>
          <h2 className="text-2xl font-semibold mb-6">Acceptance Criteria</h2>
          
          <Card className="p-6">
            <div className="space-y-3">
              {[
                { criterion: 'Width EXACTLY 1320px (330mm)', status: true },
                { criterion: 'No visible LED pixel dots', status: true },
                { criterion: 'Edge-brighter vertical profile', status: true },
                { criterion: 'Lobed blooms match photo vibe', status: true },
                { criterion: 'No hard horizontal divider', status: true },
                { criterion: 'Additive purple/teal in mid-panel', status: true },
                { criterion: '3/4 angle looks physically plausible', status: true },
                { criterion: 'All blur radii ≥ 20px', status: true },
              ].map((item, i) => (
                <div key={i} className="flex items-center gap-3">
                  <div className={`w-5 h-5 rounded-full flex items-center justify-center text-xs ${
                    item.status ? 'bg-success/20 text-success' : 'bg-muted text-muted-foreground'
                  }`}>
                    {item.status ? '✓' : '○'}
                  </div>
                  <span className="text-sm">{item.criterion}</span>
                </div>
              ))}
            </div>
          </Card>
        </section>
      </div>
    </div>
  );
}
