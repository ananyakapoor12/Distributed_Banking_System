#pragma once

#include <arpa/inet.h>
#include <sys/socket.h>
#include <cstdint>
#include <cstddef>
#include <string>

#include "protocol.h"

class UDPSocket
{
public:
    int socket_fd;
    sockaddr_in serverAddress, clientAddress;
    socklen_t clientAddressLength;
    unsigned char buffer[MAX_BUF_SIZE];

    UDPSocket(const std::string &ip, int port);
    ~UDPSocket();

    ssize_t receive(uint8_t *buf, size_t max_len, sockaddr_in &client_addr);
    void send_to(const uint8_t *buf, size_t len, const sockaddr_in &addr);
};
