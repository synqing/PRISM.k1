/**
 * UART Test Mode for PRISM.K1
 *
 * Simple line-oriented command interface over UART0 (115200-8N1) to drive
 * the same upload/control paths as the WS/TLV server, for bench bring-up.
 *
 * Commands:
 *  STATUS
 *  PLAY <name>
 *  STOP
 *  B <target_u8> <ms_u16>
 *  G <gamma_x100_u16> <ms_u16>
 *  BEGIN <name> <size_u32> <crc_u32_hex>
 *  DATA <offset_u32> <base64>
 *  END
 *
 * Replies: OK or ERR <msg> on each line.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "driver/uart.h"
#include "esp_log.h"
#include "protocol_parser.h"

static const char* TAG = "uart_test";

#define UART_PORT UART_NUM_0
#define UART_BAUD 115200
#define RX_BUF_SZ 2048

static void send_line(const char* s) { printf("%s\r\n", s); }

static esp_err_t dispatch_synthetic_tlv(uint8_t type, const uint8_t* payload, uint16_t len)
{
  // Build TLV frame in memory: TYPE(1) + LEN(2) + PAYLOAD + CRC(4)
  size_t total = 1 + 2 + len + 4;
  uint8_t* frame = (uint8_t*)malloc(total);
  if (!frame) return ESP_ERR_NO_MEM;
  frame[0] = type; frame[1] = (len >> 8) & 0xFF; frame[2] = len & 0xFF;
  if (len && payload) memcpy(&frame[3], payload, len);
  // CRC32 over header+payload (little-endian func, writing big-endian as per parser)
  extern uint32_t esp_rom_crc32_le(uint32_t crc, const uint8_t* buf, uint32_t len);
  uint32_t crc = esp_rom_crc32_le(0, frame, 3 + len);
  size_t off = 3 + len;
  frame[off+0] = (crc >> 24) & 0xFF;
  frame[off+1] = (crc >> 16) & 0xFF;
  frame[off+2] = (crc >> 8) & 0xFF;
  frame[off+3] = (crc) & 0xFF;
  // Use client_fd=1 (synthetic)
  esp_err_t ret = protocol_dispatch_command(frame, total, 1);
  free(frame);
  return ret;
}

static bool b64_decode(const char* in, uint8_t** out, size_t* out_len)
{
  // Minimal base64 decode via mbedtls (bundled with IDF)
  extern int mbedtls_base64_decode(unsigned char*, size_t, size_t*, const unsigned char*, size_t);
  size_t in_len = strlen(in);
  size_t needed = (in_len * 3) / 4 + 4;
  uint8_t* buf = (uint8_t*)malloc(needed);
  if (!buf) return false;
  size_t olen = 0;
  int rc = mbedtls_base64_decode(buf, needed, &olen, (const unsigned char*)in, in_len);
  if (rc != 0) { free(buf); return false; }
  *out = buf; *out_len = olen; return true;
}

static void uart_test_task(void* arg)
{
  (void)arg;
  uart_config_t cfg = { .baud_rate = UART_BAUD, .data_bits = UART_DATA_8_BITS, .parity = UART_PARITY_DISABLE, .stop_bits = UART_STOP_BITS_1, .flow_ctrl = UART_HW_FLOWCTRL_DISABLE };
  uart_param_config(UART_PORT, &cfg);
  uart_driver_install(UART_PORT, RX_BUF_SZ, 0, 0, NULL, 0);
  ESP_LOGI(TAG, "UART test mode ready at %d bps", UART_BAUD);
  printf("PRISM UART test mode ready\r\n");

  char line[2048]; size_t len = 0;
  while (1) {
    int c = fgetc(stdin);
    if (c == EOF) { vTaskDelay(pdMS_TO_TICKS(10)); continue; }
    if (c == '\r') continue;
    if (c != '\n') { if (len < sizeof(line)-1) line[len++] = (char)c; continue; }
    line[len] = '\0'; len = 0;
    // Parse command
    if (strncmp(line, "STATUS", 6) == 0) {
      if (dispatch_synthetic_tlv(MSG_TYPE_STATUS, NULL, 0) == ESP_OK) send_line("OK"); else send_line("ERR status");
    } else if (strncmp(line, "PLAY ", 5) == 0) {
      const char* name = line + 5; size_t nlen = strlen(name); if (nlen > 63) nlen = 63;
      uint8_t buf[1+1+64]; buf[0]=0x01; buf[1]=(uint8_t)nlen; memcpy(&buf[2], name, nlen);
      send_line(dispatch_synthetic_tlv(MSG_TYPE_CONTROL, buf, 2+nlen)==ESP_OK?"OK":"ERR play");
    } else if (strcmp(line, "STOP") == 0) {
      uint8_t cmd=0x02; send_line(dispatch_synthetic_tlv(MSG_TYPE_CONTROL, &cmd, 1)==ESP_OK?"OK":"ERR stop");
    } else if (line[0]=='B' && line[1]==' ') {
      unsigned t=0, ms=0; if (sscanf(line+2, "%u %u", &t, &ms)==2) { uint8_t buf[4]={0x10,(uint8_t)t,(uint8_t)(ms>>8),(uint8_t)ms}; send_line(dispatch_synthetic_tlv(MSG_TYPE_CONTROL, buf, 4)==ESP_OK?"OK":"ERR b"); } else send_line("ERR b args");
    } else if (line[0]=='G' && line[1]==' ') {
      unsigned gx=0, ms=0; if (sscanf(line+2, "%u %u", &gx, &ms)==2) { uint8_t buf[5]={0x11,(uint8_t)(gx>>8),(uint8_t)gx,(uint8_t)(ms>>8),(uint8_t)ms}; send_line(dispatch_synthetic_tlv(MSG_TYPE_CONTROL, buf, 5)==ESP_OK?"OK":"ERR g"); } else send_line("ERR g args");
    } else if (strncmp(line, "BEGIN ", 6)==0) {
      char name[64]={0}; unsigned size=0; unsigned crc=0; if (sscanf(line+6, "%63s %u %x", name, &size, &crc)==3) {
        size_t nlen=strlen(name); uint8_t buf[1+64+4+4]; buf[0]=(uint8_t)nlen; memcpy(&buf[1], name, nlen); buf[1+nlen]=(size>>24)&0xFF; buf[2+nlen]=(size>>16)&0xFF; buf[3+nlen]=(size>>8)&0xFF; buf[4+nlen]=(size)&0xFF; buf[5+nlen]=(crc>>24)&0xFF; buf[6+nlen]=(crc>>16)&0xFF; buf[7+nlen]=(crc>>8)&0xFF; buf[8+nlen]=(crc)&0xFF; send_line(dispatch_synthetic_tlv(MSG_TYPE_PUT_BEGIN, buf, (uint16_t)(9+nlen))==ESP_OK?"OK":"ERR begin");
      } else send_line("ERR begin args");
    } else if (strncmp(line, "DATA ", 5)==0) {
      unsigned offset=0; const char* sp=strchr(line+5,' '); if (!sp) { send_line("ERR data args"); continue; }
      if (sscanf(line+5, "%u", &offset)!=1) { send_line("ERR data off"); continue; }
      const char* b64 = sp+1; uint8_t* raw=NULL; size_t rlen=0; if (!b64_decode(b64, &raw, &rlen)) { send_line("ERR b64"); continue; }
      size_t plen = 4 + rlen; uint8_t* payload=(uint8_t*)malloc(plen); if (!payload) { free(raw); send_line("ERR mem"); continue; }
      payload[0]=(offset>>24)&0xFF; payload[1]=(offset>>16)&0xFF; payload[2]=(offset>>8)&0xFF; payload[3]=(offset)&0xFF; memcpy(&payload[4], raw, rlen);
      esp_err_t rc = dispatch_synthetic_tlv(MSG_TYPE_PUT_DATA, payload, (uint16_t)plen);
      free(payload); free(raw); send_line(rc==ESP_OK?"OK":"ERR data");
    } else if (strcmp(line, "END")==0) {
      send_line(dispatch_synthetic_tlv(MSG_TYPE_PUT_END, NULL, 0)==ESP_OK?"OK":"ERR end");
    } else if (line[0]=='\0' || strlen(line)==0) {
      // ignore
    } else {
      send_line("ERR unknown");
    }
  }
}

void uart_test_start(void)
{
  // Ensure protocol parser is initialized
  protocol_parser_init();
  xTaskCreatePinnedToCore(uart_test_task, "uart_test", 4096, NULL, 3, NULL, 0);
}

