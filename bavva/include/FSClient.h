#ifndef _FSCLINET_H_
#define _FSCLINET_H_
// includes
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <vector>

#include "global.h"
#include "FSNode.h"

class FSClient : public FSNode
{
    private:
    std::string server_ipaddress;

    public:
    FSClient(int port);
    ~FSClient();

    // private functions
    private:
    void register_self(std::string server_ip, int server_port);
    void process_command(std::string args[]);
    void process_newconnection(FSConnection *connection);
    void process_register_response(FSHeader *header);
    void make_connection(std::string peer_ip, int peer_port);
    void terminate_connection(int connection_id);
    void terminate_allconnections(void);
};
#endif /* _FSCLINET_H_ */
