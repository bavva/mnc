#ifndef _FSSERVER_H_
#define _FSSERVER_H_
// includes
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <list>

#include "global.h"
#include "FSNode.h"

class FSServer : public FSNode
{
    private:
    std::list<ServerIP> serverIPList;

    public:
    FSServer(int port);
    ~FSServer();

    // private functions
    private:
    void process_command(std::string args[]);
    void process_newconnection(FSConnection *connection);
    void bcast_serverip_list(void);
};
#endif /* _FSSERVER_H_ */
