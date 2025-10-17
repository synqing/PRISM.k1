import { TopBar } from "./TopBar";
import { Button } from "../ui/button";
import { Badge } from "../ui/badge";
import { Progress } from "../ui/progress";
import { Table, TableBody, TableCell, TableHead, TableHeader, TableRow } from "../ui/table";
import { ScrollArea } from "../ui/scroll-area";
import { Radio, Trash2, Play, Download } from "lucide-react";

const MOCK_DEVICES = [
  {
    name: 'K1-Living Room',
    ip: '192.168.1.100',
    leds: 144,
    storage: { used: 187, total: 256 },
    patterns: [
      { name: 'Ocean Sunrise', size: 92, duration: '7.4s' },
      { name: 'Rainbow Wave', size: 65, duration: '5.0s' },
      { name: 'Fire Dance', size: 30, duration: '3.2s' },
    ]
  },
  {
    name: 'K1-Bedroom',
    ip: '192.168.1.101',
    leds: 96,
    storage: { used: 45, total: 256 },
    patterns: [
      { name: 'Aurora', size: 45, duration: '6.0s' },
    ]
  }
];

export function DeviceManagerScreen() {
  return (
    <div className="h-screen flex flex-col bg-background dark">
      <TopBar connected={true} fps="Auto" quality="HQ" />
      
      <div className="flex-1 overflow-auto">
        <div className="max-w-6xl mx-auto p-8 space-y-6">
          {/* Header */}
          <div className="flex items-center justify-between">
            <div>
              <h1 className="text-2xl">Device Manager</h1>
              <p className="text-muted-foreground">Manage your K1-Lightwave devices and storage</p>
            </div>
            <Button className="gap-2">
              <Radio className="w-4 h-4" />
              Discover Devices
            </Button>
          </div>

          {/* Devices */}
          <div className="space-y-4">
            {MOCK_DEVICES.map((device) => (
              <div key={device.name} className="border border-border rounded-lg overflow-hidden">
                {/* Device Header */}
                <div className="bg-card p-4 border-b border-border">
                  <div className="flex items-start justify-between mb-4">
                    <div>
                      <h3 className="font-medium mb-1">{device.name}</h3>
                      <div className="flex items-center gap-4 text-sm text-muted-foreground">
                        <span>{device.ip}</span>
                        <Badge variant="outline">{device.leds} LEDs</Badge>
                      </div>
                    </div>
                    <div className="flex gap-2">
                      <Button variant="outline" size="sm" className="gap-2">
                        <Play className="w-4 h-4" />
                        Play
                      </Button>
                      <Button variant="outline" size="sm" className="gap-2">
                        <Download className="w-4 h-4" />
                        Sync
                      </Button>
                    </div>
                  </div>

                  {/* Storage Meter */}
                  <div className="space-y-2">
                    <div className="flex justify-between text-sm">
                      <span>Storage</span>
                      <span className="font-mono">
                        {device.storage.used} / {device.storage.total} KB
                      </span>
                    </div>
                    <Progress
                      value={(device.storage.used / device.storage.total) * 100}
                      className="h-2"
                    />
                    <div className="text-xs text-muted-foreground">
                      {device.storage.total - device.storage.used} KB available
                    </div>
                  </div>
                </div>

                {/* Patterns List */}
                <div className="bg-background">
                  <div className="px-4 py-3 border-b border-border flex items-center justify-between">
                    <h4 className="text-sm">Stored Patterns ({device.patterns.length})</h4>
                    <Button variant="ghost" size="sm" className="gap-2 text-destructive">
                      <Trash2 className="w-3.5 h-3.5" />
                      Delete Selected
                    </Button>
                  </div>
                  <Table>
                    <TableHeader>
                      <TableRow>
                        <TableHead className="w-12">
                          <input type="checkbox" className="rounded" />
                        </TableHead>
                        <TableHead>Name</TableHead>
                        <TableHead>Duration</TableHead>
                        <TableHead className="text-right">Size</TableHead>
                        <TableHead className="w-24"></TableHead>
                      </TableRow>
                    </TableHeader>
                    <TableBody>
                      {device.patterns.map((pattern, i) => (
                        <TableRow key={i}>
                          <TableCell>
                            <input type="checkbox" className="rounded" />
                          </TableCell>
                          <TableCell className="font-medium">{pattern.name}</TableCell>
                          <TableCell className="font-mono text-sm text-muted-foreground">
                            {pattern.duration}
                          </TableCell>
                          <TableCell className="text-right font-mono text-sm">
                            {pattern.size} KB
                          </TableCell>
                          <TableCell>
                            <Button variant="ghost" size="sm" className="gap-2">
                              <Play className="w-3.5 h-3.5" />
                              Play
                            </Button>
                          </TableCell>
                        </TableRow>
                      ))}
                    </TableBody>
                  </Table>
                </div>
              </div>
            ))}
          </div>
        </div>
      </div>
    </div>
  );
}
