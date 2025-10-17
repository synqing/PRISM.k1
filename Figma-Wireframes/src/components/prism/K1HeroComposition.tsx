import { K1Device } from './K1Device';
import heroImage from 'figma:asset/6348faf344d0135f0360c1a93d2c1bb3fec1bf8c.png';

interface K1HeroCompositionProps {
  glowStyle?: 'magentaFire' | 'cyanTeal' | 'triLobe';
  heightVariant?: 'h50' | 'h60' | 'h70';
  showEnvironment?: boolean;
}

export function K1HeroComposition({
  glowStyle = 'magentaFire',
  heightVariant = 'h60',
  showEnvironment = true,
}: K1HeroCompositionProps) {
  return (
    <div className="relative w-full h-full flex items-center justify-center bg-[#0D0F14] overflow-hidden">
      {/* Environment Elements */}
      {showEnvironment && (
        <>
          {/* Desk Surface Gradient */}
          <div
            className="absolute inset-x-0 bottom-0 h-1/3 pointer-events-none"
            style={{
              background: 'linear-gradient(to top, rgba(13,15,20,0.8) 0%, transparent 100%)',
            }}
          />
          
          {/* Keyboard Silhouette */}
          <div
            className="absolute bottom-16 left-1/2 -translate-x-1/2 w-[420px] h-[140px] rounded-lg opacity-20"
            style={{
              background: 'linear-gradient(to bottom, rgba(22,26,34,0.6) 0%, rgba(22,26,34,0.9) 100%)',
              transform: 'translateX(-50%) perspective(600px) rotateX(8deg)',
              boxShadow: '0 2px 8px rgba(0,0,0,0.4)',
            }}
          />
          
          {/* Monitor Glow (subtle) */}
          <div
            className="absolute top-8 left-1/2 -translate-x-1/2 w-[600px] h-[340px] rounded-lg opacity-5"
            style={{
              background: 'radial-gradient(ellipse at center, rgba(91,140,255,0.3) 0%, transparent 70%)',
              filter: 'blur(40px)',
            }}
          />
        </>
      )}

      {/* K1 Device - Hero Position */}
      <div
        className="relative z-10"
        style={{
          transform: 'translateY(-10%)',
        }}
      >
        <K1Device
          heightVariant={heightVariant}
          finish="dark-frame"
          glowStyle={glowStyle}
          angle="threeQuarter"
        />
      </div>

      {/* Floor Shadow */}
      <div
        className="absolute pointer-events-none z-0"
        style={{
          bottom: '15%',
          left: '50%',
          width: '1400px',
          height: '60px',
          background: 'radial-gradient(ellipse at center, rgba(0,0,0,0.6) 0%, transparent 70%)',
          transform: 'translateX(-50%)',
          filter: 'blur(20px)',
        }}
      />
    </div>
  );
}
