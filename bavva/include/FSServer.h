#ifndef _FSSERVER_H_
#define _FSSERVER_H_
// includes
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <vector>

#include "global.h"
#include "FSNode.h"

class FSServer : public FSNode
{
    public:
    FSServer(int port);
    ~FSServer();

    // private functions
    private:
    void process_command(std::string& command);
};
#endif /* _FSSERVER_H_ */
