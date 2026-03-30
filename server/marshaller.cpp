#include "marshaller.h"
#include "protocol.h"

#include <arpa/inet.h>   // htonl, ntohl, htons, ntohs
#include <cstring>       // memcpy
#include <stdexcept>
#include <string>

// ─── Writers ──────────────────────────────────────────────────────────────────

void write_int(uint8_t* buf, int& offset, int32_t val) {
    if (offset + 4 > MAX_BUF_SIZE)
        throw std::overflow_error("write_int: buffer overflow at offset " + std::to_string(offset));
    uint32_t net = htonl((uint32_t)val);
    memcpy(buf + offset, &net, 4);
    offset += 4;
}

void write_uint(uint8_t* buf, int& offset, uint32_t val) {
    if (offset + 4 > MAX_BUF_SIZE)
        throw std::overflow_error("write_uint: buffer overflow at offset " + std::to_string(offset));
    uint32_t net = htonl(val);
    memcpy(buf + offset, &net, 4);
    offset += 4;
}

void write_float(uint8_t* buf, int& offset, float val) {
    if (offset + 4 > MAX_BUF_SIZE)
        throw std::overflow_error("write_float: buffer overflow at offset " + std::to_string(offset));
    uint32_t tmp;
    memcpy(&tmp, &val, 4);
    tmp = htonl(tmp);
    memcpy(buf + offset, &tmp, 4);
    offset += 4;
}

void write_byte(uint8_t* buf, int& offset, uint8_t val) {
    if (offset + 1 > MAX_BUF_SIZE)
        throw std::overflow_error("write_byte: buffer overflow at offset " + std::to_string(offset));
    buf[offset] = val;
    offset += 1;
}

void write_string(uint8_t* buf, int& offset, const std::string& s) {
    if (s.size() > 65535)
        throw std::length_error("write_string: string length " + std::to_string(s.size()) + " exceeds uint16 max");
    if (offset + 2 + (int)s.size() > MAX_BUF_SIZE)
        throw std::overflow_error("write_string: buffer overflow at offset " + std::to_string(offset));
    uint16_t len = htons((uint16_t)s.size());
    memcpy(buf + offset, &len, 2);
    offset += 2;
    memcpy(buf + offset, s.data(), s.size());
    offset += (int)s.size();
}

void write_fixed_string(uint8_t* buf, int& offset, const std::string& s, int len) {
    if (offset + len > MAX_BUF_SIZE)
        throw std::overflow_error("write_fixed_string: buffer overflow at offset " + std::to_string(offset));
    memset(buf + offset, 0, len);
    int copy_len = (int)s.size() < len ? (int)s.size() : len;
    memcpy(buf + offset, s.data(), copy_len);
    offset += len;
}

void write_request_id(uint8_t* buf, int& offset, const uint8_t* request_id) {
    if (offset + 16 > MAX_BUF_SIZE)
        throw std::overflow_error("write_request_id: buffer overflow at offset " + std::to_string(offset));
    memcpy(buf + offset, request_id, 16);
    offset += 16;
}

// ─── Readers ──────────────────────────────────────────────────────────────────

int32_t read_int(const uint8_t* buf, int& offset, int buf_len) {
    if (offset + 4 > buf_len)
        throw std::out_of_range("read_int: need 4 bytes at offset " + std::to_string(offset) + ", buf_len=" + std::to_string(buf_len));
    uint32_t net;
    memcpy(&net, buf + offset, 4);
    offset += 4;
    return (int32_t)ntohl(net);
}

uint32_t read_uint(const uint8_t* buf, int& offset, int buf_len) {
    if (offset + 4 > buf_len)
        throw std::out_of_range("read_uint: need 4 bytes at offset " + std::to_string(offset) + ", buf_len=" + std::to_string(buf_len));
    uint32_t net;
    memcpy(&net, buf + offset, 4);
    offset += 4;
    return ntohl(net);
}

float read_float(const uint8_t* buf, int& offset, int buf_len) {
    if (offset + 4 > buf_len)
        throw std::out_of_range("read_float: need 4 bytes at offset " + std::to_string(offset) + ", buf_len=" + std::to_string(buf_len));
    uint32_t net;
    memcpy(&net, buf + offset, 4);
    offset += 4;
    uint32_t host = ntohl(net);
    float val;
    memcpy(&val, &host, 4);
    return val;
}

uint8_t read_byte(const uint8_t* buf, int& offset, int buf_len) {
    if (offset + 1 > buf_len)
        throw std::out_of_range("read_byte: need 1 byte at offset " + std::to_string(offset) + ", buf_len=" + std::to_string(buf_len));
    uint8_t val = buf[offset];
    offset += 1;
    return val;
}

std::string read_string(const uint8_t* buf, int& offset, int buf_len) {
    if (offset + 2 > buf_len)
        throw std::out_of_range("read_string: need 2-byte length prefix at offset " + std::to_string(offset) + ", buf_len=" + std::to_string(buf_len));
    uint16_t net_len;
    memcpy(&net_len, buf + offset, 2);
    offset += 2;
    uint16_t len = ntohs(net_len);
    if (offset + len > buf_len)
        throw std::out_of_range("read_string: need " + std::to_string(len) + " bytes at offset " + std::to_string(offset) + ", buf_len=" + std::to_string(buf_len));
    std::string s(reinterpret_cast<const char*>(buf + offset), len);
    offset += len;
    return s;
}

std::string read_fixed_string(const uint8_t* buf, int& offset, int buf_len, int len) {
    if (offset + len > buf_len)
        throw std::out_of_range("read_fixed_string: need " + std::to_string(len) + " bytes at offset " + std::to_string(offset) + ", buf_len=" + std::to_string(buf_len));
    std::string s(reinterpret_cast<const char*>(buf + offset), len);
    offset += len;
    size_t end = s.find('\0');
    if (end != std::string::npos) s.resize(end);
    return s;
}

void read_request_id(const uint8_t* buf, int& offset, int buf_len, uint8_t* dest) {
    if (offset + 16 > buf_len)
        throw std::out_of_range("read_request_id: need 16 bytes at offset " + std::to_string(offset) + ", buf_len=" + std::to_string(buf_len));
    memcpy(dest, buf + offset, 16);
    offset += 16;
}
