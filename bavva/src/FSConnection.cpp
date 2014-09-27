#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <unistd.h>
#include <cstring>
#include <assert.h>

#include "../include/FSConnection.h"
#include "../include/FSNode.h"

// class implementation
FSConnection::FSConnection(bool with_server, std::string hostname, int port, FSNode *fsnode):with_server(with_server), peer_port(port), fsnode(fsnode)
{

    link_broken = true;
    sock_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(sock_fd < 0)
        return;

    hostname_to_ip(hostname, peer_ip);

    struct sockaddr_in peer_address;                    // peer address
    memset(&peer_address, 0, sizeof(peer_address));     // zero out the structure
    peer_address.sin_family = AF_INET;                  // IPv4 address family
    peer_address.sin_port = htons(port);                // peer port

    int ret = inet_pton(AF_INET, peer_ip.c_str(), &peer_address.sin_addr.s_addr);
    if(ret <= 0)
        return;

    if(connect(sock_fd, (struct sockaddr *)&peer_address, sizeof(peer_address)) < 0)
    {
        std::cout << "Failed to connect to " << peer_ip << " and port " << port << std::endl;
        return;
    }

    fsnode->insert_readfd(sock_fd);

    state = CS_WAITINGTO_READ;
    link_broken = false;
    is_reading = true;
}

FSConnection::FSConnection(bool with_server, std::string ip, int port, FSNode *fsnode, int fd):with_server(with_server), peer_ip(ip), peer_port(port), sock_fd(fd), fsnode(fsnode)
{
    assert(fd >= 0);
    assert(fsnode != NULL);

    fsnode->insert_readfd(sock_fd);

    state = CS_WAITINGTO_READ;
    link_broken = false;
    is_reading = true;
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

bool FSConnection::is_broken(void)
{
    return link_broken;
}

void hostname_to_ip(std::string hostname, std::string &ipaddress)
{
    struct addrinfo hints, *res, *p;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(hostname.c_str(), NULL, &hints, &res) != 0)
        return;

    for(p = res; p != NULL; p = p->ai_next)
    {
        if (p->ai_family == AF_INET)
        {
            struct sockaddr_in *sin = (struct sockaddr_in *)p->ai_addr;
            ipaddress.assign(inet_ntoa(sin->sin_addr));
        }
    }

    freeaddrinfo(res);
}

void FSConnection::send_message(FSMessageType msg_type, int content_length, char *buffer)
{
    if (link_broken)
    {
        printf("Can't send. This link is broken\n");
    }

    if (state != CS_WAITINGTO_READ)
    {
        printf("One message at a time. Try later\n");
        return;
    }

    // copy to local buffer
    header.message_type = msg_type;
    header.content_length = content_length;
    memcpy(header.metadata, buffer, METADATA_SIZE);

    // start state machine and return to select
    state = CS_WAITINGTO_WRITE;
    start_writing();
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

void FSConnection::process_received_message(void)
{
    switch(header.message_type)
    {
        case MSG_TYPE_REGISTER_REQUEST:
            fsnode->process_register_request(&header);
            break;
        case MSG_TYPE_REGISTER_RESPONSE:
            fsnode->process_register_response(&header);
            break;
        default:
            break;
    }
}

void FSConnection::on_ready_toread(void)
{
    int nbytes;

    if (state == CS_WAITINGTO_READ)
    {
        header_bytesleft = sizeof(header);
        state = CS_READING_HEADER;
    }

    if (state == CS_READING_HEADER)
    {
        nbytes = recv(sock_fd, &header, header_bytesleft, 0);
        if (nbytes <= 0)
        {
            link_broken = true;
            return;
        }
        header_bytesleft -= nbytes;

        if (header_bytesleft == 0)
        {
            // process the received message
            process_received_message();

            // these are messages with out bodies
            if (header.message_type == MSG_TYPE_REGISTER_REQUEST || header.message_type == MSG_TYPE_REGISTER_RESPONSE)
            {
                state = CS_WAITINGTO_READ;
                start_reading();
            }
            else
            {
                // also set body size etc based on header
                state = CS_READING_DATA;
            }
        }
    }

    if (state == CS_READING_DATA)
    {
        // read body and reduce remaining amount
    }
}

void FSConnection::on_ready_towrite(void)
{
    int nbytes;

    if (state == CS_WAITINGTO_WRITE)
    {
        header_bytesleft = sizeof(header);
        state = CS_WRITING_HEADER;
    }

    if (state == CS_WRITING_HEADER)
    {
        nbytes = send(sock_fd, &header, header_bytesleft, 0);
        if (nbytes <= 0)
        {
            link_broken = true;
            return;
        }
        header_bytesleft -= nbytes;

        if (header_bytesleft == 0)
        {
            // these are messages with out bodies
            if (header.message_type == MSG_TYPE_REGISTER_REQUEST || header.message_type == MSG_TYPE_REGISTER_RESPONSE)
            {
                state = CS_WAITINGTO_READ;
                start_reading();
            }
            else
            {
                // also set body size etc based on header
                state = CS_WRITING_DATA;
            }
        }
    }

    if (state == CS_WRITING_DATA)
    {
        // write body and reduce reamining amount
    }
}
