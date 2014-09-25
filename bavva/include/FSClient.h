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
    void process_command(std::string& command);
};
#endif /* _FSCLINET_H_ */
