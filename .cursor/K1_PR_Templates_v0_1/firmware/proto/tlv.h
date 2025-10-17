// firmware/proto/tlv.h (PR TEMPLATE)
#pragma once
#include <cstdint>
#include <vector>

namespace proto {

enum class MsgType : uint8_t {
  PUT_BEGIN=0x10, PUT_DATA=0x11, PUT_END=0x12,
  PLAY=0x20, DELETE_=0x21, LIST=0x22, STATUS=0x30,
  SET_PARAM=0x40
};

#pragma pack(push,1)
struct FrameHeader {
  uint8_t  type;
  uint8_t  flags;
  uint16_t rsv;
  uint32_t seq;
  uint32_t len;
  uint32_t crc32; // IEEE/zlib poly 0xEDB88320
};
#pragma pack(pop)

uint32_t crc32_ieee(const void* data, size_t len);

bool parse_frame(const uint8_t* buf, size_t len, FrameHeader& out, std::vector<uint8_t>& payload);
bool build_frame(MsgType t, const std::vector<uint8_t>& payload, std::vector<uint8_t>& out);

} // namespace proto
