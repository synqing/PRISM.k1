// firmware/led/led_ws281x.h (PR TEMPLATE)
#pragma once
#include <cstdint>

class LedDriver {
public:
  bool begin(int pinEdge0, int pinEdge1, uint16_t edgeLen);
  void start_frame();
  void set_pixel(uint16_t idx, uint8_t r, uint8_t g, uint8_t b);
  void submit();
  void wait_done();
private:
  // TODO: two RMT channels; map idx→(edge, offset)
};
