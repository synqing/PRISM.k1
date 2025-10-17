import { Play, Pause, RotateCcw, Save, Upload, Download, Maximize2, Settings } from 'lucide-react';
import { Button } from '../ui/button';
import { Badge } from '../ui/badge';

interface K1ToolbarProps {
  playing?: boolean;
  onPlayPause?: () => void;
  onReset?: () => void;
  onSave?: () => void;
  onExport?: () => void;
  onImport?: () => void;
  onFullscreen?: () => void;
  onSettings?: () => void;
  nodeCount?: number;
  wireCount?: number;
}

export function K1Toolbar({
  playing = false,
  onPlayPause,
  onReset,
  onSave,
  onExport,
  onImport,
  onFullscreen,
  onSettings,
  nodeCount = 0,
  wireCount = 0,
}: K1ToolbarProps) {
  return (
    <div className="h-12 border-b border-[var(--k1-border)] glass-panel flex items-center justify-center px-4 relative z-20">
      <div className="flex items-center gap-3 max-w-2xl">
        {/* Branding */}
        <div className="flex items-center gap-2">
          <div className="w-7 h-7 rounded bg-gradient-to-br from-[var(--k1-accent)] to-[var(--k1-accent-2)] flex items-center justify-center">
            <span className="text-black font-bold text-xs">K1</span>
          </div>
          <span className="text-xs font-medium">Light Lab</span>
        </div>

        {/* Stats */}
        <div className="flex items-center gap-1.5">
          <Badge variant="outline" className="font-mono text-[10px] h-5 px-1.5">
            {nodeCount}n
          </Badge>
          <Badge variant="outline" className="font-mono text-[10px] h-5 px-1.5">
            {wireCount}w
          </Badge>
        </div>

        <div className="w-px h-5 bg-[rgba(255,255,255,0.1)]" />

        {/* Playback Controls */}
        <div className="flex items-center gap-1">
          <Button
            variant="ghost"
            size="sm"
            onClick={onReset}
            className="h-7 w-7 p-0"
            title="Reset (R)"
          >
            <RotateCcw className="w-3.5 h-3.5" />
          </Button>
          
          <Button
            variant={playing ? "default" : "outline"}
            size="sm"
            onClick={onPlayPause}
            className="h-7 w-7 p-0"
            title={playing ? "Pause (Space)" : "Play (Space)"}
          >
            {playing ? (
              <Pause className="w-3.5 h-3.5" />
            ) : (
              <Play className="w-3.5 h-3.5" />
            )}
          </Button>
        </div>

        <div className="w-px h-5 bg-[rgba(255,255,255,0.1)]" />

        {/* Actions */}
        <div className="flex items-center gap-1">
          <Button
            variant="ghost"
            size="sm"
            onClick={onSave}
            className="h-7 w-7 p-0"
            title="Save (⌘S)"
          >
            <Save className="w-3.5 h-3.5" />
          </Button>
          
          <Button
            variant="ghost"
            size="sm"
            onClick={onExport}
            className="h-7 w-7 p-0"
            title="Export to K1"
          >
            <Download className="w-3.5 h-3.5" />
          </Button>

          <Button
            variant="ghost"
            size="sm"
            onClick={onSettings}
            className="h-7 w-7 p-0"
            title="Settings"
          >
            <Settings className="w-3.5 h-3.5" />
          </Button>
        </div>
      </div>
    </div>
  );
}
