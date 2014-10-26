#ifndef _FSCONNECTION_H_
#define _FSCONNECTION_H_

// includes
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

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

// generic functions
void hostname_to_ip(std::string hostname, std::string &ipaddress);
void ip_to_hostname(std::string ipaddress, std::string &hostname);

// class definition
class FSConnection
{
    public:
    bool with_server;                   // is this connection with server

    std::string peer_ip;                // ip address of the peer
    std::string peer_name;              // hostname of the peer
    int peer_port;                      // port of the peer

    int sock_fd;                        // socket for this connection
    bool is_reading;                    // currently are we reading or writing to socket
    FSNode *fsnode;                     // our FSServer or FSClient object
    FSHeader header;                    // buffer to send or recreate headers
    char *header_ptr;                   // to remember until which point header is written

    int header_bytesleft;               // bytes left to read/write header
    int body_bytesleft;                 // bytes left to read/write body

    ConnState state;                    // to run the state machine
    bool link_broken;                   // if this flas is set, this connection will be closed soon

    FILE *fp;                           // file handle to process upload and download
    char packet_buffer[PACKET_BUFFER];  // buffer used to send and receive file chunks

    // statistics
    int total_downloads;                // from remote peer to us
    int total_uploads;                  // from us to remote peer
    unsigned long total_upload_data;    // total data we uploaded
    unsigned long total_download_data;  // total data we downloaded
    unsigned long total_upload_time;    // time taken in micro seconds for all upload data
    unsigned long total_download_time;  // time taken in micro seconds for all download data

    unsigned long current_file_size;    // size of current file we are dealing with
    unsigned long current_file_time;    // duration we spent in micro seconds on sending or receiving

    FSConnection(bool with_server, std::string hostname, int port, FSNode *fsnode);
    FSConnection(bool with_server, std::string ip, int port, FSNode *fsnode, int fd);
    ~FSConnection();

    bool is_broken(void);
    void start_reading(void);
    void start_writing(void);

    void on_ready_toread(void);
    void on_ready_towrite(void);

    void send_message(FSMessageType msg_type, int content_length, char *buffer);
    void process_received_message(void);
};
#endif /* _FSCONNECTION_H_ */