#pragma once

#include <cstdint>
#include <string>

// ─── Writers ──────────────────────────────────────────────────────────────────
// Each function appends its field to buf[] starting at offset,
// then advances offset by the number of bytes written.
// Caller must ensure buf is large enough (use MAX_BUF_SIZE from protocol.h).

// Writes a 4-byte signed integer in network byte order (big-endian).
void write_int(uint8_t* buf, int& offset, int32_t val);

// Writes a 4-byte unsigned integer in network byte order.
void write_uint(uint8_t* buf, int& offset, uint32_t val);

// Writes a 4-byte float in network byte order.
// Uses memcpy + htonl — never casts float* to uint32_t* directly.
void write_float(uint8_t* buf, int& offset, float val);

// Writes a single byte (enums, status codes, opcodes, currency).
void write_byte(uint8_t* buf, int& offset, uint8_t val);

// Writes a variable-length string as: [2-byte length][N bytes of chars].
void write_string(uint8_t* buf, int& offset, const std::string& s);

// Writes a fixed-length string — exactly len bytes, no length prefix.
// Used for passwords (PASSWORD_LEN bytes). Pads with '\0' if s is shorter.
void write_fixed_string(uint8_t* buf, int& offset, const std::string& s, int len);

// Writes a 16-byte request ID (UUID) verbatim.
void write_request_id(uint8_t* buf, int& offset, const uint8_t* request_id);

// ─── Readers ──────────────────────────────────────────────────────────────────
// Each function reads its field from buf[] starting at offset,
// then advances offset by the number of bytes consumed.

int32_t     read_int         (const uint8_t* buf, int& offset);
uint32_t    read_uint        (const uint8_t* buf, int& offset);
float       read_float       (const uint8_t* buf, int& offset);
uint8_t     read_byte        (const uint8_t* buf, int& offset);
std::string read_string      (const uint8_t* buf, int& offset);
std::string read_fixed_string(const uint8_t* buf, int& offset, int len);

// Copies 16 bytes of request ID from buf into dest.
void        read_request_id  (const uint8_t* buf, int& offset, uint8_t* dest);
