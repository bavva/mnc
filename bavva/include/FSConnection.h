#ifndef _FSCONNECTION_H_
#define _FSCONNECTION_H_

// includes
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "global.h"

class FSNode;

typedef enum
{
    CS_WAITINGTO_READ,
    CS_READING_HEADER,
    CS_READING_DATA,
    CS_WAITINGTO_WRITE,
    CS_WRITING_HEADER,
    CS_WRITING_DATA,
    CS_ERROR
}ConnState;

class FSConnection
{
    public:
    bool with_server;                   // is this connection with server

    std::string peer_ip;                // ip address of the peer
    int peer_port;                      // port of the peer

    int sock_fd;                        // socket for this connection
    bool is_reading;                    // currently are we reading or writing to socket
    FSNode *fsnode;                     // our FSServer or FSClient object
    FSHeader header;                    // buffer to send or recreate headers

    int header_bytesleft;               // bytes left to read/write header
    int body_bytesleft;                 // bytes left to read/write body

    ConnState state;                    // to run the state machine
    bool link_broken;                   // if this flas is set, this connection will be closed soon

    FSConnection(bool with_server, std::string ip, int port, FSNode *fsnode);
    FSConnection(bool with_server, std::string ip, int port, FSNode *fsnode, int fd);
    ~FSConnection();

    bool is_broken(void);
    void start_reading(void);
    void start_writing(void);

    void on_ready_toread(void);
    void on_ready_towrite(void);

    void send_message(FSMessageType msg_type, int content_length, char *buffer);
};
#endif /* _FSCONNECTION_H_ */
