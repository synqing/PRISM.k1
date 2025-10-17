// firmware/render/renderer.h (PR TEMPLATE)
#pragma once
#include "geometry.h"
#include "palette.h"
#include "prog/program.h"
#include "led/led_ws281x.h"

struct RenderCtx {
  float time_s;
  float macros[4];
};

class Renderer {
public:
  Renderer(Geometry& g, PaletteBank& pal, LedDriver& led)
    : g_(g), pal_(pal), led_(led) {}
  void render_frame(const RenderCtx& ctx, Program& prog);
  float falloffK_ = 2.2f;
  float lobeStrength_ = 0.08f;
  float aa_dx_ = 0.002f, aa_dy_ = 0.0017f;
private:
  Geometry& g_; PaletteBank& pal_; LedDriver& led_;
};
