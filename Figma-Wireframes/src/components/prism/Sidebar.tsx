import { useState } from "react";
import { Tabs, TabsContent, TabsList, TabsTrigger } from "../ui/tabs";
import { Input } from "../ui/input";
import { Button } from "../ui/button";
import { Badge } from "../ui/badge";
import { ScrollArea } from "../ui/scroll-area";
import { 
  Search, 
  Sparkles, 
  Palette, 
  FileText, 
  Monitor, 
  Trash2,
  List,
  Radio
} from "lucide-react";

const MOCK_EFFECTS = [
  'Rainbow Wave',
  'Color Breathe',
  'Sparkle',
  'Fire',
  'Ocean Sunrise',
  'Lightning',
  'Plasma',
  'Aurora'
];

const MOCK_PALETTES = [
  { name: 'Sunset', colors: ['#FF6B35', '#F7931E', '#FDC830'] },
  { name: 'Ocean', colors: ['#667EEA', '#764BA2', '#F093FB'] },
  { name: 'Forest', colors: ['#134E5E', '#71B280'] },
  { name: 'Neon', colors: ['#FF006E', '#8338EC', '#3A86FF'] }
];

const MOCK_DEVICES = [
  { name: 'K1-Living Room', ip: '192.168.1.100', leds: 144, storage: { used: 45, total: 256 } },
  { name: 'K1-Bedroom', ip: '192.168.1.101', leds: 96, storage: { used: 12, total: 256 } }
];

export function Sidebar() {
  const [tab, setTab] = useState('library');
  const [searchQuery, setSearchQuery] = useState('');

  return (
    <div className="w-[280px] border-r border-border bg-card/50 backdrop-blur-sm h-full flex flex-col">
      <Tabs value={tab} onValueChange={setTab} className="flex-1 flex flex-col">
        <TabsList className="w-full grid grid-cols-2 rounded-none border-b border-border h-12 bg-muted/30">
          <TabsTrigger value="library" className="gap-2 data-[state=active]:shadow-sm">
            <Sparkles className="w-4 h-4" />
            Library
          </TabsTrigger>
          <TabsTrigger value="devices" className="gap-2 data-[state=active]:shadow-sm">
            <Monitor className="w-4 h-4" />
            Devices
          </TabsTrigger>
        </TabsList>

        <TabsContent value="library" className="flex-1 flex flex-col m-0 p-4 gap-4">
          {/* Search */}
          <div className="relative">
            <Search className="absolute left-3 top-1/2 -translate-y-1/2 w-4 h-4 text-muted-foreground" />
            <Input
              placeholder="Search effects..."
              value={searchQuery}
              onChange={(e) => setSearchQuery(e.target.value)}
              className="pl-9"
            />
          </div>

          {/* Effects */}
          <div className="space-y-2">
            <h4 className="text-muted-foreground px-1">Effects</h4>
            <ScrollArea className="h-[200px]">
              <div className="space-y-1">
                {MOCK_EFFECTS.map((effect) => (
                  <button
                    key={effect}
                    className="w-full text-left px-3 py-2 rounded-lg hover:bg-accent/10 transition-all hover-lift active-press text-sm"
                  >
                    {effect}
                  </button>
                ))}
              </div>
            </ScrollArea>
          </div>

          {/* Palettes */}
          <div className="space-y-2">
            <h4 className="text-muted-foreground px-1">Palettes</h4>
            <div className="grid grid-cols-2 gap-2">
              {MOCK_PALETTES.map((palette) => (
                <div
                  key={palette.name}
                  className="p-2.5 border border-border rounded-lg cursor-pointer hover:border-primary/50 transition-all hover-lift active-press shadow-sm hover:shadow-elevation-1"
                >
                  <div className="flex gap-1 mb-2">
                    {palette.colors.map((color, i) => (
                      <div
                        key={i}
                        className="flex-1 h-7 rounded shadow-sm"
                        style={{ backgroundColor: color }}
                      />
                    ))}
                  </div>
                  <span className="text-xs font-medium">{palette.name}</span>
                </div>
              ))}
            </div>
          </div>
        </TabsContent>

        <TabsContent value="devices" className="flex-1 flex flex-col m-0 p-4 gap-4">
          <Button className="w-full gap-2">
            <Radio className="w-4 h-4" />
            Discover Devices
          </Button>

          <div className="space-y-2 flex-1">
            <h4 className="text-muted-foreground px-1">Connected</h4>
            <ScrollArea className="h-full">
              <div className="space-y-2">
                {MOCK_DEVICES.map((device) => (
                  <div
                    key={device.name}
                    className="p-3 border border-border rounded-lg hover:border-primary/50 transition-all cursor-pointer hover-lift shadow-sm hover:shadow-elevation-1"
                  >
                    <div className="flex items-start justify-between mb-3">
                      <div>
                        <div className="font-medium text-sm">{device.name}</div>
                        <div className="text-xs text-muted-foreground font-mono mt-0.5">{device.ip}</div>
                      </div>
                      <Badge variant="outline" className="text-xs">
                        {device.leds} LEDs
                      </Badge>
                    </div>
                    <div className="space-y-1.5">
                      <div className="flex justify-between text-xs">
                        <span className="text-muted-foreground">Storage</span>
                        <span className="font-mono font-medium">{device.storage.used}/{device.storage.total} KB</span>
                      </div>
                      <div className="w-full h-1.5 bg-muted rounded-full overflow-hidden">
                        <div
                          className="h-full bg-gradient-to-r from-primary to-primary/80 transition-all"
                          style={{ width: `${(device.storage.used / device.storage.total) * 100}%` }}
                        />
                      </div>
                    </div>
                  </div>
                ))}
              </div>
            </ScrollArea>
          </div>

          <div className="flex gap-2">
            <Button variant="outline" size="sm" className="flex-1 gap-2">
              <List className="w-4 h-4" />
              List
            </Button>
            <Button variant="outline" size="sm" className="flex-1 gap-2">
              <Trash2 className="w-4 h-4" />
              Delete
            </Button>
          </div>
        </TabsContent>
      </Tabs>
    </div>
  );
}
