// firmware/render/renderer.cpp (PR TEMPLATE)
#include "renderer.h"
static inline uint8_t to8(float x){ if(x<0) x=0; if(x>1) x=1; return (uint8_t)(x*255.0f + 0.5f); }
void Renderer::render_frame(const RenderCtx& ctx, Program& prog){
  led_.start_frame();
  const auto& P = g_.profile();
  for(uint16_t i=0;i<P.count;++i){
    uint8_t r=0,g=0,b=0;
    prog.eval_pixel(i,r,g,b);
    led_.set_pixel(i,r,g,b);
  }
  led_.submit();
}
