import { Card } from "../ui/card";
import { Badge } from "../ui/badge";

export function TokensReference() {
  const colors = {
    brand: [
      { name: 'brand-500', hex: '#5B8CFF', var: '--prism-brand-500' },
      { name: 'brand-600', hex: '#3F6EF0', var: '--prism-brand-600' },
      { name: 'brand-700', hex: '#224CCD', var: '--prism-brand-700' },
    ],
    accent: [
      { name: 'accent-500', hex: '#FF7A59', var: '--prism-accent-500' },
      { name: 'accent-600', hex: '#F55E3D', var: '--prism-accent-600' },
    ],
    grays: [
      { name: 'gray-900', hex: '#0D0F14', var: '--prism-gray-900' },
      { name: 'gray-800', hex: '#11141A', var: '--prism-gray-800' },
      { name: 'gray-700', hex: '#161A22', var: '--prism-gray-700' },
      { name: 'gray-600', hex: '#1C212B', var: '--prism-gray-600' },
      { name: 'gray-500', hex: '#242B37', var: '--prism-gray-500' },
      { name: 'gray-400', hex: '#2D3748', var: '--prism-gray-400' },
      { name: 'gray-300', hex: '#4A5568', var: '--prism-gray-300' },
      { name: 'gray-200', hex: '#A0AEC0', var: '--prism-gray-200' },
      { name: 'gray-100', hex: '#EDF2F7', var: '--prism-gray-100' },
    ],
    semantic: [
      { name: 'success', hex: '#22C55E', var: '--prism-success' },
      { name: 'warn', hex: '#F59E0B', var: '--prism-warn' },
      { name: 'error', hex: '#EF4444', var: '--prism-error' },
    ],
  };

  const spacing = [
    { name: 'xs', value: '4px', use: 'Micro spacing' },
    { name: 'sm', value: '8px', use: 'Base unit' },
    { name: 'md', value: '12px', use: 'Compact elements' },
    { name: 'base', value: '16px', use: 'Standard spacing' },
    { name: 'lg', value: '24px', use: 'Section gaps' },
    { name: 'xl', value: '32px', use: 'Major sections' },
    { name: '2xl', value: '48px', use: 'Page margins' },
  ];

  const typography = [
    { element: 'h1', size: '32px', weight: '600', family: 'Inter', use: 'Page titles' },
    { element: 'h2', size: '28px', weight: '600', family: 'Inter', use: 'Section headers' },
    { element: 'h3', size: '24px', weight: '500', family: 'Inter', use: 'Subsections' },
    { element: 'h4', size: '20px', weight: '500', family: 'Inter', use: 'Card titles' },
    { element: 'body-lg', size: '16px', weight: '400', family: 'Inter', use: 'Body text' },
    { element: 'body', size: '14px', weight: '400', family: 'Inter', use: 'UI text' },
    { element: 'body-sm', size: '12px', weight: '400', family: 'Inter', use: 'Captions' },
    { element: 'code', size: '14px', weight: '400', family: 'JetBrains Mono', use: 'Code/metrics' },
  ];

  const radii = [
    { name: 'sm', value: '4px', use: 'Buttons, badges' },
    { name: 'md', value: '8px', use: 'Cards, inputs' },
    { name: 'lg', value: '12px', use: 'Panels, modals' },
  ];

  const elevations = [
    { level: '1', shadow: '0 1px 1px rgba(0,0,0,0.05)', use: 'Subtle lift' },
    { level: '2', shadow: '0 4px 10px rgba(0,0,0,0.08)', use: 'Cards' },
    { level: '3', shadow: '0 10px 24px rgba(0,0,0,0.12)', use: 'Modals, dropdowns' },
  ];

  return (
    <div className="min-h-screen bg-background dark p-8">
      <div className="max-w-7xl mx-auto space-y-12">
        {/* Header */}
        <div>
          <h1 className="text-3xl mb-2">PRISM Design Tokens</h1>
          <p className="text-muted-foreground">
            Complete design system for K1-Lightwave Studio
          </p>
        </div>

        {/* Grid & Layout */}
        <section className="space-y-4">
          <div>
            <h2 className="text-xl mb-1">Grid & Layout</h2>
            <p className="text-sm text-muted-foreground">12-column grid, 1440×900 base</p>
          </div>
          <Card className="p-6 space-y-4">
            <div className="grid grid-cols-3 gap-4 text-sm">
              <div>
                <div className="text-muted-foreground mb-1">Base Width</div>
                <div className="font-mono">1440px</div>
              </div>
              <div>
                <div className="text-muted-foreground mb-1">Side Gutters</div>
                <div className="font-mono">80px</div>
              </div>
              <div>
                <div className="text-muted-foreground mb-1">Column Gap</div>
                <div className="font-mono">24px</div>
              </div>
              <div>
                <div className="text-muted-foreground mb-1">Top Bar</div>
                <div className="font-mono">48px</div>
              </div>
              <div>
                <div className="text-muted-foreground mb-1">Sidebar</div>
                <div className="font-mono">280px</div>
              </div>
              <div>
                <div className="text-muted-foreground mb-1">Inspector</div>
                <div className="font-mono">320px</div>
              </div>
            </div>
          </Card>
        </section>

        {/* Colors */}
        <section className="space-y-4">
          <h2 className="text-xl">Colors</h2>
          
          <div className="space-y-6">
            <div>
              <h3 className="mb-3">Brand</h3>
              <div className="grid grid-cols-3 gap-3">
                {colors.brand.map((color) => (
                  <div key={color.name} className="space-y-2">
                    <div
                      className="h-20 rounded-lg border border-border"
                      style={{ backgroundColor: color.hex }}
                    />
                    <div className="space-y-1">
                      <div className="text-sm font-medium">{color.name}</div>
                      <div className="text-xs text-muted-foreground font-mono">{color.hex}</div>
                      <div className="text-xs text-muted-foreground">{color.var}</div>
                    </div>
                  </div>
                ))}
              </div>
            </div>

            <div>
              <h3 className="mb-3">Accent</h3>
              <div className="grid grid-cols-3 gap-3">
                {colors.accent.map((color) => (
                  <div key={color.name} className="space-y-2">
                    <div
                      className="h-20 rounded-lg border border-border"
                      style={{ backgroundColor: color.hex }}
                    />
                    <div className="space-y-1">
                      <div className="text-sm font-medium">{color.name}</div>
                      <div className="text-xs text-muted-foreground font-mono">{color.hex}</div>
                      <div className="text-xs text-muted-foreground">{color.var}</div>
                    </div>
                  </div>
                ))}
              </div>
            </div>

            <div>
              <h3 className="mb-3">Neutrals</h3>
              <div className="grid grid-cols-9 gap-2">
                {colors.grays.map((color) => (
                  <div key={color.name} className="space-y-2">
                    <div
                      className="h-16 rounded border border-border"
                      style={{ backgroundColor: color.hex }}
                    />
                    <div className="text-xs font-medium">{color.name.split('-')[1]}</div>
                    <div className="text-xs text-muted-foreground font-mono">{color.hex}</div>
                  </div>
                ))}
              </div>
            </div>

            <div>
              <h3 className="mb-3">Semantic</h3>
              <div className="grid grid-cols-3 gap-3">
                {colors.semantic.map((color) => (
                  <div key={color.name} className="space-y-2">
                    <div
                      className="h-20 rounded-lg border border-border"
                      style={{ backgroundColor: color.hex }}
                    />
                    <div className="space-y-1">
                      <div className="text-sm font-medium capitalize">{color.name}</div>
                      <div className="text-xs text-muted-foreground font-mono">{color.hex}</div>
                      <div className="text-xs text-muted-foreground">{color.var}</div>
                    </div>
                  </div>
                ))}
              </div>
            </div>
          </div>
        </section>

        {/* Typography */}
        <section className="space-y-4">
          <h2 className="text-xl">Typography</h2>
          <Card className="p-6 overflow-x-auto">
            <table className="w-full text-sm">
              <thead>
                <tr className="border-b border-border">
                  <th className="text-left py-2 px-3">Element</th>
                  <th className="text-left py-2 px-3">Size</th>
                  <th className="text-left py-2 px-3">Weight</th>
                  <th className="text-left py-2 px-3">Family</th>
                  <th className="text-left py-2 px-3">Use Case</th>
                  <th className="text-left py-2 px-3">Example</th>
                </tr>
              </thead>
              <tbody>
                {typography.map((type) => (
                  <tr key={type.element} className="border-b border-border/50">
                    <td className="py-3 px-3">
                      <Badge variant="outline">{type.element}</Badge>
                    </td>
                    <td className="py-3 px-3 font-mono">{type.size}</td>
                    <td className="py-3 px-3">{type.weight}</td>
                    <td className="py-3 px-3">{type.family}</td>
                    <td className="py-3 px-3 text-muted-foreground">{type.use}</td>
                    <td
                      className="py-3 px-3"
                      style={{
                        fontSize: type.size,
                        fontWeight: type.weight,
                        fontFamily: type.family === 'JetBrains Mono' ? 'JetBrains Mono, monospace' : 'Inter, sans-serif'
                      }}
                    >
                      Sample Text
                    </td>
                  </tr>
                ))}
              </tbody>
            </table>
          </Card>
        </section>

        {/* Spacing */}
        <section className="space-y-4">
          <h2 className="text-xl">Spacing (8pt System)</h2>
          <Card className="p-6">
            <div className="space-y-3">
              {spacing.map((space) => (
                <div key={space.name} className="flex items-center gap-4">
                  <div className="w-24">
                    <Badge variant="outline">{space.name}</Badge>
                  </div>
                  <div className="w-20 font-mono text-sm">{space.value}</div>
                  <div
                    className="h-8 bg-primary/20 border-l-2 border-primary"
                    style={{ width: space.value }}
                  />
                  <div className="text-sm text-muted-foreground">{space.use}</div>
                </div>
              ))}
            </div>
          </Card>
        </section>

        {/* Border Radius */}
        <section className="space-y-4">
          <h2 className="text-xl">Border Radius</h2>
          <Card className="p-6">
            <div className="grid grid-cols-3 gap-6">
              {radii.map((radius) => (
                <div key={radius.name} className="space-y-3">
                  <div
                    className="h-24 bg-primary/20 border-2 border-primary"
                    style={{ borderRadius: radius.value }}
                  />
                  <div>
                    <div className="text-sm font-medium">{radius.name}</div>
                    <div className="text-xs text-muted-foreground font-mono">{radius.value}</div>
                    <div className="text-xs text-muted-foreground">{radius.use}</div>
                  </div>
                </div>
              ))}
            </div>
          </Card>
        </section>

        {/* Elevation */}
        <section className="space-y-4">
          <h2 className="text-xl">Elevation</h2>
          <Card className="p-6 bg-muted/30">
            <div className="grid grid-cols-3 gap-6">
              {elevations.map((elevation) => (
                <div key={elevation.level} className="space-y-3">
                  <div
                    className="h-32 bg-card rounded-lg flex items-center justify-center"
                    style={{ boxShadow: elevation.shadow }}
                  >
                    <span className="text-2xl">Level {elevation.level}</span>
                  </div>
                  <div>
                    <div className="text-sm font-medium">Elevation {elevation.level}</div>
                    <div className="text-xs text-muted-foreground">{elevation.use}</div>
                    <div className="text-xs text-muted-foreground font-mono mt-1">
                      {elevation.shadow}
                    </div>
                  </div>
                </div>
              ))}
            </div>
          </Card>
        </section>
      </div>
    </div>
  );
}
