#include <iostream>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <atomic>
#include <csignal>
#include <string>
#include <sys/socket.h>
#include <errno.h>

#include "protocol.h"

std::atomic<bool> running(true);

class UDPSocket
{
    public:
        int socket_fd;
        sockaddr_in serverAddress, clientAddress;
        socklen_t clientAddressLength = sizeof(clientAddress);
        unsigned char buffer[MAX_BUF_SIZE];

        UDPSocket(int port = 8014) : buffer{0}
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
            serverAddress.sin_addr.s_addr = INADDR_ANY;
            std::cout << "Socket created" << std::endl;
            if (bind(socket_fd, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0)
            {
                perror("Bind failed");
                close(socket_fd);
                exit(EXIT_FAILURE);
            }
            running.store(true);
        }

        ~UDPSocket()
        {
            close(socket_fd);
            running.store(false);
            std::cout << "Socket closed" << std::endl;
        }

        ssize_t receive(uint8_t *buf, size_t max_len, sockaddr_in &client_addr)
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

        void send_to(const uint8_t *buf, size_t len, const sockaddr_in &addr)
        {
            sendto(socket_fd, buf, len, 0,
                   (struct sockaddr *)&addr, sizeof(addr));
        }
};
