#include <iostream>
#include <unistd.h>
#include <cstring>
#include <atomic>
#include <errno.h>

#include "udp_socket.h"

std::atomic<bool> running(true);

UDPSocket::UDPSocket(const std::string &ip, int port) : clientAddressLength(sizeof(clientAddress)), buffer{0}
{
    if ((socket_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }
    memset(&serverAddress, 0, sizeof(serverAddress));
    memset(&clientAddress, 0, sizeof(clientAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port);
    if (inet_pton(AF_INET, ip.c_str(), &serverAddress.sin_addr) <= 0)
    {
        std::cerr << "Invalid IP address: " << ip << std::endl;
        exit(EXIT_FAILURE);
    }
    std::cout << "Socket created" << std::endl;
    if (bind(socket_fd, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0)
    {
        perror("Bind failed");
        close(socket_fd);
        exit(EXIT_FAILURE);
    }
    running.store(true);
}

UDPSocket::~UDPSocket()
{
    close(socket_fd);
    running.store(false);
    std::cout << "Socket closed" << std::endl;
}

ssize_t UDPSocket::receive(uint8_t *buf, size_t max_len, sockaddr_in &client_addr)
{
    while (running)
    {
        socklen_t addr_len = sizeof(client_addr);
        ssize_t n = recvfrom(socket_fd, buf, max_len, 0,
                             (struct sockaddr *)&client_addr, &addr_len);
        if (n < 0)
        {
            if (errno == EINTR) continue;  // interrupted by signal — retry
            perror("recvfrom failed");
            return -1;
        }
        return n;
    }
    return -1;  // running was set to false (shutdown)
}

void UDPSocket::send_to(const uint8_t *buf, size_t len, const sockaddr_in &addr)
{
    sendto(socket_fd, buf, len, 0,
           (struct sockaddr *)&addr, sizeof(addr));
}
