#ifndef _DVROUTER_H_
#define _DVROUTER_H_

// includes
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <list>
#include <map>

#include "global.h"
#include "logger.h"
#include "dvtimer.h"
#include "dvnode.h"

class DVRouter
{
private:
    // main configuration
    unsigned num_servers;           // total number of servers
    unsigned num_neighbors;         // number of neighbors to current node
    struct in_addr my_ip;           // ip address of current node
    unsigned short my_port;         // port number of current node
    unsigned my_id;                 // ID of current node

    // other peers and timers
    std::map<unsigned, DVNode*> allnodes;   // pointers to all router nodes
    std::map<unsigned, DVNode*> neighbors;  // pointers to all neighbors
    std::list<DVTimer*> timers;             // list of timers in increasing order

    // routing information
    unsigned short **routing_costs; // 2-D array of costs. routing_costs[x][y] means cost to send packet from x to y

    // fd stuff
    int max_fd;                     // maximum of all fds
    int main_fd;                    // the main fd
    fd_set read_fds;                // monitor these for reading

public:
    DVRouter(std::string topology);
    ~DVRouter();

    // basic functions
    void start(void);
    void do_bind(void);

    // DVRouter functions
    void update(unsigned server_id1, unsigned server_id2, unsigned short cost);
    void step(void);
    void packets(void);
    void display(void);
    void disable(unsigned server_id);
    void crash(void);
    void dump(void);
    void academic_integrity(void);
};
#endif /* _DVROUTER_H_ */
