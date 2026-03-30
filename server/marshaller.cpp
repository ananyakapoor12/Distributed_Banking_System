#include "marshaller.h"

#include <arpa/inet.h>   // htonl, ntohl, htons, ntohs
#include <cstring>       // memcpy

// ─── Writers ──────────────────────────────────────────────────────────────────

void write_int(uint8_t* buf, int& offset, int32_t val) {
    uint32_t net = htonl((uint32_t)val);
    memcpy(buf + offset, &net, 4);
    offset += 4;
}

void write_uint(uint8_t* buf, int& offset, uint32_t val) {
    uint32_t net = htonl(val);
    memcpy(buf + offset, &net, 4);
    offset += 4;
}

void write_float(uint8_t* buf, int& offset, float val) {
    // Safe type-pun: copy float bits into a uint32, then convert to network order
    uint32_t tmp;
    memcpy(&tmp, &val, 4);
    tmp = htonl(tmp);
    memcpy(buf + offset, &tmp, 4);
    offset += 4;
}

void write_byte(uint8_t* buf, int& offset, uint8_t val) {
    buf[offset] = val;
    offset += 1;
}

void write_string(uint8_t* buf, int& offset, const std::string& s) {
    // 2-byte length prefix (network byte order) followed by string bytes
    uint16_t len = htons((uint16_t)s.size());
    memcpy(buf + offset, &len, 2);
    offset += 2;
    memcpy(buf + offset, s.data(), s.size());
    offset += (int)s.size();
}

void write_fixed_string(uint8_t* buf, int& offset, const std::string& s, int len) {
    // Always writes exactly len bytes — zero-padded if s is shorter, truncated if longer
    memset(buf + offset, 0, len);
    int copy_len = (int)s.size() < len ? (int)s.size() : len;
    memcpy(buf + offset, s.data(), copy_len);
    offset += len;
}

void write_request_id(uint8_t* buf, int& offset, const uint8_t* request_id) {
    memcpy(buf + offset, request_id, 16);
    offset += 16;
}

// ─── Readers ──────────────────────────────────────────────────────────────────

int32_t read_int(const uint8_t* buf, int& offset) {
    uint32_t net;
    memcpy(&net, buf + offset, 4);
    offset += 4;
    return (int32_t)ntohl(net);
}

uint32_t read_uint(const uint8_t* buf, int& offset) {
    uint32_t net;
    memcpy(&net, buf + offset, 4);
    offset += 4;
    return ntohl(net);
}

float read_float(const uint8_t* buf, int& offset) {
    uint32_t net;
    memcpy(&net, buf + offset, 4);
    offset += 4;
    uint32_t host = ntohl(net);
    float val;
    memcpy(&val, &host, 4);
    return val;
}

uint8_t read_byte(const uint8_t* buf, int& offset) {
    uint8_t val = buf[offset];
    offset += 1;
    return val;
}

std::string read_string(const uint8_t* buf, int& offset) {
    uint16_t net_len;
    memcpy(&net_len, buf + offset, 2);
    offset += 2;
    uint16_t len = ntohs(net_len);
    std::string s(reinterpret_cast<const char*>(buf + offset), len);
    offset += len;
    return s;
}

std::string read_fixed_string(const uint8_t* buf, int& offset, int len) {
    // Read exactly len bytes; strip trailing null padding before returning
    std::string s(reinterpret_cast<const char*>(buf + offset), len);
    offset += len;
    // Trim null padding
    size_t end = s.find('\0');
    if (end != std::string::npos) s.resize(end);
    return s;
}

void read_request_id(const uint8_t* buf, int& offset, uint8_t* dest) {
    memcpy(dest, buf + offset, 16);
    offset += 16;
}
