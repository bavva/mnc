#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <unistd.h>
#include <cstring>

#include "../include/FSConnection.h"

// class implementation
FSConnection::FSConnection(bool with_server, std::string ip, int port):with_server(with_server), peer_ip(ip), peer_port(port)
{
    sock_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(sock_fd < 0)
        return;

    struct sockaddr_in peer_address;                    // peer address
    memset(&peer_address, 0, sizeof(peer_address));     // zero out the structure
    peer_address.sin_family = AF_INET;                  // IPv4 address family
    peer_address.sin_port = htons(port);                // peer port

    int ret = inet_pton(AF_INET, ip.c_str(), &peer_address.sin_addr.s_addr);
    if(ret <= 0)
        return;

    if(connect(sock_fd, (struct sockaddr *)&peer_address, sizeof(peer_address)) < 0)
    {
        std::cout << "Failed to connect to " << ip << " and port " << port << std::endl;
    }
}

FSConnection::FSConnection(bool with_server, std::string ip, int port, int fd):with_server(with_server), peer_ip(ip), peer_port(port), sock_fd(fd)
{
}

FSConnection::~FSConnection()
{
    if (sock_fd >= 0)
        close(sock_fd);
}
