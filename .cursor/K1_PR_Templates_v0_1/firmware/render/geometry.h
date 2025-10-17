// firmware/render/geometry.h (PR TEMPLATE)
#pragma once
#include <cstdint>

struct GeoPixel {
  float u, v;
  float edgeW0, edgeW1;
  float gain;
};

struct GeometryProfile {
  uint32_t signature;
  float gamma;
  float whiteBalance[3];
  float maxBrightness;
  uint16_t count;
  GeoPixel px[320];
};

class Geometry {
public:
  bool load(const uint8_t* blob, size_t len);
  const GeometryProfile& profile() const { return prof_; }
private:
  GeometryProfile prof_{};
};
