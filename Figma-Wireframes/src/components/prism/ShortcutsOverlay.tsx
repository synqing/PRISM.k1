import { Dialog, DialogContent, DialogHeader, DialogTitle } from "../ui/dialog";
import { Badge } from "../ui/badge";

interface ShortcutsOverlayProps {
  open: boolean;
  onOpenChange: (open: boolean) => void;
}

const SHORTCUTS = [
  { category: 'Playback', shortcuts: [
    { keys: ['Space'], description: 'Play/Pause' },
    { keys: ['J'], description: 'Reverse' },
    { keys: ['K'], description: 'Pause' },
    { keys: ['L'], description: 'Fast Forward' },
  ]},
  { category: 'Timeline', shortcuts: [
    { keys: ['Cmd', 'K'], description: 'Split Clip' },
    { keys: ['Q'], description: 'Ripple Delete' },
    { keys: ['W'], description: 'Ripple Trim' },
    { keys: ['I'], description: 'Mark In' },
    { keys: ['O'], description: 'Mark Out' },
    { keys: ['U'], description: 'Show Animated' },
  ]},
  { category: 'View', shortcuts: [
    { keys: ['Cmd', '+'], description: 'Zoom In' },
    { keys: ['Cmd', '−'], description: 'Zoom Out' },
    { keys: ['\\'], description: 'Zoom to Fit' },
  ]},
  { category: 'Animation', shortcuts: [
    { keys: ['F9'], description: 'Easy Ease' },
  ]},
];

export function ShortcutsOverlay({ open, onOpenChange }: ShortcutsOverlayProps) {
  return (
    <Dialog open={open} onOpenChange={onOpenChange}>
      <DialogContent className="max-w-2xl max-h-[80vh] overflow-auto">
        <DialogHeader>
          <DialogTitle>Keyboard Shortcuts</DialogTitle>
        </DialogHeader>

        <div className="space-y-6">
          {SHORTCUTS.map((category) => (
            <div key={category.category} className="space-y-2">
              <h3 className="text-sm text-muted-foreground">{category.category}</h3>
              <div className="space-y-1">
                {category.shortcuts.map((shortcut, i) => (
                  <div key={i} className="flex items-center justify-between py-2 px-3 rounded hover:bg-accent/10">
                    <span>{shortcut.description}</span>
                    <div className="flex items-center gap-1">
                      {shortcut.keys.map((key, j) => (
                        <div key={j} className="flex items-center gap-1">
                          {j > 0 && <span className="text-muted-foreground">+</span>}
                          <Badge variant="outline" className="font-mono">
                            {key}
                          </Badge>
                        </div>
                      ))}
                    </div>
                  </div>
                ))}
              </div>
            </div>
          ))}
        </div>
      </DialogContent>
    </Dialog>
  );
}
