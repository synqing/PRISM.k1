import { CSSProperties } from 'react';

interface K1DeviceProps {
  heightVariant?: 'h50' | 'h60' | 'h70';
  finish?: 'dark-frame' | 'light-frame';
  glowStyle?: 'magentaFire' | 'cyanTeal' | 'triLobe';
  angle?: 'front' | 'threeQuarter';
  className?: string;
}

// Scale: 1px = 0.25mm, so 330mm = 1320px
const DEVICE_WIDTH = 1320;
const HEIGHT_MAP = {
  h50: 200,   // 50mm
  h60: 240,   // 60mm
  h70: 280,   // 70mm
};

// Glow style color configurations
const GLOW_STYLES = {
  magentaFire: {
    topLobes: ['#7B68EE', '#5B8CFF'],
    bottomLobes: ['#FF2A2A', '#FF7A1A', '#FFC64D'],
    additive: ['#A06BFF', '#7AD0FF'],
    outerGlow: '#FF5A5A',
  },
  cyanTeal: {
    topLobes: ['#1E90FF', '#00CED1'],
    bottomLobes: ['#30E384', '#7AE96A', '#FFE38A'],
    additive: ['#7AD0FF', '#6BE6C1'],
    outerGlow: '#00CED1',
  },
  triLobe: {
    topLobes: ['#7B68EE', '#5B8CFF'],
    bottomLobes: ['#FF2A2A', '#FF7A1A', '#FFC64D'],
    additive: ['#A06BFF', '#7AD0FF'],
    outerGlow: '#9B59B6',
  },
};

export function K1Device({
  heightVariant = 'h60',
  finish = 'dark-frame',
  glowStyle = 'magentaFire',
  angle = 'front',
  className = '',
}: K1DeviceProps) {
  const height = HEIGHT_MAP[heightVariant];
  const colors = GLOW_STYLES[glowStyle];
  const isDark = finish === 'dark-frame';
  
  // Lobe positions
  const topLobeX = glowStyle === 'triLobe' 
    ? [18, 50, 82] 
    : [8, 33, 58, 83];
  const bottomLobeX = glowStyle === 'triLobe' 
    ? [12, 46, 80] 
    : [15, 40, 65, 90];

  const containerStyle: CSSProperties = {
    width: DEVICE_WIDTH,
    height: height,
    position: 'relative',
    ...(angle === 'threeQuarter' && {
      transform: 'perspective(2000px) rotateY(-6deg) rotateX(2deg)',
      transformStyle: 'preserve-3d',
    }),
  };

  return (
    <div className={`k1-device ${className}`} style={containerStyle}>
      {/* Outer Glow */}
      <div
        className="absolute -inset-8 rounded-xl opacity-15 pointer-events-none"
        style={{
          background: colors.outerGlow,
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
          {/* LGP Layers Stack */}
          <div className="absolute inset-0">
            {/* 1. Base Frost */}
            <div
              className="absolute inset-0"
              style={{
                backgroundColor: '#FFFFFF',
                filter: 'blur(6px)',
                opacity: 0.88,
              }}
            />

            {/* 2. Vertical Edge Feather */}
            <div
              className="absolute inset-0"
              style={{
                background: 'linear-gradient(to bottom, rgba(255,255,255,0.16) 0%, rgba(255,255,255,0.02) 50%, rgba(255,255,255,0.16) 100%)',
                filter: 'blur(12px)',
                mixBlendMode: isDark ? 'screen' : 'overlay',
              }}
            />

            {/* 3. Top Edge Injection (SVG Radial Lobes) */}
            <svg
              className="absolute inset-0 w-full h-full pointer-events-none"
              style={{ filter: 'blur(28px)', mixBlendMode: 'screen' }}
            >
              <defs>
                {topLobeX.map((xPercent, i) => (
                  <radialGradient key={`top-${i}`} id={`topLobe${i}-${glowStyle}`} cx="50%" cy="50%">
                    <stop offset="0%" stopColor={colors.topLobes[i % colors.topLobes.length]} stopOpacity="0.85" />
                    <stop offset="40%" stopColor={colors.topLobes[i % colors.topLobes.length]} stopOpacity="0.5" />
                    <stop offset="100%" stopColor={colors.topLobes[i % colors.topLobes.length]} stopOpacity="0" />
                  </radialGradient>
                ))}
              </defs>
              {topLobeX.map((xPercent, i) => (
                <ellipse
                  key={`top-ellipse-${i}`}
                  cx={`${xPercent}%`}
                  cy="12%"
                  rx={height * 1.1}
                  ry={height * 0.7}
                  fill={`url(#topLobe${i}-${glowStyle})`}
                />
              ))}
            </svg>

            {/* 4. Bottom Edge Injection (SVG Radial Lobes) */}
            <svg
              className="absolute inset-0 w-full h-full pointer-events-none"
              style={{ filter: 'blur(26px)', mixBlendMode: 'screen' }}
            >
              <defs>
                {bottomLobeX.map((xPercent, i) => (
                  <radialGradient key={`bottom-${i}`} id={`bottomLobe${i}-${glowStyle}`} cx="50%" cy="50%">
                    <stop offset="0%" stopColor={colors.bottomLobes[i % colors.bottomLobes.length]} stopOpacity="0.9" />
                    <stop offset="35%" stopColor={colors.bottomLobes[i % colors.bottomLobes.length]} stopOpacity="0.6" />
                    <stop offset="100%" stopColor={colors.bottomLobes[i % colors.bottomLobes.length]} stopOpacity="0" />
                  </radialGradient>
                ))}
              </defs>
              {bottomLobeX.map((xPercent, i) => (
                <ellipse
                  key={`bottom-ellipse-${i}`}
                  cx={`${xPercent}%`}
                  cy="88%"
                  rx={height * 1.1}
                  ry={height * 0.7}
                  fill={`url(#bottomLobe${i}-${glowStyle})`}
                />
              ))}
            </svg>

            {/* 5. Additive Field (Mid-Panel Blends) */}
            <svg
              className="absolute inset-0 w-full h-full pointer-events-none"
              style={{ filter: 'blur(32px)', mixBlendMode: 'screen', opacity: 0.22 }}
            >
              <defs>
                <linearGradient id={`horizontal-${glowStyle}`} x1="0%" y1="0%" x2="100%" y2="0%">
                  <stop offset="0%" stopColor={colors.additive[0]} stopOpacity="0.3" />
                  <stop offset="50%" stopColor={colors.additive[1]} stopOpacity="0.5" />
                  <stop offset="100%" stopColor={colors.additive[0]} stopOpacity="0.3" />
                </linearGradient>
                <radialGradient id={`pool1-${glowStyle}`} cx="30%" cy="50%">
                  <stop offset="0%" stopColor={colors.additive[0]} stopOpacity="0.6" />
                  <stop offset="100%" stopColor={colors.additive[0]} stopOpacity="0" />
                </radialGradient>
                <radialGradient id={`pool2-${glowStyle}`} cx="70%" cy="50%">
                  <stop offset="0%" stopColor={colors.additive[1]} stopOpacity="0.6" />
                  <stop offset="100%" stopColor={colors.additive[1]} stopOpacity="0" />
                </radialGradient>
              </defs>
              <rect width="100%" height="100%" fill={`url(#horizontal-${glowStyle})`} />
              <ellipse cx="30%" cy="50%" rx={DEVICE_WIDTH * 0.25} ry={height * 0.8} fill={`url(#pool1-${glowStyle})`} />
              <ellipse cx="70%" cy="50%" rx={DEVICE_WIDTH * 0.25} ry={height * 0.8} fill={`url(#pool2-${glowStyle})`} />
            </svg>

            {/* 6. Micro-Sheen & Noise */}
            <div
              className="absolute inset-0"
              style={{
                background: 'linear-gradient(15deg, rgba(255,255,255,0.03) 0%, transparent 100%)',
                opacity: 0.4,
              }}
            />
            <div
              className="absolute inset-0 pointer-events-none"
              style={{
                backgroundImage: `url("data:image/svg+xml,%3Csvg viewBox='0 0 200 200' xmlns='http://www.w3.org/2000/svg'%3E%3Cfilter id='noiseFilter'%3E%3CfeTurbulence type='fractalNoise' baseFrequency='0.9' numOctaves='3' stitchTiles='stitch'/%3E%3C/filter%3E%3Crect width='100%25' height='100%25' filter='url(%23noiseFilter)'/%3E%3C/svg%3E")`,
                opacity: 0.02,
                mixBlendMode: 'overlay',
              }}
            />
          </div>
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
