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

    while(!connections.empty())
        connections.pop_front();
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

    for (std::list<int>::iterator it = read_sockets.begin(); it != read_sockets.end(); it++)
    {
        if (*it > max)
            max = *it;
    }

    for (std::list<int>::iterator it = write_sockets.begin(); it != write_sockets.end(); it++)
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
    for (std::list<int>::iterator it = read_sockets.begin(); it != read_sockets.end(); it++)
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
    for (std::list<int>::iterator it = write_sockets.begin(); it != write_sockets.end(); it++)
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

void FSNode::process_command(std::string args[])
{
    std::cout << "FSNode does not process any commands\n";
}

void FSNode::process_newconnection(FSConnection *connection)
{
    std::cout << "FSNode does not process any connections\n";
    delete connection;
}

void FSNode::start(void)
{
    int nbytes;
    std::string args[3];
    char space[] = " ";

    fd_set temp_read_fds;
    fd_set temp_write_fds;

    insert_readfd(STDIN_FILENO);
    insert_readfd(listen_fd);

    while(1)
    {
        temp_read_fds = read_fds;
        temp_write_fds = write_fds;

        if (select(max_fd + 1, &temp_read_fds, &temp_write_fds, NULL, NULL) < 0)
            break;

        if (FD_ISSET(listen_fd, &temp_read_fds))
        {
            struct sockaddr_in peer_address;
            socklen_t peer_addresslen = sizeof(peer_address);

            int peer_fd = accept(listen_fd, (struct sockaddr *) &peer_address, &peer_addresslen);
            if (peer_fd >= 0)
            {
                char peer_name[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &peer_address.sin_addr.s_addr, peer_name, sizeof(peer_name));
                process_newconnection(new FSConnection(false, peer_name, ntohs(peer_address.sin_port), peer_fd, this));
            }
        }

        for (std::list<FSConnection*>::const_iterator it = connections.begin(); it != connections.end(); it++)
        {
            if ((*it)->is_reading)
            {
                if (FD_ISSET((*it)->sock_fd, &temp_read_fds))
                    (*it)->on_ready_toread();
            }
            else
            {
                if (FD_ISSET((*it)->sock_fd, &temp_write_fds))
                    (*it)->on_ready_towrite();
            }
        }

        if (FD_ISSET(STDIN_FILENO, &temp_read_fds))
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
                command_buffer[write_here - 1] = '\0';
                write_here = 0;

                char *token = strtok(command_buffer, space);
                for (int i = 0; i < 3; i++)
                {
                    if (token == NULL)
                        break;

                    args[i] = token;
                    token = strtok(NULL, space);
                }

                std::transform(args[0].begin(), args[0].end(), args[0].begin(), tolower);
                process_command(args);

                args[0].clear();
                args[1].clear();
                args[2].clear();
            }
        }
    }
}
