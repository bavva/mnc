#ifndef _FSNODE_H_
#define _FSNODE_H_

// includes
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <vector>
#include "global.h"

class FSNode
{
    protected:
    int port;                           // port on which we are listening

    std::vector<int> read_sockets;      // copy of read_fds
    std::vector<int> write_sockets;     // copy of write_fds
    fd_set read_fds;                    // monitor these for reading
    fd_set write_fds;                   // monitor these for writing

    int max_fd;                         // maximum of all fds
    int listen_fd;                      // fd on which we accept connections

    char command_buffer[COMMAND_BUFFER];    // buffer to keep command data until completely entered
    int write_here;                     // write from command_buffer + write_here

    std::string local_ip;               // IP address of current process

    public:
    FSNode(int port);
    ~FSNode();

    // functions
    void start(void);
    void do_bind(void);
    virtual void process_command(std::string args[]);
    void update_localip(void);
    void update_maxfd(void);
    void insert_readfd(int fd);
    void remove_readfd(int fd);
    void insert_writefd(int fd);
    void remove_writefd(int fd);
};
#endif /* _FSNODE_H_ */
