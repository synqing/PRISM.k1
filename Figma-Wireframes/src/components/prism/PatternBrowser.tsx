import { useState } from "react";
import { Input } from "../ui/input";
import { Button } from "../ui/button";
import { Badge } from "../ui/badge";
import { Tabs, TabsContent, TabsList, TabsTrigger } from "../ui/tabs";
import { Search, Upload } from "lucide-react";

const MOCK_PATTERNS = [
  { id: 1, name: 'Ocean Sunrise', category: 'Nature', duration: '7.4s', size: 220 },
  { id: 2, name: 'Rainbow Wave', category: 'Abstract', duration: '5.0s', size: 156 },
  { id: 3, name: 'Fire Dance', category: 'Nature', duration: '10.0s', size: 312 },
  { id: 4, name: 'Neon Pulse', category: 'Geometric', duration: '4.2s', size: 98 },
  { id: 5, name: 'Aurora', category: 'Nature', duration: '15.0s', size: 428 },
  { id: 6, name: 'Lightning Storm', category: 'Weather', duration: '8.0s', size: 245 },
  { id: 7, name: 'Plasma Flow', category: 'Abstract', duration: '12.0s', size: 367 },
  { id: 8, name: 'Sparkle Rain', category: 'Particle', duration: '6.0s', size: 189 },
];

const CATEGORIES = ['All', 'Nature', 'Abstract', 'Geometric', 'Weather', 'Particle'];

export function PatternBrowser() {
  const [searchQuery, setSearchQuery] = useState('');
  const [selectedCategory, setSelectedCategory] = useState('All');
  const [selectedPattern, setSelectedPattern] = useState<number | null>(null);

  const filteredPatterns = MOCK_PATTERNS.filter(pattern => {
    const matchesSearch = pattern.name.toLowerCase().includes(searchQuery.toLowerCase());
    const matchesCategory = selectedCategory === 'All' || pattern.category === selectedCategory;
    return matchesSearch && matchesCategory;
  });

  return (
    <div className="h-full flex flex-col bg-background">
      {/* Header */}
      <div className="border-b border-border glass backdrop-blur-xl p-6 space-y-4">
        <div className="flex items-center justify-between">
          <div>
            <h1 className="text-3xl font-semibold tracking-tight">Pattern Browser</h1>
            <p className="text-muted-foreground mt-1">Browse and sync patterns to your device</p>
          </div>
          <Button className="gap-2 hover-lift active-press glow-primary shadow-elevation-1">
            <Upload className="w-4 h-4" />
            Sync Selected
          </Button>
        </div>

        <div className="flex gap-4">
          <div className="flex-1 relative">
            <Search className="absolute left-3 top-1/2 -translate-y-1/2 w-4 h-4 text-muted-foreground" />
            <Input
              placeholder="Search patterns..."
              value={searchQuery}
              onChange={(e) => setSearchQuery(e.target.value)}
              className="pl-9"
            />
          </div>
        </div>
      </div>

      {/* Categories */}
      <div className="border-b border-border px-6 py-3">
        <Tabs value={selectedCategory} onValueChange={setSelectedCategory}>
          <TabsList>
            {CATEGORIES.map(category => (
              <TabsTrigger key={category} value={category}>
                {category}
              </TabsTrigger>
            ))}
          </TabsList>
        </Tabs>
      </div>

      {/* Pattern Grid */}
      <div className="flex-1 overflow-auto p-6">
        <div className="grid grid-cols-4 gap-4">
          {filteredPatterns.map((pattern) => (
            <div
              key={pattern.id}
              onClick={() => setSelectedPattern(pattern.id)}
              className={`group cursor-pointer border rounded-xl overflow-hidden transition-all hover-lift ${
                selectedPattern === pattern.id
                  ? 'border-primary ring-2 ring-primary ring-offset-2 ring-offset-background shadow-elevation-2 glow-primary'
                  : 'border-border hover:border-primary/50 shadow-sm hover:shadow-elevation-1'
              }`}
            >
              {/* Thumbnail */}
              <div className="aspect-video bg-gradient-to-br from-primary/10 via-accent/10 to-primary/10 relative overflow-hidden">
                {/* Animated Pattern Preview */}
                <div className="absolute inset-0">
                  <div className="w-full h-full gradient-animated opacity-40" />
                  
                  {/* LED Grid Preview */}
                  <div className="absolute inset-0 p-4 flex items-center justify-center">
                    <div className="w-3/4 h-2/3 grid grid-cols-6 grid-rows-4 gap-1">
                      {Array.from({ length: 24 }).map((_, i) => (
                        <div 
                          key={i} 
                          className="rounded-sm bg-gradient-to-br from-primary/60 to-accent/60 shadow-sm"
                          style={{
                            animation: `pulse ${1.5 + (i % 4) * 0.2}s ease-in-out infinite`,
                            animationDelay: `${(i % 6) * 0.1}s`
                          }}
                        />
                      ))}
                    </div>
                  </div>
                </div>
                
                {/* Selection Indicator */}
                {selectedPattern === pattern.id && (
                  <div className="absolute top-3 right-3 w-7 h-7 bg-primary rounded-full flex items-center justify-center shadow-elevation-2 glow-primary">
                    <div className="w-4 h-4 border-2 border-primary-foreground rounded-full" />
                  </div>
                )}
                
                {/* Hover Overlay */}
                <div className="absolute inset-0 bg-gradient-to-t from-black/40 via-transparent to-transparent opacity-0 group-hover:opacity-100 transition-opacity" />
              </div>

              {/* Info */}
              <div className="p-3 space-y-2">
                <div className="font-medium">{pattern.name}</div>
                <div className="flex items-center justify-between text-xs text-muted-foreground">
                  <Badge variant="secondary" className="text-xs">
                    {pattern.category}
                  </Badge>
                  <span className="font-mono">{pattern.duration}</span>
                </div>
                <div className="text-xs text-muted-foreground font-mono">
                  {pattern.size} KB
                </div>
              </div>
            </div>
          ))}
        </div>
      </div>
    </div>
  );
}
