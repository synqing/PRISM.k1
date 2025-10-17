// firmware/proto/tlv.cpp (PR TEMPLATE)
#include "tlv.h"
namespace proto {
uint32_t crc32_ieee(const void* data, size_t len) {
  // TODO: table-driven CRC-32 (poly 0xEDB88320)
  return 0;
}
bool parse_frame(const uint8_t* buf, size_t len, FrameHeader& out, std::vector<uint8_t>& payload) {
  // TODO
  return false;
}
bool build_frame(MsgType t, const std::vector<uint8_t>& payload, std::vector<uint8_t>& out) {
  // TODO
  return false;
}
} // namespace proto
