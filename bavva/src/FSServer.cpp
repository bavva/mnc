#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <unistd.h>
#include <cstring>

#include "../include/FSServer.h"

// parameters
static const int MAXPENDING = 5;

// class implementation
FSServer::FSServer(int port):port(port)
{
    FD_ZERO(&read_fds);
    FD_ZERO(&write_fds);

    do_bind();
}

FSServer::~FSServer()
{
    close(listen_fd);
}

void FSServer::update_maxfd(void)
{
    unsigned max = 0;

    for (std::vector<int>::iterator it = read_sockets.begin(); it != read_sockets.end(); it++)
    {
        if (*it > max)
            max = *it;
    }

    for (std::vector<int>::iterator it = write_sockets.begin(); it != write_sockets.end(); it++)
    {
        if (*it > max)
            max = *it;
    }

    max_fd = max;
}

void FSServer::insert_readfd(int fd)
{
    read_sockets.insert(read_sockets.end(), fd);
    FD_SET(fd, &read_fds);

    if (fd > max_fd)
        max_fd = fd;
}

void FSServer::insert_writefd(int fd)
{
    write_sockets.insert(write_sockets.end(), fd);
    FD_SET(fd, &write_fds);

    if (fd > max_fd)
        max_fd = fd;
}

void FSServer::remove_readfd(int fd)
{
    for (std::vector<int>::iterator it = read_sockets.begin(); it != read_sockets.end(); it++)
    {
        if (*it == fd)
        {
            read_sockets.erase(it);
            break;
        }
    }

    FD_CLR(fd, &read_fds);

    if (fd == max_fd)
        update_maxfd();
}

void FSServer::remove_writefd(int fd)
{
    for (std::vector<int>::iterator it = write_sockets.begin(); it != write_sockets.end(); it++)
    {
        if (*it == fd)
        {
            write_sockets.erase(it);
            break;
        }
    }

    FD_CLR(fd, &write_fds);

    if (fd == max_fd)
        update_maxfd();
}

void FSServer::do_bind(void)
{
    if((listen_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    {
        printf("socket() failed\n");
        exit(-1);
    }

    //Construct local address structure
    struct sockaddr_in servAddr;                    // Local address
    memset(&servAddr, 0 , sizeof(servAddr));        // Zero out structure
    servAddr.sin_family = AF_INET;                  // IPv4 address family
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);   // Any incoming interface
    servAddr.sin_port = htons(port);                // Local port

    // Bind to the local address
    if(bind(listen_fd, (struct sockaddr*) &servAddr, sizeof(servAddr)) < 0)
    {
        printf("bind() failed");
        exit(-1);
    }

    // Mark the socket so it will listen for incoming connections
    if(listen(listen_fd, MAXPENDING) < 0)
    {
        printf("listen() failed");
        exit(-1);
    }
}

void FSServer::start(void)
{
    insert_readfd(STDIN_FILENO);
    insert_readfd(listen_fd);

    while(1)
    {
        if (select(max_fd + 1, &read_fds, &write_fds, NULL, NULL) < 0)
            break;

        if (FD_ISSET(STDIN_FILENO, &read_fds))
        {
            std::string command;
            std::cin >> command;
            std::cout << "command is " << command;
        }
    }
}
