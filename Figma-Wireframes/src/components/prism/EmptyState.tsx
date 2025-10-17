import { Button } from "../ui/button";
import { MonitorX, FileX, Zap, WifiOff } from "lucide-react";

interface EmptyStateProps {
  type: 'no-device' | 'no-patterns' | 'no-selection' | 'device-lost';
  onAction?: () => void;
}

export function EmptyState({ type, onAction }: EmptyStateProps) {
  const configs = {
    'no-device': {
      icon: MonitorX,
      title: 'No K1-Lightwave Found',
      description: 'No K1-Lightwave found. Click Discover or enter IP.',
      action: 'Discover Devices'
    },
    'no-patterns': {
      icon: FileX,
      title: 'No Patterns Yet',
      description: "It's quiet here. Build your first pattern with a template.",
      action: 'Browse Templates'
    },
    'no-selection': {
      icon: Zap,
      title: 'No Effect Selected',
      description: 'Select an effect clip from the timeline to view its properties.',
      action: null
    },
    'device-lost': {
      icon: WifiOff,
      title: 'Connection Lost',
      description: "Connection lost. We'll retry in the background.",
      action: 'Retry Now'
    }
  };

  const config = configs[type];
  const Icon = config.icon;

  return (
    <div className="h-full flex items-center justify-center p-8">
      <div className="text-center space-y-4 max-w-md">
        <div className="flex justify-center">
          <div className="w-16 h-16 rounded-full bg-muted flex items-center justify-center">
            <Icon className="w-8 h-8 text-muted-foreground" />
          </div>
        </div>
        <div className="space-y-2">
          <h3 className="text-lg">{config.title}</h3>
          <p className="text-muted-foreground">{config.description}</p>
        </div>
        {config.action && (
          <Button onClick={onAction}>
            {config.action}
          </Button>
        )}
      </div>
    </div>
  );
}
