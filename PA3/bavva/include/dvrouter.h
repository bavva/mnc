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

class DVTimer
{
public:
    unsigned node_id;   // ID of node corresponding to this timer
    time_t fire_at;     // time at which the timer should fire

    DVTimer(unsigned node_id, time_t fire_at):node_id(node_id), fire_at(fire_at){};
    ~DVTimer(){};
};

class DVNode
{
public:
    struct in_addr node_ip;     // ip address of the node
    unsigned short node_port;   // port of the node
    unsigned short node_cost;   // cost from us to this node
    unsigned node_id;           // ID of the node
    bool is_neighbor;           // whether this node is our neighbor

    DVNode(struct in_addr node_ip, unsigned short node_port, unsigned short node_cost, 
            unsigned node_id, bool is_neighbor):node_ip(node_ip), node_port(node_port), 
            node_cost(node_cost), node_id(node_id), is_neighbor(is_neighbor){};
    ~DVNode(){};
};

class DVRouter
{
private:
    // main configuration
    unsigned num_servers;           // total number of servers
    unsigned num_neighbors;         // number of neighbors to current node
    struct in_addr my_ip;           // ip address of current node
    unsigned short my_port;         // port number of current node
    unsigned my_id;                 // ID of current node
    time_t router_timeout;          // after this timeout, routers send routes

    // other peers and timers
    std::map<unsigned, DVNode*> allnodes;   // pointers to all router nodes
    std::map<unsigned, DVNode*> neighbors;  // pointers to all neighbors
    std::list<DVTimer*> timers;             // list of timers in increasing order

    // routing information
    unsigned short **routing_costs; // 2-D array of costs. routing_costs[x][y] means cost to send packet from x to y

    // fd stuff
    int main_fd;                    // the main fd
    fd_set read_fds;                // monitor these for reading

    // internal things
    char command_buffer[COMMAND_BUFFER];    // buffer to store command
    int write_here;                         // where to write in buffer

public:
    DVRouter(std::string topology, time_t router_timeout);
    ~DVRouter();

    // basic functions
    void start(void);

private:
    // basic functions
    void do_bind(void);
    void initialize(std::string topology);
    void update_localip(void);
    void process_command(std::string args[]);

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
