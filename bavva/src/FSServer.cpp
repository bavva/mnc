#include <iostream>
#include <cstdlib>
#include <cstdio>

#include "../include/FSServer.h"

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

void FSServer::start(void)
{
    printf("Starting server on port %d\n", port);
}
