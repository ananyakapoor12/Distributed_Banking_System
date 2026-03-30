#pragma once

#include <cstdint>
#include <string>
#include <stdexcept>

// ─── Writers ──────────────────────────────────────────────────────────────────
// Each function appends its field to buf[] starting at offset,
// then advances offset by the number of bytes written.
// Throws std::overflow_error if the write would exceed MAX_BUF_SIZE.
// Throws std::length_error if a string value exceeds the wire-format limit.

void write_int         (uint8_t* buf, int& offset, int32_t val);
void write_uint        (uint8_t* buf, int& offset, uint32_t val);
void write_float       (uint8_t* buf, int& offset, float val);
void write_byte        (uint8_t* buf, int& offset, uint8_t val);
void write_string      (uint8_t* buf, int& offset, const std::string& s);
void write_fixed_string(uint8_t* buf, int& offset, const std::string& s, int len);
void write_request_id  (uint8_t* buf, int& offset, const uint8_t* request_id);

// ─── Readers ──────────────────────────────────────────────────────────────────
// Each function reads its field from buf[] starting at offset,
// then advances offset by the number of bytes consumed.
// buf_len is the total valid length of buf[].
// Throws std::out_of_range if the read would go past buf_len.

int32_t     read_int         (const uint8_t* buf, int& offset, int buf_len);
uint32_t    read_uint        (const uint8_t* buf, int& offset, int buf_len);
float       read_float       (const uint8_t* buf, int& offset, int buf_len);
uint8_t     read_byte        (const uint8_t* buf, int& offset, int buf_len);
std::string read_string      (const uint8_t* buf, int& offset, int buf_len);
std::string read_fixed_string(const uint8_t* buf, int& offset, int buf_len, int len);
void        read_request_id  (const uint8_t* buf, int& offset, int buf_len, uint8_t* dest);
