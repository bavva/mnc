#ifndef _DVROUTER_H_
#define _DVROUTER_H_

// includes
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdint.h>
#include <list>
#include <map>

#include "global.h"
#include "logger.h"

typedef enum
{
    LINK_NORMAL,
    LINK_TIMEDOUT,
    LINK_DISABLED
}LinkState;

class DVTimer
{
public:
    int16_t node_id;        // ID of node corresponding to this timer
    time_t fire_at;         // time at which the timer should fire

    DVTimer(int16_t node_id, time_t fire_at):node_id(node_id), fire_at(fire_at){};
    ~DVTimer(){};
};

class DVNode
{
public:
    struct in_addr node_ip;     // ip address of the node
    uint16_t node_port;         // port of the node
    uint16_t link_cost;         // cost from us to this node
    int16_t node_id;            // ID of the node
    int16_t route_thru;         // next hop id for this node
    bool is_neighbor;           // whether this node is our neighbor
    int idle_count;             // count of how many heart beats it missed
    LinkState link_state;       // state of link

    DVNode(struct in_addr node_ip, uint16_t node_port, uint16_t link_cost, 
            int16_t node_id, int16_t route_thru, bool is_neighbor):node_ip(node_ip), node_port(node_port), 
            link_cost(link_cost), node_id(node_id), route_thru(route_thru), is_neighbor(is_neighbor),
            idle_count(0), link_state(LINK_NORMAL){};
    ~DVNode(){};

    // functions
    uint16_t get_linkcost(void)
    {
        if (link_state == LINK_NORMAL)
            return link_cost;

        return INFINITE_COST;
    }
};

class DVRouter
{
private:
    // main configuration
    uint16_t num_servers;     // total number of servers
    uint16_t num_neighbors;   // number of neighbors to current node
    struct in_addr my_ip;     // ip address of current node
    uint16_t my_port;         // port number of current node
    int16_t my_id;            // ID of current node
    time_t router_timeout;    // after this timeout, routers send routes
    unsigned pckts_recvd;     // number of packets received since last reset

    // other peers and timers
    std::map<int16_t, DVNode*> allnodes;    // pointers to all router nodes
    std::map<int16_t, DVNode*> neighbors;   // pointers to all neighbors
    std::list<DVTimer*> timer_list;         // list of timers in increasing order
    std::map<int16_t, std::list<DVTimer*>::iterator> timer_map;     // map of all timers running
    /*
     * Use timer_list to insert timers and top timers in order
     * Use timer_map when you want to delete a timer randomly.
     * Get iterator from map and delete that in list
     */

    // routing information
    uint16_t **routing_costs; // 2-D array of costs. routing_costs[x][y] means cost to send packet from x to y

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
    void start_timer(int16_t id);
    void remove_timer(int16_t id);
    void on_fire(int16_t id);
    time_t process_timers(void);


    // DVRouter functions
    void update(int16_t id1, int16_t id2, uint16_t cost, int16_t via);
    bool update_linkcost(int16_t id1, int16_t id2, uint16_t cost);
    bool update_linkstate(int16_t id1, int16_t id2, LinkState state);
    void check_alternate_routes(int16_t id);
};
#endif /* _DVROUTER_H_ */
