#include "blebridge.h"
#include <string.h>

static uint32_t crc_table[256];
		// Build a frame into dst. Returns frame length in bytes, or 0 if dst_len too small.
size_t build_frame_from_buf(const uint8_t *payload, uint16_t payload_len,uint8_t *dst, size_t dst_len);
uint32_t crc32_compute(const uint8_t *data, size_t len) ;

static void crc32_init_once() 
{
  static bool inited = false;
  if (inited) return;
  inited = true;
  const uint32_t poly = 0xEDB88320UL;
  for (uint32_t i = 0; i < 256; ++i) {
    uint32_t c = i;
    for (int j = 0; j < 8; ++j) {
      if (c & 1) c = poly ^ (c >> 1);
      else c >>= 1;
    }
    crc_table[i] = c;
  }
}


// Build a frame from a raw byte buffer (payload may contain zero bytes)
size_t build_frame_from_buf(const uint8_t *payload, uint16_t payload_len,
                            uint8_t *dst, size_t dst_len) {
  const size_t overhead = 2 + 2 + 4;
  if (dst_len < overhead + payload_len) return 0;
  size_t idx = 0;
  dst[idx++] = 0x55; dst[idx++] = 0xAA;
  dst[idx++] = (uint8_t)((payload_len >> 8) & 0xFF);
  dst[idx++] = (uint8_t)(payload_len & 0xFF);
  if (payload_len) {
    memcpy(dst + idx, payload, payload_len);
    idx += payload_len;
  }
  uint32_t crc = crc32_compute(payload, payload_len);
  dst[idx++] = (uint8_t)(crc & 0xFF);
  dst[idx++] = (uint8_t)((crc >> 8) & 0xFF);
  dst[idx++] = (uint8_t)((crc >> 16) & 0xFF);
  dst[idx++] = (uint8_t)((crc >> 24) & 0xFF);
  return idx;
}

uint32_t crc32_compute(const uint8_t *data, size_t len) 
{
  crc32_init_once();
  uint32_t c = 0xFFFFFFFFUL;
  for (size_t i = 0; i < len; ++i) {
    uint8_t idx = (uint8_t)((c ^ data[i]) & 0xFF);
    c = crc_table[idx] ^ (c >> 8);
  }
  return c ^ 0xFFFFFFFFUL;
}

uint32_t BLEBRIDGE_LIB::computeCRC(const uint8_t *data, size_t len) 
{
  return crc32_compute(data, len);
}
size_t BLEBRIDGE_LIB::build_frame_from_cstr(const char *payload_cstr, uint8_t *dst, size_t dst_len) {
  if (!payload_cstr) return 0;
  size_t payload_len = strlen(payload_cstr);
  if (payload_len > 0xFFFF) return 0; // payload length must fit in uint16_t
  return build_frame_from_buf(reinterpret_cast<const uint8_t*>(payload_cstr),(uint16_t)payload_len, dst, dst_len);
}



size_t BLEBRIDGE_LIB::parse_frame(const uint8_t *buf, size_t buf_len,
                   size_t *payload_offset_out, uint16_t *payload_len_out, size_t *consumed_out) {
  const size_t header = 2 + 2;
  const size_t crc_size = 4;

  if (buf_len < header + crc_size) 
  {
    *consumed_out = 0;
    return 0;
  }
  // Check marker at start
  if (!(buf[0] == 0x55 && buf[1] == 0xAA)) {
    *consumed_out = 1; // drop one byte and resync
    return 0;
  }
  // Read length big-endian
  uint16_t payload_len = (uint16_t(buf[2]) << 8) | uint16_t(buf[3]);
  size_t full_len = header + payload_len + crc_size;
  if (payload_len > 0x8000) { *consumed_out = 1; return 0; } // sanity limit
  if (buf_len < full_len) {
    *consumed_out = 0; // wait for more bytes
    return 0;
  }
  const uint8_t *payload = buf + header;
  const uint8_t *crcptr = payload + payload_len;
  uint32_t recv_crc = (uint32_t)crcptr[0] | ((uint32_t)crcptr[1] << 8) | ((uint32_t)crcptr[2] << 16) | ((uint32_t)crcptr[3] << 24);
  uint32_t calc = crc32_compute(payload, payload_len);
  if (recv_crc != calc) {
    *consumed_out = 1; // bad CRC, drop leading byte to resync
    return 0;
  }
  *payload_offset_out = header;
  *payload_len_out = payload_len;
  *consumed_out = full_len;
  return payload_len;
}
