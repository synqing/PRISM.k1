import { useEffect } from "react";
import { TopBar } from "./TopBar";
import { Sidebar } from "./Sidebar";
import { Inspector } from "./Inspector";
import { Timeline } from "./Timeline";
import { Transport } from "./Transport";
import { Preview3D } from "./Preview3D";
import { ShortcutsOverlay } from "./ShortcutsOverlay";
import { ResizableHandle, ResizablePanel, ResizablePanelGroup } from "../ui/resizable";
import { useAppState } from "../../lib/appState";
import { useKeyboardShortcuts } from "../../lib/useKeyboardShortcuts";

export function TimelineEditorScreen() {
  const { 
    isPlaying,
    currentTime,
    setCurrentTime,
    quality,
    setQuality,
    showShortcuts,
    setShowShortcuts
  } = useAppState();
  
  // Enable keyboard shortcuts
  useKeyboardShortcuts();
  
  // Animation loop
  useEffect(() => {
    if (!isPlaying) return;
    
    let lastTime = Date.now();
    let animationFrameId: number;
    
    const animate = () => {
      const now = Date.now();
      const delta = now - lastTime;
      lastTime = now;
      
      setCurrentTime(currentTime + delta);
      animationFrameId = requestAnimationFrame(animate);
    };
    
    animationFrameId = requestAnimationFrame(animate);
    return () => cancelAnimationFrame(animationFrameId);
  }, [isPlaying, currentTime, setCurrentTime]);

  return (
    <div className="h-screen flex flex-col bg-background">
      <TopBar connected={true} fps="Auto" quality={quality} />
      
      <div className="flex-1 flex overflow-hidden">
        <Sidebar />
        
        <div className="flex-1 flex flex-col">
          <ResizablePanelGroup direction="vertical">
            {/* Preview */}
            <ResizablePanel defaultSize={55} minSize={30}>
              <Preview3D 
                quality={quality} 
                onQualityToggle={() => setQuality(quality === 'HQ' ? 'LQ' : 'HQ')}
              />
            </ResizablePanel>
            
            <ResizableHandle className="bg-border hover:bg-primary/50 transition-colors" />
            
            {/* Timeline */}
            <ResizablePanel defaultSize={45} minSize={30}>
              <Timeline />
            </ResizablePanel>
          </ResizablePanelGroup>
          
          <Transport onShowShortcuts={() => setShowShortcuts(true)} />
        </div>
        
        <Inspector selectedClip="Ocean Sunrise" />
      </div>
      
      {/* Keyboard Shortcuts Overlay */}
      {showShortcuts && (
        <ShortcutsOverlay onClose={() => setShowShortcuts(false)} />
      )}
    </div>
  );
}
