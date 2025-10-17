import { CSSProperties, useEffect, useRef, useState } from 'react';
import { 
  AnimationConfig, 
  generateAnimationFrame, 
  calculateSurfaceColor,
  AnimationPattern,
  PATTERN_PRESETS 
} from './K1AnimationEngine';

interface K1DeviceAnimatedProps {
  heightVariant?: 'h50' | 'h60' | 'h70';
  finish?: 'dark-frame' | 'light-frame';
  angle?: 'front' | 'threeQuarter';
  className?: string;
  pattern?: AnimationPattern;
  config?: Partial<AnimationConfig>;
  isPlaying?: boolean;
  currentTime?: number; // External time control (ms)
  onTimeUpdate?: (time: number) => void;
}

const DEVICE_WIDTH = 1320;
const HEIGHT_MAP = {
  h50: 200,
  h60: 240,
  h70: 280,
};

export function K1DeviceAnimated({
  heightVariant = 'h60',
  finish = 'dark-frame',
  angle = 'front',
  className = '',
  pattern = 'radialBloom',
  config,
  isPlaying = true,
  currentTime,
  onTimeUpdate,
}: K1DeviceAnimatedProps) {
  const height = HEIGHT_MAP[heightVariant];
  const isDark = finish === 'dark-frame';
  const canvasRef = useRef<HTMLCanvasElement>(null);
  const animationFrameRef = useRef<number>();
  const startTimeRef = useRef<number>(Date.now());
  const [internalTime, setInternalTime] = useState(0);
  
  // Merge preset config with custom overrides
  const animConfig: AnimationConfig = {
    ...PATTERN_PRESETS[pattern],
    ...config
  };

  const containerStyle: CSSProperties = {
    width: DEVICE_WIDTH,
    height: height,
    position: 'relative',
    ...(angle === 'threeQuarter' && {
      transform: 'perspective(2000px) rotateY(-6deg) rotateX(2deg)',
      transformStyle: 'preserve-3d',
    }),
  };

  // Set canvas resolution once on mount or size change
  useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas) return;
    
    const dpr = window.devicePixelRatio || 1;
    canvas.width = DEVICE_WIDTH * dpr;
    canvas.height = height * dpr;
    
    const ctx = canvas.getContext('2d');
    if (ctx) {
      ctx.scale(dpr, dpr);
    }
  }, [height]);

  // Render frame (called every time currentTime/internalTime changes)
  useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas) return;

    const ctx = canvas.getContext('2d');
    if (!ctx) return;

    // Use external time if provided, otherwise use internal
    const time = currentTime !== undefined ? currentTime : internalTime;
    
    // Generate animation frame
    const frame = generateAnimationFrame(animConfig, time);
    
    // Clear canvas
    ctx.clearRect(0, 0, DEVICE_WIDTH, height);
    
    // Render LGP surface using pixel-by-pixel calculation
    const imageData = ctx.createImageData(DEVICE_WIDTH, height);
    const data = imageData.data;
    
    for (let y = 0; y < height; y++) {
      for (let x = 0; x < DEVICE_WIDTH; x++) {
        const posX = x / DEVICE_WIDTH;
        const posY = y / height;
        
        const color = calculateSurfaceColor(
          posX,
          posY,
          frame.ch1.lobes,
          frame.ch2.lobes
        );
        
        const idx = (y * DEVICE_WIDTH + x) * 4;
        data[idx] = color.r * 255;     // R
        data[idx + 1] = color.g * 255; // G
        data[idx + 2] = color.b * 255; // B
        data[idx + 3] = 255;           // A
      }
    }
    
    ctx.putImageData(imageData, 0, 0);
  }, [currentTime, internalTime, pattern, animConfig, height]);
  
  // Separate animation loop (only depends on isPlaying)
  useEffect(() => {
    if (!isPlaying || currentTime !== undefined) return; // Only run internal loop if not externally controlled
    
    const animate = () => {
      const newTime = Date.now() - startTimeRef.current;
      setInternalTime(newTime);
      if (onTimeUpdate) onTimeUpdate(newTime);
      animationFrameRef.current = requestAnimationFrame(animate);
    };
    
    animationFrameRef.current = requestAnimationFrame(animate);
    
    return () => {
      if (animationFrameRef.current) {
        cancelAnimationFrame(animationFrameRef.current);
      }
    };
  }, [isPlaying, currentTime, onTimeUpdate]);

  // Reset start time when pattern changes
  useEffect(() => {
    startTimeRef.current = Date.now();
    setInternalTime(0);
  }, [pattern]);

  return (
    <div className={`k1-device-animated ${className}`} style={containerStyle}>
      {/* Outer Glow */}
      <div
        className="absolute -inset-8 rounded-xl opacity-15 pointer-events-none"
        style={{
          background: animConfig.ch1Colors[0],
          filter: 'blur(36px)',
          mixBlendMode: 'screen',
        }}
      />

      {/* Device Frame */}
      <div
        className="absolute inset-0 rounded-xl overflow-hidden"
        style={{
          backgroundColor: isDark ? '#161A22' : '#ECEFF4',
          boxShadow: angle === 'front' 
            ? '0 4px 10px rgba(0,0,0,0.45)' 
            : '0 8px 24px rgba(0,0,0,0.5)',
        }}
      >
        {/* Frame Inner Stroke */}
        <div
          className="absolute inset-0 rounded-xl pointer-events-none"
          style={{
            border: `1px solid ${isDark ? 'rgba(255,255,255,0.06)' : 'rgba(0,0,0,0.06)'}`,
          }}
        />

        {/* LGP Window (12px inset) */}
        <div
          className="absolute rounded-lg overflow-hidden"
          style={{
            top: 12,
            left: 12,
            right: 12,
            bottom: 12,
            borderRadius: 8,
          }}
        >
          {/* Canvas for animated LGP surface */}
          <canvas
            ref={canvasRef}
            style={{
              width: DEVICE_WIDTH,
              height: height,
              imageRendering: 'auto',
              filter: 'blur(8px)', // Simulate frosted acrylic diffusion
            }}
          />
          
          {/* Frost overlay */}
          <div
            className="absolute inset-0 pointer-events-none"
            style={{
              backgroundColor: 'rgba(255,255,255,0.08)',
              mixBlendMode: isDark ? 'screen' : 'overlay',
            }}
          />
          
          {/* Micro-sheen */}
          <div
            className="absolute inset-0 pointer-events-none"
            style={{
              background: 'linear-gradient(15deg, rgba(255,255,255,0.03) 0%, transparent 100%)',
              opacity: 0.4,
            }}
          />
        </div>
      </div>

      {/* Three-Quarter Angle: Desk Reflection */}
      {angle === 'threeQuarter' && (
        <div
          className="absolute pointer-events-none"
          style={{
            top: '100%',
            left: 0,
            right: 0,
            height: height * 0.4,
            background: 'linear-gradient(to bottom, rgba(255,255,255,0.08) 0%, transparent 100%)',
            filter: 'blur(8px)',
            opacity: 0.12,
            transform: 'scaleY(-1) translateY(4px)',
          }}
        />
      )}
    </div>
  );
}
