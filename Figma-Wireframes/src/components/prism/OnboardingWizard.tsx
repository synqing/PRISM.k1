import { useState } from "react";
import { Button } from "../ui/button";
import { Card } from "../ui/card";
import { Progress } from "../ui/progress";
import { Sparkles, Radio, FileText, Zap } from "lucide-react";

interface OnboardingWizardProps {
  onComplete?: () => void;
}

export function OnboardingWizard({ onComplete }: OnboardingWizardProps) {
  const [step, setStep] = useState(0);

  const steps = [
    {
      title: 'Welcome to PRISM Studio',
      description: 'Create stunning LED patterns with timeline-based editing and live 3D preview.',
      icon: Sparkles,
      content: (
        <div className="space-y-4">
          <div className="aspect-video bg-gradient-to-br from-primary/20 via-accent/20 to-primary/20 rounded-lg flex items-center justify-center">
            <div className="w-48 h-32 bg-gradient-to-r from-primary via-accent to-primary rounded animate-pulse" />
          </div>
          <ul className="space-y-2 text-muted-foreground">
            <li className="flex items-start gap-2">
              <div className="w-1.5 h-1.5 rounded-full bg-primary mt-2" />
              <span>Timeline-first editing for precise control</span>
            </li>
            <li className="flex items-start gap-2">
              <div className="w-1.5 h-1.5 rounded-full bg-primary mt-2" />
              <span>Real-time 3D preview at 60fps</span>
            </li>
            <li className="flex items-start gap-2">
              <div className="w-1.5 h-1.5 rounded-full bg-primary mt-2" />
              <span>One-click sync to your K1-Lightwave devices</span>
            </li>
          </ul>
        </div>
      )
    },
    {
      title: 'Discover Your Device',
      description: 'Connect to your K1-Lightwave to sync patterns.',
      icon: Radio,
      content: (
        <div className="space-y-4">
          <div className="p-8 bg-muted/50 rounded-lg text-center space-y-3">
            <Radio className="w-12 h-12 mx-auto text-muted-foreground animate-pulse" />
            <p className="text-sm text-muted-foreground">
              Make sure your K1-Lightwave is powered on and connected to the same network.
            </p>
          </div>
          <Button className="w-full gap-2">
            <Radio className="w-4 h-4" />
            Scan for Devices
          </Button>
          <div className="text-center">
            <button className="text-sm text-muted-foreground hover:text-foreground">
              Or enter IP manually
            </button>
          </div>
        </div>
      )
    },
    {
      title: 'Pick a Template',
      description: 'Start with a pre-built pattern or create from scratch.',
      icon: FileText,
      content: (
        <div className="grid grid-cols-2 gap-3">
          {['Ocean Sunrise', 'Rainbow Wave', 'Fire Dance', 'Blank Canvas'].map((template) => (
            <Card
              key={template}
              className="p-4 cursor-pointer hover:border-primary transition-colors"
            >
              <div className="aspect-video bg-gradient-to-br from-primary/10 via-accent/10 to-primary/10 rounded mb-2" />
              <div className="text-sm font-medium">{template}</div>
            </Card>
          ))}
        </div>
      )
    },
    {
      title: 'Edit & Sync',
      description: 'Customize your pattern and sync it to your device.',
      icon: Zap,
      content: (
        <div className="space-y-4">
          <div className="aspect-video bg-muted/50 rounded-lg flex items-center justify-center">
            <div className="text-center space-y-2">
              <Zap className="w-12 h-12 mx-auto text-primary" />
              <p className="text-sm text-muted-foreground">
                You're all set! Use the timeline to edit and the sync button to upload.
              </p>
            </div>
          </div>
          <div className="space-y-2 text-sm text-muted-foreground">
            <div className="flex items-start gap-2">
              <div className="w-1.5 h-1.5 rounded-full bg-accent mt-2" />
              <span>Press Space to play/pause</span>
            </div>
            <div className="flex items-start gap-2">
              <div className="w-1.5 h-1.5 rounded-full bg-accent mt-2" />
              <span>Drag effects from the library to the timeline</span>
            </div>
            <div className="flex items-start gap-2">
              <div className="w-1.5 h-1.5 rounded-full bg-accent mt-2" />
              <span>Adjust parameters in the inspector</span>
            </div>
          </div>
        </div>
      )
    }
  ];

  const currentStep = steps[step];
  const Icon = currentStep.icon;

  return (
    <div className="fixed inset-0 z-50 bg-background/95 backdrop-blur-lg flex items-center justify-center p-8">
      <Card className="w-full max-w-3xl p-8 space-y-6 shadow-elevation-3 border-border/50">
        {/* Progress */}
        <div className="space-y-3">
          <div className="flex justify-between items-center text-sm">
            <span className="text-muted-foreground font-medium">
              Step {step + 1} of {steps.length}
            </span>
            <span className="font-mono font-semibold text-primary">
              {Math.round(((step + 1) / steps.length) * 100)}%
            </span>
          </div>
          <div className="relative h-2 bg-muted rounded-full overflow-hidden">
            <div 
              className="absolute inset-y-0 left-0 bg-gradient-to-r from-primary to-accent transition-all duration-300 rounded-full"
              style={{ width: `${((step + 1) / steps.length) * 100}%` }}
            />
          </div>
        </div>

        {/* Content */}
        <div className="space-y-6">
          <div className="flex items-start gap-5">
            <div className="w-14 h-14 rounded-xl bg-gradient-to-br from-primary/20 to-accent/20 flex items-center justify-center flex-shrink-0 shadow-elevation-1">
              <Icon className="w-7 h-7 text-primary" />
            </div>
            <div className="flex-1">
              <h2 className="text-2xl font-semibold mb-2">{currentStep.title}</h2>
              <p className="text-muted-foreground leading-relaxed">{currentStep.description}</p>
            </div>
          </div>

          <div className="pt-2">
            {currentStep.content}
          </div>
        </div>

        {/* Navigation */}
        <div className="flex justify-between pt-6 border-t border-border">
          <Button
            variant="outline"
            onClick={() => setStep(Math.max(0, step - 1))}
            disabled={step === 0}
            className="hover-lift"
          >
            Back
          </Button>
          <div className="flex gap-3">
            <Button variant="ghost" onClick={onComplete} className="hover-lift">
              Skip Tutorial
            </Button>
            {step < steps.length - 1 ? (
              <Button onClick={() => setStep(step + 1)} className="hover-lift active-press glow-primary">
                Next
              </Button>
            ) : (
              <Button onClick={onComplete} className="hover-lift active-press glow-primary">
                Get Started
              </Button>
            )}
          </div>
        </div>
      </Card>
    </div>
  );
}
