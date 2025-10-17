# TLV v0.9 — Transport Types (PROPOSED)
WebSocket endpoint `/ws` on device (esp_http_server with WS enabled). Each frame is:
```
struct Frame {
  uint8_t  type;    // see MsgType
  uint8_t  flags;   // reserved 0
  uint16_t rsv;     // 0
  uint32_t seq;     // increasing
  uint32_t len;     // payload length
  uint32_t crc32;   // IEEE 0xEDB88320 over payload
  uint8_t  payload[len];
}
```
**Types**
- 0x10 PUT_BEGIN  { format:u8 (1=PRISM_CLIP, 2=K1_PROGRAM), total_len:u32, name_len:u16, name[] }
- 0x11 PUT_DATA   { offset:u32, data[] }
- 0x12 PUT_END    { final_crc:u32 }
- 0x20 PLAY       { pattern_id:u32 }
- 0x21 DELETE     { pattern_id:u32 }
- 0x22 LIST       { empty }
- 0x30 STATUS     { empty } → reply with device JSON
- 0x40 SET_PARAM  { id:u16, value_f32 }
