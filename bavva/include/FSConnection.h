#ifndef _FSCONNECTION_H_
#define _FSCONNECTION_H_

// includes
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "global.h"

class FSNode;

class FSConnection
{
    public:
    bool with_server;                   // is this connection with server

    std::string peer_ip;                // ip address of the peer
    int peer_port;                      // port of the peer

    int sock_fd;                        // socket for this connection

    bool is_reading;                    // currently are we reading or writing to socket

    FSNode *fsnode;                     // our FSServer or FSClient object

    FSConnection(bool with_server, std::string ip, int port, FSNode *fsnode);
    FSConnection(bool with_server, std::string ip, int port, int fd, FSNode *fsnode);
    ~FSConnection();
};
#endif /* _FSCONNECTION_H_ */
