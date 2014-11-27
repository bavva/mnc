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
    short node_id;      // ID of node corresponding to this timer
    time_t fire_at;     // time at which the timer should fire

    DVTimer(short node_id, time_t fire_at):node_id(node_id), fire_at(fire_at){};
    ~DVTimer(){};
};

class DVNode
{
public:
    struct in_addr node_ip;     // ip address of the node
    unsigned short node_port;   // port of the node
    unsigned short link_cost;   // cost from us to this node
    short node_id;              // ID of the node
    short route_thru;           // next hop id for this node
    bool is_neighbor;           // whether this node is our neighbor
    int idle_count;             // count of how many heart beats it missed

    DVNode(struct in_addr node_ip, unsigned short node_port, unsigned short link_cost, 
            short node_id, short route_thru, bool is_neighbor):node_ip(node_ip), node_port(node_port), 
            link_cost(link_cost), node_id(node_id), route_thru(route_thru), is_neighbor(is_neighbor), idle_count(0){};
    ~DVNode(){};
};

class DVRouter
{
private:
    // main configuration
    unsigned short num_servers;     // total number of servers
    unsigned short num_neighbors;   // number of neighbors to current node
    struct in_addr my_ip;           // ip address of current node
    unsigned short my_port;         // port number of current node
    short my_id;                    // ID of current node
    time_t router_timeout;          // after this timeout, routers send routes
    unsigned pckts_recvd;           // number of packets received since last reset

    // other peers and timers
    std::map<short, DVNode*> allnodes;   // pointers to all router nodes
    std::map<short, DVNode*> neighbors;  // pointers to all neighbors
    std::list<DVTimer*> timer_list;    // list of timers in increasing order
    std::map<short, std::list<DVTimer*>::iterator> timer_map;     // map of all timers running
    /*
     * Use timer_list to insert timers and top timers in order
     * Use timer_map when you want to delete a timer randomly.
     * Get iterator from map and delete that in list
     */

    // routing information
    unsigned short **routing_costs; // 2-D array of costs. routing_costs[x][y] means cost to send packet from x to y

    // fd stuff
    int main_fd;                    // the main fd
    fd_set read_fds;                // monitor these for reading

    // internal things
    char command_buffer[COMMAND_BUFFER];    // buffer to store command
    int write_here;                         // where to write in buffer
    char *packet_buffer;                    // buffer to frame packet
    int packet_buffer_size;                 // size of packet_buffer
    std::string error_msg;                  // error message

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
    bool process_command(std::string args[]);
    void frame_bcast_packet(void);
    void broadcast_costs(void);
    void process_recvd_packet(void);

    // timer functions
    void start_timer(short id);
    void remove_timer(short id);
    void on_fire(short id);
    time_t process_timers(void);


    // DVRouter functions
    void update(short id1, short id2, unsigned short cost, short via);
    bool update_linkcost(short id1, short id2, unsigned short cost);
    void check_alternate_routes(short id);
};
#endif /* _DVROUTER_H_ */
