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
    public:
    FSClient(int port);
    ~FSClient();

    // private functions
    private:
    void register_self(std::string server_ip, int server_port);
    void process_command(std::string args[]);
};
#endif /* _FSCLINET_H_ */
