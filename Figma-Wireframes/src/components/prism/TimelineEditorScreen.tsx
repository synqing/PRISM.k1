import { TopBar } from "./TopBar";
import { Sidebar } from "./Sidebar";
import { Inspector } from "./Inspector";
import { Timeline } from "./Timeline";
import { Transport } from "./Transport";
import { Preview3D } from "./Preview3D";
import { ResizableHandle, ResizablePanel, ResizablePanelGroup } from "../ui/resizable";

import { useState } from "react";

export function TimelineEditorScreen() {
  const [isPlaying, setIsPlaying] = useState(false);

  return (
    <div className="h-screen flex flex-col bg-background">
      <TopBar connected={true} fps="Auto" quality="HQ" />
      
      <div className="flex-1 flex overflow-hidden">
        <Sidebar />
        
        <div className="flex-1 flex flex-col">
          <ResizablePanelGroup direction="vertical">
            {/* Preview */}
            <ResizablePanel defaultSize={55} minSize={30}>
              <Preview3D quality="HQ" />
            </ResizablePanel>
            
            <ResizableHandle className="bg-border hover:bg-primary/50 transition-colors" />
            
            {/* Timeline */}
            <ResizablePanel defaultSize={45} minSize={30}>
              <Timeline />
            </ResizablePanel>
          </ResizablePanelGroup>
          
          <Transport 
            isPlaying={isPlaying} 
            onPlayPause={() => setIsPlaying(!isPlaying)}
            loop={false} 
            snap={true} 
          />
        </div>
        
        <Inspector selectedClip="Ocean Sunrise" />
      </div>
    </div>
  );
}
