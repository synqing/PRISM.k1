// firmware/prog/program.h (PR TEMPLATE)
#pragma once
#include <cstdint>

struct Uniforms {
  float time_s;
  float macros[4];
};

class Program {
public:
  bool load(const uint8_t* blob, size_t len);
  void set_param(uint16_t id, float v);
  void begin_frame(const Uniforms& u);
  void eval_pixel(uint16_t idx, uint8_t& r, uint8_t& g, uint8_t& b) const;
};
