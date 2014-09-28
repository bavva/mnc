#ifndef _FSNODE_H_
#define _FSNODE_H_

// includes
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <list>

#include "FSConnection.h"
#include "global.h"

class FSNode
{
    protected:
    int port;                           // port on which we are listening
    std::string local_ip;               // IP address of current process

    std::list<int> read_sockets;        // copy of read_fds
    std::list<int> write_sockets;       // copy of write_fds
    fd_set read_fds;                    // monitor these for reading
    fd_set write_fds;                   // monitor these for writing

    bool is_server;                     // is this node server

    int max_fd;                         // maximum of all fds
    int listen_fd;                      // fd on which we accept connections

    char command_buffer[COMMAND_BUFFER];    // buffer to keep command data until completely entered
    int write_here;                     // write from command_buffer + write_here

    std::list<FSConnection*> connections;      // all established connections
    std::list<ServerIP *> server_ip_list;   // server ip list

    bool bcast_serverip_list_flag;  // flag to indicate we need to broadcast server ip list

    public:
    FSNode(int port, bool is_server);
    ~FSNode();

    // functions
    virtual void process_register_request(FSHeader *header);
    virtual void process_register_response(FSHeader *header);
    virtual void process_command(std::string args[]);
    virtual void process_newconnection(FSConnection *connection);

    void start(void);
    void do_bind(void);
    void bcast_serverip_list(void);
    void update_localip(void);
    void update_maxfd(void);
    void insert_readfd(int fd);
    void remove_readfd(int fd);
    void insert_writefd(int fd);
    void remove_writefd(int fd);
    void add_serverip(ServerIP *sip);
    void set_bcast_serverip_list_flag(void);
    void print_server_ip_list(void);
    void print_all_connections(void);
};
#endif /* _FSNODE_H_ */
