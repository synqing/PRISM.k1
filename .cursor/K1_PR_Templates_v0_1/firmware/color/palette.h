// firmware/color/palette.h (PR TEMPLATE)
#pragma once
#include <cstdint>

class PaletteBank {
public:
  const uint8_t* lut(uint8_t idx) const { return nullptr; } // 256*3 bytes
};
