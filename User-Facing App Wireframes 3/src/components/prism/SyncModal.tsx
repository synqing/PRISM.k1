import { Dialog, DialogContent, DialogHeader, DialogTitle, DialogFooter } from "../ui/dialog";
import { Button } from "../ui/button";
import { Progress } from "../ui/progress";
import { Badge } from "../ui/badge";
import { Table, TableBody, TableCell, TableHead, TableHeader, TableRow } from "../ui/table";
import { Checkbox } from "../ui/checkbox";
import { Upload, AlertCircle, CheckCircle2 } from "lucide-react";

interface SyncModalProps {
  open: boolean;
  onOpenChange: (open: boolean) => void;
  uploading?: boolean;
  progress?: number;
}

const MOCK_SEGMENTS = [
  { name: 'Ocean Sunrise - Part 1', start: '0.0s', end: '4.0s', fps: 60, size: 128 },
  { name: 'Ocean Sunrise - Part 2', start: '4.0s', end: '7.4s', fps: 60, size: 92 }
];

export function SyncModal({ open, onOpenChange, uploading = false, progress = 0 }: SyncModalProps) {
  const totalSize = MOCK_SEGMENTS.reduce((sum, seg) => sum + seg.size, 0);
  const deviceTotal = 256;
  const deviceUsed = 45;
  const availableStorage = deviceTotal - deviceUsed;
  const willFit = totalSize <= availableStorage;
  const storagePercentage = ((deviceUsed + totalSize) / deviceTotal) * 100;

  return (
    <Dialog open={open} onOpenChange={onOpenChange}>
      <DialogContent className="max-w-3xl glass shadow-elevation-3 border-border/50">
        <DialogHeader>
          <DialogTitle className="flex items-center gap-2">
            <div className="w-8 h-8 rounded-lg bg-primary/10 flex items-center justify-center">
              <Upload className="w-4 h-4 text-primary" />
            </div>
            Sync to Device
          </DialogTitle>
        </DialogHeader>

        <div className="space-y-6">
          {/* Segment List */}
          <div>
            <div className="flex items-center justify-between mb-3">
              <h4 className="text-sm font-medium">Segments to Upload</h4>
              <Badge variant="outline" className="font-mono">
                {MOCK_SEGMENTS.length} segment{MOCK_SEGMENTS.length !== 1 ? 's' : ''}
              </Badge>
            </div>
            <div className="border border-border rounded-lg overflow-hidden shadow-sm">
              <Table>
                <TableHeader>
                  <TableRow className="bg-muted/50">
                    <TableHead>Name</TableHead>
                    <TableHead className="w-24">Start</TableHead>
                    <TableHead className="w-24">End</TableHead>
                    <TableHead className="w-20">FPS</TableHead>
                    <TableHead className="text-right w-24">Size</TableHead>
                  </TableRow>
                </TableHeader>
                <TableBody>
                  {MOCK_SEGMENTS.map((segment, i) => (
                    <TableRow key={i} className="hover:bg-accent/5">
                      <TableCell className="font-medium">{segment.name}</TableCell>
                      <TableCell className="font-mono text-sm text-muted-foreground">{segment.start}</TableCell>
                      <TableCell className="font-mono text-sm text-muted-foreground">{segment.end}</TableCell>
                      <TableCell>
                        <Badge variant="secondary" className="font-mono text-xs">
                          {segment.fps}
                        </Badge>
                      </TableCell>
                      <TableCell className="text-right font-mono text-sm">{segment.size} KB</TableCell>
                    </TableRow>
                  ))}
                  <TableRow className="bg-muted/30 font-semibold">
                    <TableCell colSpan={4}>Total</TableCell>
                    <TableCell className="text-right font-mono">{totalSize} KB</TableCell>
                  </TableRow>
                </TableBody>
              </Table>
            </div>
          </div>

          {/* Storage Meter */}
          <div className="space-y-3 p-4 bg-muted/30 rounded-lg border border-border">
            <div className="flex justify-between items-start">
              <div>
                <div className="text-sm font-medium mb-1">Device Storage</div>
                <div className="text-xs text-muted-foreground">
                  K1-Living Room • 192.168.1.100
                </div>
              </div>
              <div className="text-right">
                <div className="font-mono text-sm font-semibold">
                  {deviceUsed + totalSize} / {deviceTotal} KB
                </div>
                <div className="text-xs text-muted-foreground">
                  {Math.round(storagePercentage)}% used
                </div>
              </div>
            </div>
            
            <div className="relative h-4 bg-background rounded-full overflow-hidden border border-border">
              {/* Current usage */}
              <div
                className="absolute top-0 left-0 h-full bg-muted-foreground/40 transition-all"
                style={{ width: `${(deviceUsed / deviceTotal) * 100}%` }}
              />
              {/* New content */}
              <div
                className={`absolute top-0 h-full transition-all ${
                  willFit ? 'bg-gradient-to-r from-primary to-primary/80' : 'bg-gradient-to-r from-destructive to-destructive/80'
                }`}
                style={{ 
                  left: `${(deviceUsed / deviceTotal) * 100}%`,
                  width: `${(totalSize / deviceTotal) * 100}%` 
                }}
              />
            </div>
            
            <div className="flex items-center justify-between text-xs">
              <div className="flex items-center gap-3">
                <span className="flex items-center gap-1.5">
                  <div className="w-2 h-2 rounded-full bg-muted-foreground/40" />
                  Used: {deviceUsed} KB
                </span>
                <span className="flex items-center gap-1.5">
                  <div className={`w-2 h-2 rounded-full ${willFit ? 'bg-primary' : 'bg-destructive'}`} />
                  New: {totalSize} KB
                </span>
              </div>
              {!willFit ? (
                <span className="text-destructive flex items-center gap-1 font-medium">
                  <AlertCircle className="w-3.5 h-3.5" />
                  Insufficient storage
                </span>
              ) : (
                <span className="text-success flex items-center gap-1 font-medium">
                  <CheckCircle2 className="w-3.5 h-3.5" />
                  {availableStorage - totalSize} KB free after upload
                </span>
              )}
            </div>
          </div>

          {/* Upload Progress */}
          {uploading && (
            <div className="space-y-3 p-4 glass rounded-lg border border-primary/50 glow-primary">
              <div className="flex justify-between items-center text-sm">
                <span className="font-medium">Uploading segments...</span>
                <span className="font-mono font-semibold text-primary">{progress}%</span>
              </div>
              <Progress value={progress} className="h-2" />
              <div className="text-xs text-muted-foreground">
                Transferring {Math.round((progress / 100) * totalSize)} / {totalSize} KB
              </div>
            </div>
          )}

          {/* Play after upload option */}
          <div className="flex items-center space-x-2 p-3 bg-muted/20 rounded-lg">
            <Checkbox id="play-after" defaultChecked />
            <label
              htmlFor="play-after"
              className="text-sm font-medium leading-none peer-disabled:cursor-not-allowed peer-disabled:opacity-70 cursor-pointer"
            >
              Play pattern after upload completes
            </label>
          </div>
        </div>

        <DialogFooter>
          <Button 
            variant="outline" 
            onClick={() => onOpenChange(false)} 
            disabled={uploading}
            className="hover-lift"
          >
            Cancel
          </Button>
          <Button 
            disabled={!willFit || uploading} 
            className="gap-2 hover-lift active-press glow-primary"
          >
            <Upload className="w-4 h-4" />
            {uploading ? 'Uploading...' : 'Upload & Sync'}
          </Button>
        </DialogFooter>
      </DialogContent>
    </Dialog>
  );
}
