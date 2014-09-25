#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <unistd.h>
#include <cstring>
#include <algorithm>

#include "../include/FSNode.h"

#define IP_IN_INTERNET "8.8.8.8"
#define PORT_OF_IP_IN_INTERNET  53

// parameters
static const int MAXPENDING = 5;

// class implementation
FSNode::FSNode(int port):port(port)
{
    write_here = 0;

    FD_ZERO(&read_fds);
    FD_ZERO(&write_fds);

    do_bind();
    update_localip();
}

FSNode::~FSNode()
{
    close(listen_fd);
}

void FSNode::update_localip(void)
{
    struct sockaddr_in remote_address;
    int sockfd;

    // create socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
    {
        printf("Unabled to create datagram socket\n");
        return;
    }

    memset((char *)&remote_address, 0, sizeof(remote_address));
    remote_address.sin_family = AF_INET;
    remote_address.sin_port = htons(PORT_OF_IP_IN_INTERNET);
    if (inet_aton(IP_IN_INTERNET, &remote_address.sin_addr) == 0)
    {
        printf("inet_aton failed\n");
        return;
    }

    if (connect(sockfd, (struct sockaddr *)&remote_address, sizeof(remote_address)) < 0)
    {
        printf("UDP connect failed\n");
        return;
    }

    struct sockaddr_in local_address;
    socklen_t addressLength = sizeof(local_address);
    getsockname(sockfd, (struct sockaddr*)&local_address, &addressLength);

    local_ip = inet_ntoa(local_address.sin_addr);

    close(sockfd);
}

void FSNode::update_maxfd(void)
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

void FSNode::insert_readfd(int fd)
{
    read_sockets.insert(read_sockets.end(), fd);
    FD_SET(fd, &read_fds);

    if (fd > max_fd)
        max_fd = fd;
}

void FSNode::insert_writefd(int fd)
{
    write_sockets.insert(write_sockets.end(), fd);
    FD_SET(fd, &write_fds);

    if (fd > max_fd)
        max_fd = fd;
}

void FSNode::remove_readfd(int fd)
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

void FSNode::remove_writefd(int fd)
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

void FSNode::do_bind(void)
{
    if((listen_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    {
        printf("socket() failed\n");
        exit(-1);
    }

    //Construct local address structure
    struct sockaddr_in local_address;                           // Local address
    memset(&local_address, 0 , sizeof(local_address));          // Zero out structure
    local_address.sin_family = AF_INET;                         // IPv4 address family
    local_address.sin_addr.s_addr = htonl(INADDR_ANY);          // Any incoming interface
    local_address.sin_port = htons(port);                       // Local port

    // Bind to the local address
    if(bind(listen_fd, (struct sockaddr*) &local_address, sizeof(local_address)) < 0)
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

void FSNode::process_command(std::string& command)
{
    std::cout << "FSNode does not process any commands\n";
}

void FSNode::start(void)
{
    int nbytes;
    std::string command;

    insert_readfd(STDIN_FILENO);
    insert_readfd(listen_fd);

    while(1)
    {
        if (select(max_fd + 1, &read_fds, &write_fds, NULL, NULL) < 0)
            break;

        if (FD_ISSET(STDIN_FILENO, &read_fds))
        {
            if ((nbytes = read(STDIN_FILENO, command_buffer + write_here, 
                            COMMAND_BUFFER - write_here)) <= 0)
            {
                if (nbytes == 0)
                    printf("Exiting at user's request\n");
                else
                    printf("recv failed with %d\n", nbytes);

                break;
            }

            write_here += nbytes;

            if (command_buffer[write_here - 1] == '\n')
            {
                command.assign(command_buffer, 0, write_here - 1);
                std::transform(command.begin(), command.end(), command.begin(), tolower);
                write_here = 0;

                process_command(command);
                command.clear();
            }
        }
    }
}
