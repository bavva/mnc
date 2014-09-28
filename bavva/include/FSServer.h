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
    std::list<StatisticsEntry*> stats;
    int stat_wait_count;

    public:
    FSServer(int port);
    ~FSServer();

    // private functions
    private:
    void process_command(std::string args[]);
    void process_newconnection(FSConnection *connection);
    void process_register_request(FSHeader *header);
    void fetch_stats(void);
    void print_stats(void);
    void send_fetch_stat_request(std::string host1, std::string host2);
};
#endif /* _FSSERVER_H_ */
