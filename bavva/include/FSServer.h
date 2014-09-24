#ifndef _FSSERVER_H_
#define _FSSERVER_H_
// includes
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <vector>
#include "global.h"

class FSServer
{
    private:
    int port;                           // port on which we are listening

    std::vector<int> read_sockets;      // copy of read_fds
    std::vector<int> write_sockets;     // copy of write_fds
    fd_set read_fds;                    // monitor these for reading
    fd_set write_fds;                   // monitor these for writing

    int max_fd;                         // maximum of all fds
    int listen_fd;                      // fd on which we accept connections

    char command_buffer[COMMAND_BUFFER];    // buffer to keep command data until completely entered
    int write_here;                     // write from command_buffer + write_here

    public:
    FSServer(int port);
    ~FSServer();

    // functions
    void process_command(std::string& command);
    void start(void);
    void update_maxfd(void);
    void insert_readfd(int fd);
    void remove_readfd(int fd);
    void insert_writefd(int fd);
    void remove_writefd(int fd);

    // private functions
    private:
    void do_bind(void);
};
#endif /* _FSSERVER_H_ */
