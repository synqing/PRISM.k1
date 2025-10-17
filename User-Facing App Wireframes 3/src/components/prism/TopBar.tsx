import { Badge } from "../ui/badge";
import { Button } from "../ui/button";
import { HelpCircle, Wifi, WifiOff, ChevronDown } from "lucide-react";
import {
  DropdownMenu,
  DropdownMenuContent,
  DropdownMenuItem,
  DropdownMenuTrigger,
} from "../ui/dropdown-menu";

interface TopBarProps {
  connected?: boolean;
  fps?: string;
  quality?: 'HQ' | 'LQ';
  onQualityToggle?: () => void;
  theme?: 'light' | 'dark';
}

export function TopBar({ connected = false, fps = 'Auto', quality = 'HQ', onQualityToggle, theme = 'dark' }: TopBarProps) {
  return (
    <div className="h-12 border-b border-border glass backdrop-blur-xl flex items-center justify-between px-4 gap-4 relative z-10">
      {/* Left: Logo & Menu */}
      <div className="flex items-center gap-6">
        <div className="flex items-center gap-2.5">
          <div className="w-7 h-7 bg-gradient-to-br from-primary via-primary to-accent rounded-lg flex items-center justify-center shadow-elevation-1 glow-primary">
            <div className="w-3 h-3 bg-white/90 rounded-sm" />
          </div>
          <span className="font-semibold tracking-tight">PRISM Studio</span>
        </div>
        <nav className="flex items-center gap-0.5">
          {['File', 'Edit', 'View', 'Help'].map((item) => (
            <Button 
              key={item} 
              variant="ghost" 
              size="sm" 
              className="h-8 px-3 hover-lift active-press"
            >
              {item}
            </Button>
          ))}
        </nav>
      </div>

      {/* Right: Status & Controls */}
      <div className="flex items-center gap-3">
        <Badge 
          variant={connected ? "default" : "secondary"} 
          className={`gap-1.5 px-3 py-1 ${connected ? 'glow-primary' : ''}`}
        >
          {connected ? <Wifi className="w-3.5 h-3.5" /> : <WifiOff className="w-3.5 h-3.5" />}
          {connected ? 'Connected' : 'Not Connected'}
        </Badge>
        
        <div className="h-5 w-px bg-border/50" />
        
        <DropdownMenu>
          <DropdownMenuTrigger asChild>
            <Button variant="ghost" size="sm" className="h-8 px-2.5 gap-1.5 hover-lift">
              <span className="text-xs text-muted-foreground">FPS:</span>
              <span className="font-mono">{fps}</span>
              <ChevronDown className="w-3 h-3 text-muted-foreground" />
            </Button>
          </DropdownMenuTrigger>
          <DropdownMenuContent align="end" className="glass shadow-elevation-2">
            <DropdownMenuItem>Auto</DropdownMenuItem>
            <DropdownMenuItem>60</DropdownMenuItem>
            <DropdownMenuItem>30</DropdownMenuItem>
            <DropdownMenuItem>24</DropdownMenuItem>
            <DropdownMenuItem>15</DropdownMenuItem>
          </DropdownMenuContent>
        </DropdownMenu>
        
        <Button
          variant="outline"
          size="sm"
          className="h-8 px-3 font-mono hover-lift active-press"
          onClick={onQualityToggle}
        >
          {quality}
        </Button>
        
        <Button variant="ghost" size="sm" className="h-8 w-8 p-0 hover-lift active-press">
          <HelpCircle className="w-4 h-4" />
        </Button>
      </div>
    </div>
  );
}
