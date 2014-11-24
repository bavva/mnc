#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <unistd.h>
#include <cstring>
#include <algorithm>

#include "dvrouter.h"

#define IP_IN_INTERNET "8.8.8.8"
#define PORT_OF_IP_IN_INTERNET  53

// class implementation
DVRouter::DVRouter(std::string topology)
{
    // internal things
    write_here = 0;

    // read topology file and update variables
    
    // all this need to be done before start
    update_localip();
    do_bind();
}

DVRouter::~DVRouter()
{
}

void DVRouter::update_localip(void)
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

    my_ip = local_address.sin_addr;

    close(sockfd);
}

void DVRouter::do_bind(void)
{
    if((main_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_UDP)) < 0)
    {
        printf("socket() failed\n");
        exit(-1);
    }

    //Construct local address structure
    struct sockaddr_in local_address;                           // Local address
    memset(&local_address, 0 , sizeof(local_address));          // Zero out structure
    local_address.sin_family = AF_INET;                         // IPv4 address family
    local_address.sin_addr.s_addr = htonl(INADDR_ANY);          // Any incoming interface
    local_address.sin_port = htons(my_port);                    // Local port

    // Bind to the local address
    if(bind(main_fd, (struct sockaddr*) &local_address, sizeof(local_address)) < 0)
    {
        printf("bind() failed");
        exit(-1);
    }
}

void DVRouter::process_command(std::string args[])
{
    std::cout << "process command " << args[0] << std::endl;
}

void DVRouter::start(void)
{
    int nbytes, max_fd;
    std::string args[CMD_MAX_ARGS];
    char space[] = " ";

    fd_set read_fds;
    fd_set temp_read_fds;

    FD_SET(STDIN_FILENO, &read_fds);
    FD_SET(main_fd, &read_fds);

    max_fd = std::max(STDIN_FILENO, main_fd);

    while(1)
    {
        temp_read_fds = read_fds;

        if (select(max_fd + 1, &temp_read_fds, NULL, NULL, NULL) < 0)
            break;

        if (FD_ISSET(main_fd, &temp_read_fds))
        {
            // recv packet and process
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
                for (int i = 0; i < CMD_MAX_ARGS; i++)
                {
                    if (token == NULL)
                        break;

                    args[i].assign(token);
                    token = strtok(NULL, space);
                }

                std::transform(args[0].begin(), args[0].end(), args[0].begin(), tolower);
                process_command(args);

                for (int i = 0; i < CMD_MAX_ARGS; i++)
                    args[i].clear();
            }
        }
    }
}
