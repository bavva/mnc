#ifndef _FSCONNECTION_H_
#define _FSCONNECTION_H_

// includes
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "global.h"

class FSConnection
{
    public:
    bool with_server;                   // is this connection with server

    std::string peer_ip;                // ip address of the peer
    int peer_port;                      // port of the peer

    int sock_fd;                        // socket for this connection

    FSConnection(bool with_server, std::string ip, int port);
    FSConnection(bool with_server, std::string ip, int port, int fd);
    ~FSConnection();
};
#endif /* _FSCONNECTION_H_ */
