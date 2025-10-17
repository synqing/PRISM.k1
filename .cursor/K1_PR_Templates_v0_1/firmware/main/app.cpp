// firmware/main/app.cpp (PR TEMPLATE)
#include "render/geometry.h"
#include "color/palette.h"
#include "prog/program.h"
#include "render/renderer.h"
#include "led/led_ws281x.h"

static Geometry    g_geom;
static PaletteBank g_pal;
static LedDriver   g_led;
static Program     g_prog;
static Renderer    g_renderer(g_geom, g_pal, g_led);

extern "C" void app_main(){
  // TODO: init Wi-Fi, start WS server, load default geometry, start tasks.
}
