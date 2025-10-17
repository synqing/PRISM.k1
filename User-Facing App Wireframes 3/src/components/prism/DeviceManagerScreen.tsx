import { useState } from "react";
import { TopBar } from "./TopBar";
import { Button } from "../ui/button";
import { Badge } from "../ui/badge";
import { Progress } from "../ui/progress";
import { Input } from "../ui/input";
import { Table, TableBody, TableCell, TableHead, TableHeader, TableRow } from "../ui/table";
import { Dialog, DialogContent, DialogDescription, DialogFooter, DialogHeader, DialogTitle } from "../ui/dialog";
import { Label } from "../ui/label";
import { Radio, Trash2, Play, Download, Plus, Wifi, WifiOff, RefreshCw, AlertCircle } from "lucide-react";
import { useAppState } from "../../lib/appState";
import { toast } from "sonner@2.0.3";
import { Checkbox } from "../ui/checkbox";

export function DeviceManagerScreen() {
  const { devices, addDevice, removeDevice, updateDevice, patterns } = useAppState();
  const [showAddDevice, setShowAddDevice] = useState(false);
  const [newDeviceIp, setNewDeviceIp] = useState('');
  const [newDeviceName, setNewDeviceName] = useState('');
  const [selectedDevicePatterns, setSelectedDevicePatterns] = useState<Record<string, string[]>>({});
  const [discovering, setDiscovering] = useState(false);

  const handleDiscover = () => {
    setDiscovering(true);
    toast.info('Scanning network for K1-Lightwave devices...');
    
    // Simulate device discovery
    setTimeout(() => {
      setDiscovering(false);
      toast.success('Network scan complete');
    }, 2000);
  };

  const handleAddDevice = () => {
    if (!newDeviceIp || !newDeviceName) {
      toast.error('Please fill in all fields');
      return;
    }

    const newDevice = {
      id: `device-${Date.now()}`,
      name: newDeviceName,
      ip: newDeviceIp,
      ledCount: 320,
      storageUsed: 0,
      storageTotal: 256000,
      status: 'online' as const,
      lastSeen: new Date()
    };

    addDevice(newDevice);
    toast.success(`Added ${newDeviceName}`);
    setShowAddDevice(false);
    setNewDeviceIp('');
    setNewDeviceName('');
  };

  const handleDeleteDevice = (deviceId: string) => {
    const device = devices.find(d => d.id === deviceId);
    if (device) {
      removeDevice(deviceId);
      toast.success(`Removed ${device.name}`);
    }
  };

  const handlePlayPattern = (deviceId: string, patternName: string) => {
    const device = devices.find(d => d.id === deviceId);
    toast.success(`Playing "${patternName}" on ${device?.name}`);
  };

  const handleSyncToDevice = (deviceId: string) => {
    const device = devices.find(d => d.id === deviceId);
    const selectedPatternIds = selectedDevicePatterns[deviceId] || [];
    
    if (selectedPatternIds.length === 0) {
      toast.error('No patterns selected');
      return;
    }

    toast.info(`Syncing ${selectedPatternIds.length} pattern(s) to ${device?.name}...`);
    
    // Simulate sync
    setTimeout(() => {
      const totalSize = selectedPatternIds.reduce((sum, id) => {
        const pattern = patterns.find(p => p.id === id);
        return sum + (pattern?.size || 0);
      }, 0);

      updateDevice(deviceId, {
        storageUsed: (device?.storageUsed || 0) + totalSize
      });

      toast.success('Sync complete!');
      setSelectedDevicePatterns({ ...selectedDevicePatterns, [deviceId]: [] });
    }, 1500);
  };

  const handleDeletePattern = (deviceId: string) => {
    const device = devices.find(d => d.id === deviceId);
    const selectedPatternIds = selectedDevicePatterns[deviceId] || [];

    if (selectedPatternIds.length === 0) {
      toast.error('No patterns selected');
      return;
    }

    const totalSize = selectedPatternIds.reduce((sum, id) => {
      const pattern = patterns.find(p => p.id === id);
      return sum + (pattern?.size || 0);
    }, 0);

    updateDevice(deviceId, {
      storageUsed: Math.max(0, (device?.storageUsed || 0) - totalSize)
    });

    toast.success(`Deleted ${selectedPatternIds.length} pattern(s)`);
    setSelectedDevicePatterns({ ...selectedDevicePatterns, [deviceId]: [] });
  };

  const togglePatternSelection = (deviceId: string, patternId: string) => {
    const current = selectedDevicePatterns[deviceId] || [];
    const newSelection = current.includes(patternId)
      ? current.filter(id => id !== patternId)
      : [...current, patternId];
    
    setSelectedDevicePatterns({ ...selectedDevicePatterns, [deviceId]: newSelection });
  };

  const selectAllPatterns = (deviceId: string) => {
    const allPatternIds = patterns.map(p => p.id);
    setSelectedDevicePatterns({ ...selectedDevicePatterns, [deviceId]: allPatternIds });
  };

  const deselectAllPatterns = (deviceId: string) => {
    setSelectedDevicePatterns({ ...selectedDevicePatterns, [deviceId]: [] });
  };

  return (
    <div className="h-screen flex flex-col bg-background">
      <TopBar connected={true} fps="Auto" quality="HQ" />
      
      <div className="flex-1 overflow-auto">
        <div className="max-w-6xl mx-auto p-8 space-y-6">
          {/* Header */}
          <div className="flex items-center justify-between">
            <div>
              <h1 className="text-3xl font-semibold tracking-tight">Device Manager</h1>
              <p className="text-muted-foreground mt-1">
                {devices.length} device{devices.length !== 1 ? 's' : ''} connected
              </p>
            </div>
            <div className="flex gap-2">
              <Button 
                variant="outline" 
                className="gap-2 hover-lift active-press"
                onClick={handleDiscover}
                disabled={discovering}
              >
                <Radio className={`w-4 h-4 ${discovering ? 'animate-pulse' : ''}`} />
                {discovering ? 'Scanning...' : 'Auto-Discover'}
              </Button>
              <Button 
                className="gap-2 hover-lift active-press glow-primary"
                onClick={() => setShowAddDevice(true)}
              >
                <Plus className="w-4 h-4" />
                Add Device
              </Button>
            </div>
          </div>

          {/* Devices */}
          {devices.length === 0 ? (
            <div className="border border-dashed border-border rounded-lg p-12 text-center space-y-4">
              <div className="w-16 h-16 mx-auto rounded-full bg-muted/50 flex items-center justify-center">
                <WifiOff className="w-8 h-8 text-muted-foreground" />
              </div>
              <div>
                <div className="font-medium mb-1">No devices found</div>
                <p className="text-sm text-muted-foreground">
                  Click "Auto-Discover" to scan your network or add a device manually
                </p>
              </div>
              <div className="flex gap-2 justify-center">
                <Button variant="outline" onClick={handleDiscover}>
                  <Radio className="w-4 h-4 mr-2" />
                  Auto-Discover
                </Button>
                <Button onClick={() => setShowAddDevice(true)}>
                  <Plus className="w-4 h-4 mr-2" />
                  Add Manually
                </Button>
              </div>
            </div>
          ) : (
            <div className="space-y-4">
              {devices.map((device) => {
                const storagePercent = (device.storageUsed / device.storageTotal) * 100;
                const selectedPatterns = selectedDevicePatterns[device.id] || [];
                const allSelected = selectedPatterns.length === patterns.length && patterns.length > 0;
                const someSelected = selectedPatterns.length > 0 && selectedPatterns.length < patterns.length;

                return (
                  <div key={device.id} className="border border-border rounded-xl overflow-hidden shadow-sm hover:shadow-elevation-1 transition-shadow">
                    {/* Device Header */}
                    <div className="bg-card p-5 border-b border-border">
                      <div className="flex items-start justify-between mb-4">
                        <div className="flex items-start gap-3">
                          <div className={`w-3 h-3 rounded-full mt-1.5 ${
                            device.status === 'online' 
                              ? 'bg-green-500 shadow-[0_0_8px_rgba(34,197,94,0.6)]' 
                              : 'bg-red-500'
                          }`} />
                          <div>
                            <h3 className="font-semibold mb-1">{device.name}</h3>
                            <div className="flex items-center gap-4 text-sm text-muted-foreground">
                              <span className="font-mono flex items-center gap-1">
                                <Wifi className="w-3 h-3" />
                                {device.ip}
                              </span>
                              <Badge variant="outline">{device.ledCount} LEDs</Badge>
                              <span className="text-xs">
                                Last seen {device.lastSeen.toLocaleTimeString()}
                              </span>
                            </div>
                          </div>
                        </div>
                        <div className="flex gap-2">
                          <Button 
                            variant="outline" 
                            size="sm" 
                            className="gap-2 hover-lift"
                            onClick={() => {
                              updateDevice(device.id, { lastSeen: new Date() });
                              toast.success('Device refreshed');
                            }}
                          >
                            <RefreshCw className="w-4 h-4" />
                            Refresh
                          </Button>
                          <Button 
                            variant="outline" 
                            size="sm" 
                            className="gap-2 hover-lift"
                            onClick={() => handleSyncToDevice(device.id)}
                          >
                            <Download className="w-4 h-4" />
                            Sync Selected
                          </Button>
                          <Button 
                            variant="outline" 
                            size="sm" 
                            className="gap-2 hover-lift text-destructive hover:text-destructive"
                            onClick={() => handleDeleteDevice(device.id)}
                          >
                            <Trash2 className="w-4 h-4" />
                          </Button>
                        </div>
                      </div>

                      {/* Storage Meter */}
                      <div className="space-y-2">
                        <div className="flex justify-between text-sm">
                          <span className="text-muted-foreground">Storage</span>
                          <span className="font-mono">
                            {Math.round(device.storageUsed / 1024)} / {Math.round(device.storageTotal / 1024)} KB
                          </span>
                        </div>
                        <Progress
                          value={storagePercent}
                          className={`h-2 ${storagePercent > 90 ? 'bg-red-500/20' : ''}`}
                        />
                        <div className="flex justify-between text-xs">
                          <span className="text-muted-foreground">
                            {Math.round((device.storageTotal - device.storageUsed) / 1024)} KB available
                          </span>
                          {storagePercent > 90 && (
                            <span className="text-destructive flex items-center gap-1">
                              <AlertCircle className="w-3 h-3" />
                              Storage almost full
                            </span>
                          )}
                        </div>
                      </div>
                    </div>

                    {/* Patterns List */}
                    <div className="bg-background">
                      <div className="px-5 py-3 border-b border-border flex items-center justify-between">
                        <h4 className="text-sm font-medium">
                          Available Patterns ({patterns.length})
                          {selectedPatterns.length > 0 && (
                            <span className="text-primary ml-2">
                              — {selectedPatterns.length} selected
                            </span>
                          )}
                        </h4>
                        <div className="flex gap-2">
                          {selectedPatterns.length > 0 && (
                            <Button 
                              variant="ghost" 
                              size="sm" 
                              className="gap-2 text-destructive hover:text-destructive"
                              onClick={() => handleDeletePattern(device.id)}
                            >
                              <Trash2 className="w-3.5 h-3.5" />
                              Delete Selected
                            </Button>
                          )}
                        </div>
                      </div>
                      
                      {patterns.length === 0 ? (
                        <div className="p-8 text-center text-muted-foreground">
                          <p className="text-sm">No patterns in library</p>
                          <p className="text-xs mt-1">Create patterns in the Studio tab</p>
                        </div>
                      ) : (
                        <Table>
                          <TableHeader>
                            <TableRow>
                              <TableHead className="w-12">
                                <Checkbox
                                  checked={allSelected}
                                  onCheckedChange={(checked) => {
                                    if (checked) {
                                      selectAllPatterns(device.id);
                                    } else {
                                      deselectAllPatterns(device.id);
                                    }
                                  }}
                                  className={someSelected ? 'data-[state=checked]:bg-primary/50' : ''}
                                />
                              </TableHead>
                              <TableHead>Name</TableHead>
                              <TableHead>Duration</TableHead>
                              <TableHead className="text-right">Size</TableHead>
                              <TableHead className="w-24"></TableHead>
                            </TableRow>
                          </TableHeader>
                          <TableBody>
                            {patterns.map((pattern) => (
                              <TableRow key={pattern.id}>
                                <TableCell>
                                  <Checkbox
                                    checked={selectedPatterns.includes(pattern.id)}
                                    onCheckedChange={() => togglePatternSelection(device.id, pattern.id)}
                                  />
                                </TableCell>
                                <TableCell className="font-medium">{pattern.name}</TableCell>
                                <TableCell className="font-mono text-sm text-muted-foreground">
                                  {(pattern.duration / 1000).toFixed(1)}s
                                </TableCell>
                                <TableCell className="text-right font-mono text-sm">
                                  {Math.round(pattern.size / 1024)} KB
                                </TableCell>
                                <TableCell>
                                  <Button 
                                    variant="ghost" 
                                    size="sm" 
                                    className="gap-2 hover-lift"
                                    onClick={() => handlePlayPattern(device.id, pattern.name)}
                                  >
                                    <Play className="w-3.5 h-3.5" />
                                    Play
                                  </Button>
                                </TableCell>
                              </TableRow>
                            ))}
                          </TableBody>
                        </Table>
                      )}
                    </div>
                  </div>
                );
              })}
            </div>
          )}
        </div>
      </div>

      {/* Add Device Dialog */}
      <Dialog open={showAddDevice} onOpenChange={setShowAddDevice}>
        <DialogContent>
          <DialogHeader>
            <DialogTitle>Add Device Manually</DialogTitle>
            <DialogDescription>
              Enter the IP address and name for your K1-Lightwave device
            </DialogDescription>
          </DialogHeader>
          <div className="space-y-4 py-4">
            <div className="space-y-2">
              <Label htmlFor="device-name">Device Name</Label>
              <Input
                id="device-name"
                placeholder="e.g., K1-Living Room"
                value={newDeviceName}
                onChange={(e) => setNewDeviceName(e.target.value)}
              />
            </div>
            <div className="space-y-2">
              <Label htmlFor="device-ip">IP Address</Label>
              <Input
                id="device-ip"
                placeholder="e.g., 192.168.1.100"
                value={newDeviceIp}
                onChange={(e) => setNewDeviceIp(e.target.value)}
              />
              <p className="text-xs text-muted-foreground">
                Make sure the device is on the same network
              </p>
            </div>
          </div>
          <DialogFooter>
            <Button variant="outline" onClick={() => setShowAddDevice(false)}>
              Cancel
            </Button>
            <Button onClick={handleAddDevice}>
              Add Device
            </Button>
          </DialogFooter>
        </DialogContent>
      </Dialog>
    </div>
  );
}
