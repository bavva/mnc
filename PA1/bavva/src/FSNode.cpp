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
FSNode::FSNode(int port, bool is_server):port(port), is_server(is_server), stats_ready(false)
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
    {
        delete connections.front();
        connections.pop_front();
    }
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

    for (std::list<int>::iterator it = read_sockets.begin(); it != read_sockets.end(); ++it)
    {
        if (*it > max)
            max = *it;
    }

    for (std::list<int>::iterator it = write_sockets.begin(); it != write_sockets.end(); ++it)
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
    for (std::list<int>::iterator it = read_sockets.begin(); it != read_sockets.end(); ++it)
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
    for (std::list<int>::iterator it = write_sockets.begin(); it != write_sockets.end(); ++it)
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

void FSNode::add_serverip(ServerIP *sip)
{
    for (std::list<ServerIP *>::iterator it = server_ip_list.begin(); it != server_ip_list.end(); ++it)
    {
        if (sip->server_ip == (*it)->server_ip)
        {
            delete sip;
            return;
        }
    }

    server_ip_list.push_back(sip);
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

void FSNode::process_register_request(FSHeader *header)
{
    std::cout << "A request to REGISTER with us(client) is rejected\n";
}

void FSNode::process_stats_request(FSHeader *header)
{
    std::cout << "Only a client can process STATS request\n";
}

void FSNode::process_stats_response(FSHeader *header)
{
    std::cout << "Only a server can process STATS response\n";
}

void FSNode::process_register_response(FSHeader *header)
{
    std::cout << "SERVER_IP_LIST message is applicable to client only\n";
}

void FSNode::process_next_stat_request(void)
{
    std::cout << "message is not applicable to client\n";
}

void FSNode::set_bcast_serverip_list_flag(void)
{
    bcast_serverip_list_flag = true;
}

std::string FSNode::get_localip(void)
{
    return local_ip;
}

void FSNode::print_server_ip_list(void)
{
    int i = 1;

    printf ("%-5s%-35s%-20s%-8s\n", "ID", "Hostname", "IP Address", "Port No");
    for (std::list<ServerIP*>::iterator it = server_ip_list.begin(); it != server_ip_list.end(); ++it)
    {
        printf ("%-5d%-35s%-20s%-8d\n", i, (*it)->server_name.c_str(), (*it)->server_ip.c_str(), (*it)->port);
        i++;
    }
}

void FSNode::print_all_connections(void)
{
    int i = 1;

    printf ("%-5s%-35s%-20s%-8s\n", "ID", "Hostname", "IP Address", "Port No");
    for (std::list<FSConnection*>::iterator it = connections.begin(); it != connections.end(); ++it)
    {
        printf ("%-5d%-35s%-20s%-8d\n", i++, (*it)->peer_name.c_str(), (*it)->peer_ip.c_str(), (*it)->peer_port);
    }
}

void FSNode::bcast_serverip_list(void)
{
    int nchars;
    char buffer[METADATA_SIZE];
    char *writer = NULL;

    // clean the buffer
    writer = buffer;
    memset(writer, 0, METADATA_SIZE);

    for (std::list<ServerIP*>::iterator it = server_ip_list.begin(); it != server_ip_list.end(); ++it)
    {
        strncpy(writer, (*it)->server_name.c_str(), (*it)->server_name.length());
        writer += (*it)->server_name.length();
        strncpy(writer, ",", 1);
        writer += 1;

        strncpy(writer, (*it)->server_ip.c_str(), (*it)->server_ip.length());
        writer += (*it)->server_ip.length();
        strncpy(writer, ",", 1);
        writer += 1;

        nchars = sprintf(writer, "%d", (*it)->port);
        writer += nchars;
        strncpy(writer, ",", 1);
        writer += 1;
    }

    for (std::list<FSConnection*>::iterator it = connections.begin(); it != connections.end(); ++it)
    {
        (*it)->send_message(MSG_TYPE_REGISTER_RESPONSE, server_ip_list.size(), buffer);
    }
}

void FSNode::start(void)
{
    int nbytes;
    std::string args[7];
    char space[] = " ";

    fd_set temp_read_fds;
    fd_set temp_write_fds;

    insert_readfd(STDIN_FILENO);
    insert_readfd(listen_fd);

    while(1)
    {
        // clear broken links
        for (std::list<FSConnection*>::iterator it = connections.begin(); it != connections.end();)
        {
            if ((*it)->is_broken())
            {
                if (is_server)
                {
                    bcast_serverip_list_flag = true;
                    for (std::list<ServerIP*>::iterator ite = server_ip_list.begin(); ite != server_ip_list.end(); ++ite)
                    {
                        if ((*it)->peer_ip == (*ite)->server_ip)
                        {
                            delete (*ite);
                            server_ip_list.erase(ite);
                            break;
                        }
                    }
                }

                printf ("Connection with %s is terminated\n", (*it)->peer_ip.c_str());
                delete (*it);
                it = connections.erase(it);
            }
            else
            {
                ++it;
            }
        }

        if (request_exit)
            break; // break from main loop and let all destructors do their job

        if (stats_ready == true)
        {
            process_next_stat_request();
        }

        if (bcast_serverip_list_flag)
        {
            bcast_serverip_list();
            bcast_serverip_list_flag = false;
        }

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
                process_newconnection(new FSConnection(false, peer_name, ntohs(peer_address.sin_port), this, peer_fd));
            }
        }

        for (std::list<FSConnection*>::iterator it = connections.begin(); it != connections.end(); ++it)
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
                for (int i = 0; i < 7; i++)
                {
                    if (token == NULL)
                        break;

                    args[i].assign(token);
                    token = strtok(NULL, space);
                }

                std::transform(args[0].begin(), args[0].end(), args[0].begin(), tolower);
                process_command(args);

                for (int i = 0; i < 7; i++)
                    args[i].clear();
            }
        }
    }
}