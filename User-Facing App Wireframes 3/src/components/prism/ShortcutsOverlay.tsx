import { Button } from "../ui/button";
import { X } from "lucide-react";
import { Badge } from "../ui/badge";

interface ShortcutsOverlayProps {
  onClose: () => void;
}

export function ShortcutsOverlay({ onClose }: ShortcutsOverlayProps) {
  const shortcuts = [
    {
      category: "Playback",
      items: [
        { keys: ["Space"], description: "Play / Pause" },
        { keys: ["K"], description: "Pause" },
        { keys: ["J"], description: "Reverse playback" },
        { keys: ["L"], description: "Forward (1x → 2x → 4x)" },
        { keys: ["Home"], description: "Jump to start" },
        { keys: ["End"], description: "Jump to end" },
      ]
    },
    {
      category: "Timeline Navigation",
      items: [
        { keys: ["←"], description: "Step backward 1 frame" },
        { keys: ["→"], description: "Step forward 1 frame" },
        { keys: ["Shift", "←"], description: "Jump back 1 second" },
        { keys: ["Shift", "→"], description: "Jump forward 1 second" },
      ]
    },
    {
      category: "Loop Points",
      items: [
        { keys: ["I"], description: "Set loop in point" },
        { keys: ["O"], description: "Set loop out point" },
        { keys: ["M"], description: "Toggle loop" },
      ]
    },
    {
      category: "Timeline Editing",
      items: [
        { keys: ["Delete"], description: "Delete selected segments" },
        { keys: ["Backspace"], description: "Delete selected segments" },
        { keys: ["Cmd/Ctrl", "Click"], description: "Multi-select segments" },
        { keys: ["Cmd/Ctrl", "D"], description: "Duplicate segment" },
      ]
    },
    {
      category: "File Operations",
      items: [
        { keys: ["Cmd/Ctrl", "S"], description: "Save pattern" },
        { keys: ["Cmd/Ctrl", "E"], description: "Export pattern" },
      ]
    },
    {
      category: "Interface",
      items: [
        { keys: ["?"], description: "Show shortcuts (this overlay)" },
        { keys: ["Esc"], description: "Close overlay / Deselect" },
      ]
    }
  ];

  return (
    <div 
      className="fixed inset-0 z-50 flex items-center justify-center bg-black/80 backdrop-blur-sm"
      onClick={onClose}
    >
      <div 
        className="max-w-4xl w-full max-h-[90vh] bg-card border border-border rounded-2xl shadow-elevation-3 overflow-hidden"
        onClick={(e) => e.stopPropagation()}
      >
        {/* Header */}
        <div className="border-b border-border glass backdrop-blur-xl p-6 flex items-center justify-between">
          <div>
            <h2 className="text-2xl font-semibold">Keyboard Shortcuts</h2>
            <p className="text-muted-foreground text-sm mt-1">
              Video editor-style controls for fast workflow
            </p>
          </div>
          <Button
            variant="ghost"
            size="sm"
            className="h-8 w-8 p-0 hover-lift active-press"
            onClick={onClose}
          >
            <X className="w-4 h-4" />
          </Button>
        </div>

        {/* Shortcuts Grid */}
        <div className="p-6 overflow-auto max-h-[calc(90vh-120px)]">
          <div className="grid grid-cols-2 gap-6">
            {shortcuts.map((section) => (
              <div key={section.category} className="space-y-3">
                <h3 className="font-semibold text-sm uppercase tracking-wide text-muted-foreground">
                  {section.category}
                </h3>
                <div className="space-y-2">
                  {section.items.map((shortcut, i) => (
                    <div 
                      key={i}
                      className="flex items-center justify-between p-2 rounded-lg hover:bg-muted/50 transition-colors"
                    >
                      <span className="text-sm">{shortcut.description}</span>
                      <div className="flex items-center gap-1">
                        {shortcut.keys.map((key, j) => (
                          <div key={j} className="flex items-center gap-1">
                            <Badge 
                              variant="outline" 
                              className="font-mono px-2 py-0.5 bg-muted/50 border-border shadow-sm"
                            >
                              {key}
                            </Badge>
                            {j < shortcut.keys.length - 1 && (
                              <span className="text-xs text-muted-foreground">+</span>
                            )}
                          </div>
                        ))}
                      </div>
                    </div>
                  ))}
                </div>
              </div>
            ))}
          </div>
        </div>

        {/* Footer */}
        <div className="border-t border-border glass backdrop-blur-xl p-4 flex items-center justify-between text-sm text-muted-foreground">
          <span>Press <Badge variant="outline" className="font-mono mx-1">Esc</Badge> or click outside to close</span>
          <span>PRISM Studio v1.0</span>
        </div>
      </div>
    </div>
  );
}
