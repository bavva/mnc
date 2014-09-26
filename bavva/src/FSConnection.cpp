#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <unistd.h>
#include <cstring>
#include <assert.h>

#include "../include/FSConnection.h"
#include "../include/FSNode.h"

// class implementation
FSConnection::FSConnection(bool with_server, std::string ip, int port, FSNode *fsnode):with_server(with_server), peer_ip(ip), peer_port(port), fsnode(fsnode)
{
    is_reading = true;

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
        return;
    }

    fsnode->insert_readfd(sock_fd);
}

FSConnection::FSConnection(bool with_server, std::string ip, int port, FSNode *fsnode, int fd):with_server(with_server), peer_ip(ip), peer_port(port), sock_fd(fd), fsnode(fsnode)
{
    assert(fd >= 0);
    assert(fsnode != NULL);

    is_reading = true;
    fsnode->insert_readfd(sock_fd);
}

FSConnection::~FSConnection()
{
    if (sock_fd >= 0)
    {
        if (is_reading)
            fsnode->remove_readfd(sock_fd);
        else
            fsnode->remove_writefd(sock_fd);
        close(sock_fd);
    }
}

void FSConnection::start_reading(void)
{
    if (is_reading)
        return; // we are ready to read

    fsnode->remove_writefd(sock_fd);
    fsnode->insert_readfd(sock_fd);

    is_reading = true;
}

void FSConnection::start_writing(void)
{
    if (is_reading == false)
        return; // we are ready to write

    fsnode->remove_readfd(sock_fd);
    fsnode->insert_writefd(sock_fd);

    is_reading = false;
}

void FSConnection::on_ready_toread(void)
{
}

void FSConnection::on_ready_towrite(void)
{
}
