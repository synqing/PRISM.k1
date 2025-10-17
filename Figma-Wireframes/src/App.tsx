import { useState, useEffect } from "react";
import { TimelineEditorScreen } from "./components/prism/TimelineEditorScreen";
import { PatternBrowser } from "./components/prism/PatternBrowser";
import { DeviceManagerScreen } from "./components/prism/DeviceManagerScreen";
import { NodeEditor } from "./components/prism/NodeEditor";
import { EmptyState } from "./components/prism/EmptyState";
import { OnboardingWizard } from "./components/prism/OnboardingWizard";
import { SyncModal } from "./components/prism/SyncModal";
import { ShortcutsOverlay } from "./components/prism/ShortcutsOverlay";
import { ThemeToggle } from "./components/prism/ThemeToggle";
import { TokensReference } from "./components/prism/TokensReference";
import { K1Showcase } from "./components/prism/K1Showcase";
import { PatternStudio } from "./components/prism/PatternStudio";
import { PatternStudioGlass } from "./components/prism/PatternStudioGlass";
import { Button } from "./components/ui/button";
import { Tabs, TabsContent, TabsList, TabsTrigger } from "./components/ui/tabs";
import { toast, Toaster } from "sonner@2.0.3";
import { 
  LayoutGrid, 
  Monitor, 
  Code, 
  MonitorX, 
  FileX, 
  WifiOff,
  Palette,
  Lightbulb,
  Wand2
} from "lucide-react";

function App() {
  const [currentScreen, setCurrentScreen] = useState<
    'timeline' | 'browser' | 'devices' | 'node-editor' | 'tokens' | 'k1-showcase' | 'pattern-studio' | 'pattern-studio-glass' |
    'empty-device' | 'empty-patterns' | 'device-lost'
  >('pattern-studio-glass');
  
  const [theme, setTheme] = useState<'light' | 'dark'>('dark');
  const [showOnboarding, setShowOnboarding] = useState(false);
  const [showSyncModal, setShowSyncModal] = useState(false);
  const [showShortcuts, setShowShortcuts] = useState(false);
  const [syncProgress, setSyncProgress] = useState(0);
  const [isSyncing, setIsSyncing] = useState(false);

  useEffect(() => {
    const root = document.documentElement;
    if (theme === 'dark') {
      root.classList.add('dark');
    } else {
      root.classList.remove('dark');
    }
  }, [theme]);

  const handleSync = () => {
    setShowSyncModal(true);
  };

  const handleStartSync = () => {
    setIsSyncing(true);
    setSyncProgress(0);
    
    const interval = setInterval(() => {
      setSyncProgress((prev) => {
        if (prev >= 100) {
          clearInterval(interval);
          setIsSyncing(false);
          setShowSyncModal(false);
          toast.success('Synced Ocean Sunrise (2 segments • 7.4s, Auto FPS)');
          return 100;
        }
        return prev + 10;
      });
    }, 200);
  };

  const renderScreen = () => {
    switch (currentScreen) {
      case 'pattern-studio-glass':
        return <PatternStudioGlass />;
      case 'pattern-studio':
        return <PatternStudio />;
      case 'k1-showcase':
        return <K1Showcase />;
      case 'timeline':
        return <TimelineEditorScreen />;
      case 'browser':
        return <PatternBrowser />;
      case 'devices':
        return <DeviceManagerScreen />;
      case 'tokens':
        return <TokensReference />;
      case 'node-editor':
        return <NodeEditor onClose={() => setCurrentScreen('timeline')} />;
      case 'empty-device':
        return (
          <div className="h-screen flex flex-col bg-background">
            <div className="h-12 border-b border-border glass px-4 flex items-center">
              <span className="font-semibold">PRISM Studio</span>
            </div>
            <EmptyState type="no-device" onAction={() => setCurrentScreen('devices')} />
          </div>
        );
      case 'empty-patterns':
        return (
          <div className="h-screen flex flex-col bg-background">
            <div className="h-12 border-b border-border glass px-4 flex items-center">
              <span className="font-semibold">PRISM Studio</span>
            </div>
            <EmptyState type="no-patterns" onAction={() => setCurrentScreen('browser')} />
          </div>
        );
      case 'device-lost':
        return (
          <div className="h-screen flex flex-col bg-background">
            <div className="h-12 border-b border-border glass px-4 flex items-center">
              <span className="font-semibold">PRISM Studio</span>
            </div>
            <EmptyState type="device-lost" onAction={() => toast.info('Reconnecting...')} />
          </div>
        );
      default:
        return null;
    }
  };

  return (
    <>
      {/* Demo Navigation */}
      <div className="fixed top-4 left-1/2 -translate-x-1/2 z-50">
        <div className="glass shadow-elevation-2 border border-border/50 rounded-lg p-2">
          <Tabs value={currentScreen} onValueChange={(v) => setCurrentScreen(v as any)}>
            <TabsList className="grid grid-cols-11 w-full">
              <TabsTrigger value="pattern-studio-glass" className="gap-1.5 text-xs px-2">
                <Wand2 className="w-3.5 h-3.5" />
                Glass
              </TabsTrigger>
              <TabsTrigger value="pattern-studio" className="gap-1.5 text-xs px-2">
                <Wand2 className="w-3.5 h-3.5" />
                Studio
              </TabsTrigger>
              <TabsTrigger value="k1-showcase" className="gap-1.5 text-xs px-2">
                <Lightbulb className="w-3.5 h-3.5" />
                K1
              </TabsTrigger>
              <TabsTrigger value="timeline" className="gap-1.5 text-xs px-2">
                <LayoutGrid className="w-3.5 h-3.5" />
                Timeline
              </TabsTrigger>
              <TabsTrigger value="browser" className="gap-1.5 text-xs px-2">
                <LayoutGrid className="w-3.5 h-3.5" />
                Browser
              </TabsTrigger>
              <TabsTrigger value="devices" className="gap-1.5 text-xs px-2">
                <Monitor className="w-3.5 h-3.5" />
                Devices
              </TabsTrigger>
              <TabsTrigger value="node-editor" className="gap-1.5 text-xs px-2">
                <Code className="w-3.5 h-3.5" />
                Nodes
              </TabsTrigger>
              <TabsTrigger value="tokens" className="gap-1.5 text-xs px-2">
                <Palette className="w-3.5 h-3.5" />
                Tokens
              </TabsTrigger>
              <TabsTrigger value="empty-device" className="gap-1.5 text-xs px-2">
                <MonitorX className="w-3.5 h-3.5" />
                No Dev
              </TabsTrigger>
              <TabsTrigger value="empty-patterns" className="gap-1.5 text-xs px-2">
                <FileX className="w-3.5 h-3.5" />
                No Pat
              </TabsTrigger>
              <TabsTrigger value="device-lost" className="gap-1.5 text-xs px-2">
                <WifiOff className="w-3.5 h-3.5" />
                Lost
              </TabsTrigger>
            </TabsList>
          </Tabs>
        </div>
      </div>

      {/* Demo Controls */}
      <div className="fixed bottom-4 right-4 z-50 flex flex-col gap-2">
        <ThemeToggle theme={theme} onToggle={() => setTheme(theme === 'dark' ? 'light' : 'dark')} />
        <Button
          onClick={() => setShowOnboarding(true)}
          variant="outline"
          className="shadow-elevation-2 glass hover-lift"
        >
          Show Onboarding
        </Button>
        <Button
          onClick={handleSync}
          variant="outline"
          className="shadow-elevation-2 glass hover-lift"
        >
          Show Sync Modal
        </Button>
        <Button
          onClick={() => setShowShortcuts(true)}
          variant="outline"
          className="shadow-elevation-2 glass hover-lift"
        >
          Show Shortcuts
        </Button>
        <Button
          onClick={() => toast.error('Storage is full. Free at least 12 KB or reduce FPS.')}
          variant="outline"
          className="shadow-elevation-2 glass hover-lift"
        >
          Show Error Toast
        </Button>
      </div>

      {/* Main Content */}
      {renderScreen()}

      {/* Modals & Overlays */}
      {showOnboarding && (
        <OnboardingWizard onComplete={() => setShowOnboarding(false)} />
      )}
      
      <SyncModal
        open={showSyncModal}
        onOpenChange={(open) => {
          if (!isSyncing) {
            setShowSyncModal(open);
          }
        }}
        uploading={isSyncing}
        progress={syncProgress}
      />

      {showShortcuts && (
        <ShortcutsOverlay onClose={() => setShowShortcuts(false)} />
      )}

      <Toaster 
        position="bottom-right" 
        toastOptions={{
          classNames: {
            toast: 'glass shadow-elevation-2 border-border/50',
          },
        }}
      />
    </>
  );
}

export default App;
