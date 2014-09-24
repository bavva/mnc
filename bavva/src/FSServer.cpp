#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <unistd.h>
#include <cstring>
#include <algorithm>

#include "../include/FSServer.h"

// parameters
static const int MAXPENDING = 5;

// class implementation
FSServer::FSServer(int port):port(port)
{
    write_here = 0;

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

void FSServer::process_command(std::string& command)
{
    if (command == "creator")
    {
        printf("Bharadwaj Avva\nbavva\nbavva@buffalo.edu\n");
        printf("I have read and understood the course academic integrity policy located at http://www.cse.buffalo.edu/faculty/dimitrio/courses/cse4589_f14/index.html#integrity\n");
    }
    else if (command == "help")
    {
        printf("Following commands are available to use\n"
                "1. CREATOR: Displays full name, UBIT name and UB email address of creator\n"
                "2. HELP: Displays this help information\n"
                "3. MYIP: Displays the IP address of this process\n"
                "4. MYPORT: Displays the port on which this process is listening for incoming connections\n");
    }
    else
    {
        std::cout << "Unknown command " << command << " entered\n";
    }
}

void FSServer::start(void)
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
