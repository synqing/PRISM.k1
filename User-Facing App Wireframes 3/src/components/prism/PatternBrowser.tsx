import { useState } from "react";
import { Input } from "../ui/input";
import { Button } from "../ui/button";
import { Badge } from "../ui/badge";
import { Tabs, TabsContent, TabsList, TabsTrigger } from "../ui/tabs";
import { Search, Upload, Heart, Play, Download, Trash2, MoreVertical } from "lucide-react";
import { useAppState } from "../../lib/appState";
import { toast } from "sonner@2.0.3";
import { K1DeviceAnimated } from "./K1DeviceAnimated";
import {
  DropdownMenu,
  DropdownMenuContent,
  DropdownMenuItem,
  DropdownMenuSeparator,
  DropdownMenuTrigger,
} from "../ui/dropdown-menu";

const CATEGORIES = ['All', 'Animated', 'LGP', 'Favorites'];

export function PatternBrowser() {
  const { patterns, togglePatternFavorite, setCurrentPattern, removePattern, addSegment, tracks } = useAppState();
  const [searchQuery, setSearchQuery] = useState('');
  const [selectedCategory, setSelectedCategory] = useState('All');
  const [selectedPatternId, setSelectedPatternId] = useState<string | null>(null);
  const [previewPatternId, setPreviewPatternId] = useState<string | null>(null);

  const filteredPatterns = patterns.filter(pattern => {
    const matchesSearch = pattern.name.toLowerCase().includes(searchQuery.toLowerCase());
    const matchesCategory = 
      selectedCategory === 'All' || 
      (selectedCategory === 'Favorites' && pattern.favorite) ||
      pattern.tags.includes(selectedCategory.toLowerCase());
    return matchesSearch && matchesCategory;
  });
  
  const handleSync = (patternId: string) => {
    const pattern = patterns.find(p => p.id === patternId);
    if (pattern) {
      toast.success(`Syncing "${pattern.name}" to device...`);
      // Simulate sync delay
      setTimeout(() => {
        toast.success('Pattern synced successfully!');
      }, 1500);
    }
  };
  
  const handleLoad = (patternId: string) => {
    const pattern = patterns.find(p => p.id === patternId);
    if (pattern) {
      setCurrentPattern(pattern);
      toast.success(`Loaded "${pattern.name}" into editor`);
    }
  };
  
  const handleAddToTimeline = (patternId: string) => {
    const pattern = patterns.find(p => p.id === patternId);
    if (pattern && tracks.length > 0) {
      const firstTrack = tracks[0];
      const lastSegment = firstTrack.segments[firstTrack.segments.length - 1];
      const startTime = lastSegment ? lastSegment.startTime + lastSegment.duration : 0;
      
      addSegment(firstTrack.id, {
        id: `segment-${Date.now()}`,
        patternId: pattern.id,
        startTime,
        duration: pattern.duration,
        color: pattern.favorite ? '#FF7A59' : '#5B8CFF'
      });
      
      toast.success(`Added "${pattern.name}" to timeline`);
    }
  };
  
  const handleDelete = (patternId: string) => {
    const pattern = patterns.find(p => p.id === patternId);
    if (pattern) {
      removePattern(patternId);
      toast.success(`Deleted "${pattern.name}"`);
    }
  };

  const selectedPattern = patterns.find(p => p.id === selectedPatternId);
  const previewPattern = patterns.find(p => p.id === previewPatternId);

  return (
    <div className="h-full flex bg-background">
      {/* Main Content */}
      <div className="flex-1 flex flex-col">
        {/* Header */}
        <div className="border-b border-border glass backdrop-blur-xl p-6 space-y-4">
          <div className="flex items-center justify-between">
            <div>
              <h1 className="text-3xl font-semibold tracking-tight">Pattern Browser</h1>
              <p className="text-muted-foreground mt-1">
                {filteredPatterns.length} pattern{filteredPatterns.length !== 1 ? 's' : ''} 
                {selectedCategory !== 'All' && ` in ${selectedCategory}`}
              </p>
            </div>
            <div className="flex gap-2">
              <Button 
                variant="outline" 
                className="gap-2 hover-lift active-press"
                onClick={() => toast.info('Import feature coming soon')}
              >
                <Download className="w-4 h-4" />
                Import
              </Button>
              <Button 
                className="gap-2 hover-lift active-press glow-primary shadow-elevation-1"
                onClick={() => {
                  if (selectedPatternId) {
                    handleSync(selectedPatternId);
                  } else {
                    toast.error('Please select a pattern first');
                  }
                }}
              >
                <Upload className="w-4 h-4" />
                Sync Selected
              </Button>
            </div>
          </div>

          <div className="flex gap-4">
            <div className="flex-1 relative">
              <Search className="absolute left-3 top-1/2 -translate-y-1/2 w-4 h-4 text-muted-foreground" />
              <Input
                placeholder="Search patterns..."
                value={searchQuery}
                onChange={(e) => setSearchQuery(e.target.value)}
                className="pl-9"
              />
            </div>
          </div>
        </div>

        {/* Categories */}
        <div className="border-b border-border px-6 py-3">
          <Tabs value={selectedCategory} onValueChange={setSelectedCategory}>
            <TabsList>
              {CATEGORIES.map(category => (
                <TabsTrigger key={category} value={category}>
                  {category}
                  {category === 'Favorites' && (
                    <Badge variant="secondary" className="ml-2 text-xs">
                      {patterns.filter(p => p.favorite).length}
                    </Badge>
                  )}
                </TabsTrigger>
              ))}
            </TabsList>
          </Tabs>
        </div>

        {/* Pattern Grid */}
        <div className="flex-1 overflow-auto p-6">
          {filteredPatterns.length === 0 ? (
            <div className="h-full flex items-center justify-center">
              <div className="text-center space-y-3">
                <div className="w-16 h-16 mx-auto rounded-full bg-muted/50 flex items-center justify-center">
                  <Search className="w-8 h-8 text-muted-foreground" />
                </div>
                <div className="font-medium">No patterns found</div>
                <p className="text-sm text-muted-foreground">
                  {searchQuery ? 'Try a different search term' : 'Create your first pattern in the Studio'}
                </p>
              </div>
            </div>
          ) : (
            <div className="grid grid-cols-4 gap-4">
              {filteredPatterns.map((pattern) => (
                <div
                  key={pattern.id}
                  onClick={() => setSelectedPatternId(pattern.id)}
                  onMouseEnter={() => setPreviewPatternId(pattern.id)}
                  onMouseLeave={() => setPreviewPatternId(null)}
                  className={`group cursor-pointer border rounded-xl overflow-hidden transition-all hover-lift ${
                    selectedPatternId === pattern.id
                      ? 'border-primary ring-2 ring-primary ring-offset-2 ring-offset-background shadow-elevation-2 glow-primary'
                      : 'border-border hover:border-primary/50 shadow-sm hover:shadow-elevation-1'
                  }`}
                >
                  {/* Thumbnail with Live Preview */}
                  <div className="aspect-video bg-gradient-to-br from-[#0D0F14] via-[#1a1d24] to-[#0D0F14] relative overflow-hidden">
                    {/* Live K1 Preview on Hover */}
                    {previewPatternId === pattern.id ? (
                      <div className="absolute inset-0 flex items-center justify-center p-4">
                        <div style={{ transform: 'scale(0.18)' }}>
                          <K1DeviceAnimated
                            heightVariant="h60"
                            finish="dark-frame"
                            angle="flat"
                            pattern={pattern.config.pattern}
                            config={pattern.config}
                            isPlaying={true}
                          />
                        </div>
                      </div>
                    ) : (
                      /* Static Grid Preview */
                      <div className="absolute inset-0 p-4 flex items-center justify-center">
                        <div className="w-3/4 h-2/3 grid grid-cols-6 grid-rows-4 gap-1">
                          {Array.from({ length: 24 }).map((_, i) => (
                            <div 
                              key={i} 
                              className="rounded-sm bg-gradient-to-br from-primary/40 to-accent/40 shadow-sm opacity-60"
                            />
                          ))}
                        </div>
                      </div>
                    )}
                    
                    {/* Favorite Badge */}
                    <button
                      onClick={(e) => {
                        e.stopPropagation();
                        togglePatternFavorite(pattern.id);
                      }}
                      className="absolute top-3 left-3 w-8 h-8 rounded-full flex items-center justify-center transition-all hover-lift active-press hover:scale-110"
                      style={{
                        background: pattern.favorite ? 'rgba(255, 122, 89, 0.2)' : 'rgba(0, 0, 0, 0.5)',
                        backdropFilter: 'blur(8px)',
                      }}
                    >
                      <Heart 
                        className={`w-4 h-4 transition-all ${
                          pattern.favorite 
                            ? 'fill-accent text-accent' 
                            : 'text-white/70 hover:text-white'
                        }`}
                      />
                    </button>
                    
                    {/* Actions Menu */}
                    <div className="absolute top-3 right-3">
                      <DropdownMenu>
                        <DropdownMenuTrigger asChild>
                          <button
                            onClick={(e) => e.stopPropagation()}
                            className="w-8 h-8 rounded-full flex items-center justify-center transition-all hover-lift active-press hover:scale-110"
                            style={{
                              background: 'rgba(0, 0, 0, 0.5)',
                              backdropFilter: 'blur(8px)',
                            }}
                          >
                            <MoreVertical className="w-4 h-4 text-white/70" />
                          </button>
                        </DropdownMenuTrigger>
                        <DropdownMenuContent align="end">
                          <DropdownMenuItem onClick={() => handleLoad(pattern.id)}>
                            <Play className="w-4 h-4 mr-2" />
                            Load in Studio
                          </DropdownMenuItem>
                          <DropdownMenuItem onClick={() => handleAddToTimeline(pattern.id)}>
                            <Upload className="w-4 h-4 mr-2" />
                            Add to Timeline
                          </DropdownMenuItem>
                          <DropdownMenuItem onClick={() => handleSync(pattern.id)}>
                            <Upload className="w-4 h-4 mr-2" />
                            Sync to Device
                          </DropdownMenuItem>
                          <DropdownMenuSeparator />
                          <DropdownMenuItem 
                            onClick={() => handleDelete(pattern.id)}
                            className="text-destructive focus:text-destructive"
                          >
                            <Trash2 className="w-4 h-4 mr-2" />
                            Delete
                          </DropdownMenuItem>
                        </DropdownMenuContent>
                      </DropdownMenu>
                    </div>
                    
                    {/* Selection Indicator */}
                    {selectedPatternId === pattern.id && (
                      <div className="absolute bottom-3 right-3 w-7 h-7 bg-primary rounded-full flex items-center justify-center shadow-elevation-2 glow-primary">
                        <div className="w-3 h-3 bg-primary-foreground rounded-full" />
                      </div>
                    )}
                  </div>

                  {/* Info */}
                  <div className="p-3 space-y-2">
                    <div className="font-medium truncate">{pattern.name}</div>
                    <div className="flex items-center gap-2 text-xs">
                      {pattern.tags.slice(0, 2).map(tag => (
                        <Badge key={tag} variant="secondary" className="text-xs capitalize">
                          {tag}
                        </Badge>
                      ))}
                    </div>
                    <div className="flex items-center justify-between text-xs text-muted-foreground">
                      <span className="font-mono">{(pattern.duration / 1000).toFixed(1)}s</span>
                      <span className="font-mono">{Math.round(pattern.size / 1024)} KB</span>
                    </div>
                  </div>
                </div>
              ))}
            </div>
          )}
        </div>
      </div>

      {/* Details Sidebar */}
      {selectedPattern && (
        <div className="w-80 border-l border-border glass backdrop-blur-xl p-6 space-y-6 overflow-auto">
          <div>
            <h3 className="font-semibold mb-2">Pattern Details</h3>
            <div className="space-y-3 text-sm">
              <div>
                <div className="text-muted-foreground mb-1">Name</div>
                <div className="font-medium">{selectedPattern.name}</div>
              </div>
              <div>
                <div className="text-muted-foreground mb-1">Description</div>
                <div className="text-sm">{selectedPattern.description}</div>
              </div>
              <div>
                <div className="text-muted-foreground mb-1">Duration</div>
                <div className="font-mono">{(selectedPattern.duration / 1000).toFixed(1)}s</div>
              </div>
              <div>
                <div className="text-muted-foreground mb-1">File Size</div>
                <div className="font-mono">{Math.round(selectedPattern.size / 1024)} KB</div>
              </div>
              <div>
                <div className="text-muted-foreground mb-1">Created</div>
                <div className="text-sm">{selectedPattern.createdAt.toLocaleDateString()}</div>
              </div>
              <div>
                <div className="text-muted-foreground mb-1">Tags</div>
                <div className="flex flex-wrap gap-1">
                  {selectedPattern.tags.map(tag => (
                    <Badge key={tag} variant="outline" className="text-xs capitalize">
                      {tag}
                    </Badge>
                  ))}
                </div>
              </div>
            </div>
          </div>

          <div className="pt-4 border-t border-border space-y-2">
            <Button 
              className="w-full gap-2 glow-primary"
              onClick={() => handleLoad(selectedPattern.id)}
            >
              <Play className="w-4 h-4" />
              Load in Studio
            </Button>
            <Button 
              variant="outline" 
              className="w-full gap-2"
              onClick={() => handleAddToTimeline(selectedPattern.id)}
            >
              <Upload className="w-4 h-4" />
              Add to Timeline
            </Button>
            <Button 
              variant="outline" 
              className="w-full gap-2"
              onClick={() => handleSync(selectedPattern.id)}
            >
              <Upload className="w-4 h-4" />
              Sync to Device
            </Button>
          </div>
        </div>
      )}
    </div>
  );
}
