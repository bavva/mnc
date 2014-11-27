#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <unistd.h>
#include <cstring>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <assert.h>

#include "../include/dvrouter.h"

#define IP_IN_INTERNET "8.8.8.8"
#define PORT_OF_IP_IN_INTERNET  53

// helper function
uint16_t cost_sum(uint16_t cost1, uint16_t cost2)
{
    if (cost1 == INFINITE_COST || cost2 == INFINITE_COST)
        return INFINITE_COST;

    return cost1 + cost2;
}

// class implementation
DVRouter::DVRouter(std::string topology, time_t router_timeout)
{
    // internal things
    my_id = -1;
    my_port = 0;
    main_fd = 0;
    write_here = 0;
    pckts_recvd = 0;
    this->router_timeout = router_timeout;
    update_localip(); // has to be done before initialize

    // read topology file and update variables
    initialize(topology);
    
    // all this need to be done before start
    do_bind();
}

DVRouter::~DVRouter()
{
    // free routing table
    for (int i = 0; i < num_servers; i++)
        delete routing_costs[i];
    delete routing_costs;
    routing_costs = NULL;

    // free all nodes
    for (std::map<int16_t, DVNode*>::iterator it = allnodes.begin(); it != allnodes.end(); it++)
        delete it->second;
    allnodes.clear();
    neighbors.clear();

    // free all timers
    while (!timer_list.empty())
    {
        delete timer_list.front();
        timer_list.pop_front();
    }

    timer_list.clear();
    timer_map.clear();

    // free packet buffer
    delete packet_buffer;
    packet_buffer = NULL;

    // close main fd
    if (main_fd > 0)
    {
        close(main_fd);
        main_fd = 0;
    }
}

void DVRouter::initialize(std::string topology)
{
    std::string line;
    std::ifstream input(topology.c_str());

    // to create DVNodes(servers)
    std::string ipaddressstr;
    uint16_t port;
    int16_t id;

    // for ipaddress stff
    struct in_addr ipaddr;

    // for neighbors cost
    int16_t id1, id2;
    uint16_t cost;

    // read number of servers
    {
    std::getline(input, line);
    std::istringstream iss(line);
    iss >> num_servers;
    assert(num_servers > 0);
    }

    // read number of neighbors
    {
    std::getline(input, line);
    std::istringstream iss(line);
    iss >> num_neighbors;
    assert(num_neighbors > 0 && num_neighbors <= num_servers);
    }

    // create routing table
    routing_costs = new uint16_t*[num_servers]; // allocate memory
    for (int i = 0; i < num_servers; i++)
        routing_costs[i] = new uint16_t[num_servers];

    // fill routing table
    for (int i = 0; i < num_servers; i++)
    {
        for (int j = 0; j < num_servers; j++)
        {
            if (i == j)
                routing_costs[i][j] = 0;
            else
                routing_costs[i][j] = INFINITE_COST;
        }
    }

    // create all DVNodes(servers)
    for (int i = 0; i < num_servers; i++)
    {
        std::getline(input, line);
        std::istringstream iss(line);

        iss >> id;
        iss >> ipaddressstr;
        iss >> port;

        inet_aton(ipaddressstr.c_str(), &ipaddr);

        // if our ip, update our port and our id
        if (my_ip.s_addr == ipaddr.s_addr)
        {
            my_port = port;
            my_id = id;
        }
        else
        {
            DVNode *node = new DVNode(ipaddr, port, INFINITE_COST, id, -1, false);
            allnodes[id] = node;
        }
    }

    // we should be having our id and port correctly updated by now
    assert (my_port> 0 && my_id > 0);

    // read the costs of neighbors and update
    while (std::getline(input, line))
    {
        std::istringstream iss(line);

        iss >> id1;
        iss >> id2;
        iss >> cost;

        // one id should be ours because this is neighbor
        if (id1 != my_id && id2 != my_id)
            continue;

        id = (id1 == my_id) ? id2 : id1;
        allnodes[id]->link_cost = cost;
        allnodes[id]->is_neighbor = true;
        neighbors[id] = allnodes[id];

        // update cost in routing table too
        routing_costs[my_id - 1][id - 1] = cost;
        routing_costs[id - 1][my_id - 1] = cost;
        allnodes[id]->route_thru = id;
    }

    // allocate buffer to frame packet
    packet_buffer_size = 8 + num_servers * 12;
    packet_buffer = new char[packet_buffer_size];
}

void DVRouter::start_timer(int16_t id)
{
    DVTimer *new_timer = NULL;
    std::list<DVTimer*>::iterator it;

    // don't start duplicate timer
    if (timer_map.find(id) != timer_map.end())
    {
        printf ("WARNING: Timer for %d is already running\n", id);
        return;
    }

    // create timer node
    new_timer = new DVTimer(id, time(NULL) + router_timeout);

    // insert in map and list
    timer_list.push_back(new_timer);
    it = timer_list.end();
    timer_map[id] = --it;
}

void DVRouter::remove_timer(int16_t id)
{
    DVTimer *timer = NULL;
    std::list<DVTimer*>::iterator it;

    // return if timer doen't exist
    if (timer_map.find(id) == timer_map.end())
    {
        //printf ("WARNING: Timer for %d is not running\n", id);
        return;
    }

    // get timer and iterator of timer in the list
    it = timer_map[id];
    timer = *it;

    // delete timer in list and map
    timer_list.erase(it);
    timer_map.erase(id);

    // free timer
    delete timer;
}

void DVRouter::on_fire(int16_t id)
{
    // if my_id, broadcast costs
    if (id == my_id)
    {
        broadcast_costs();
        start_timer(my_id);
        return;
    }

    if (neighbors.find(id) == neighbors.end()) // not neighbor?
    {
        printf ("WARNING: some how a timer was started for non neighbor %d!!\n", id);
        return;
    }

    // increment idle counter
    neighbors[id]->idle_count++;

    // missed 3 beats, set cost to infinity and reset idle count
    if (neighbors[id]->idle_count >= 3)
    {
        if (neighbors[id]->route_thru == id)
            update_linkstate(my_id, id, LINK_TIMEDOUT);

        return;
    }

    // restart timer for this node
    start_timer(id);
}

time_t DVRouter::process_timers(void)
{
    time_t current_time = time(NULL);
    DVTimer *timer = NULL;
    int16_t id;

    while (!timer_list.empty())
    {
        timer = timer_list.front();
        id = timer->node_id;

        if (timer->fire_at > current_time)
            return timer->fire_at - current_time;

        remove_timer(id);
        on_fire(id);
    }

    return router_timeout;
}

void DVRouter::update_localip(void)
{
    struct sockaddr_in remote_address;
    int sockfd;

    // create socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
    {
        printf("Unabled to create datagram socket\n");
        return;
    }

    memset((char *)&remote_address, 0, sizeof(remote_address));
    remote_address.sin_family = AF_INET;
    remote_address.sin_port = htons(PORT_OF_IP_IN_INTERNET);
    if (inet_aton(IP_IN_INTERNET, &remote_address.sin_addr) == 0)
    {
        printf("inet_aton failed\n");
        return;
    }

    if (connect(sockfd, (struct sockaddr *)&remote_address, sizeof(remote_address)) < 0)
    {
        printf("UDP connect failed\n");
        return;
    }

    struct sockaddr_in local_address;
    socklen_t addressLength = sizeof(local_address);
    getsockname(sockfd, (struct sockaddr*)&local_address, &addressLength);

    my_ip = local_address.sin_addr;

    close(sockfd);
}

void DVRouter::do_bind(void)
{
    if((main_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
    {
        printf("socket() failed\n");
        exit(-1);
    }

    //Construct local address structure
    struct sockaddr_in local_address;                           // Local address
    memset(&local_address, 0 , sizeof(local_address));          // Zero out structure
    local_address.sin_family = AF_INET;                         // IPv4 address family
    local_address.sin_addr.s_addr = htonl(INADDR_ANY);          // Any incoming interface
    local_address.sin_port = htons(my_port);                    // Local port

    // Bind to the local address
    if(bind(main_fd, (struct sockaddr*) &local_address, sizeof(local_address)) < 0)
    {
        printf("bind() failed");
        exit(-1);
    }
}

void DVRouter::process_recvd_packet(void)
{
    char *reader = packet_buffer;

    // sender details
    uint16_t count;
    uint16_t sender_port;
    struct in_addr sender_ip;
    int16_t sender_id = -1;

    // remote node details
    uint16_t remote_cost;
    int16_t remote_id;

    // packet contents to print in order
    std::map<int16_t, uint16_t> packet_contents;

    // copy number of fields. this is equal to total servers
    memcpy(&count, reader, 2);
    reader = reader + 2;

    // copy sender port
    memcpy(&sender_port, reader, 2);
    reader = reader + 2;

    // copy sender ip
    memcpy(&sender_ip, reader, 4);
    reader = reader + 4;

    // based on sender ip, get sender id
    for (std::map<int16_t, DVNode*>::iterator it = allnodes.begin(); it != allnodes.end(); it++)
    {
        if (sender_ip.s_addr == it->second->node_ip.s_addr)
        {
            sender_id = it->second->node_id;
            break;
        }
    }

    if (sender_id == -1)
    {
        printf ("WARNING: received packet from anonymous %s\n", inet_ntoa(sender_ip));
        return;
    }
    else if (neighbors[sender_id]->link_state == LINK_DISABLED)
    {
        printf ("WARNING: ignoring packet received on disabled link\n");
        return;
    }
    else
    {
        cse4589_print_and_log("RECEIVED A MESSAGE FROM SERVER %d\n", sender_id);

        // increment received packet count
        pckts_recvd++;

        // reset idle count
        neighbors[sender_id]->idle_count = 0;

        // reset link status
        if (neighbors[sender_id]->link_state == LINK_TIMEDOUT)
            neighbors[sender_id]->link_state = LINK_NORMAL;
    }

    // for each server in packet, update routing table
    for (unsigned i = 0; i < count; i++)
    {
        // we only want id and cost, lets skip the nonsense
        reader = reader + 8;

        // copy remote id
        memcpy(&remote_id, reader, 2);
        reader = reader + 2;

        // copy remote cost
        memcpy(&remote_cost, reader, 2);
        reader = reader + 2;

        packet_contents[remote_id] = remote_cost;

        // update routing table
        if (remote_id != my_id)
            update(sender_id, remote_id, remote_cost, -1);
    }

    // print all packet contents
    for (std::map<int16_t, uint16_t>::iterator it = packet_contents.begin(); it != packet_contents.end(); it++)
        cse4589_print_and_log("%-15d%-15d\n", it->first , it->second);

    // check if we can route through sender. if we are already routing through sender, update cost
    for (std::map<int16_t, DVNode*>::iterator it = allnodes.begin(); it != allnodes.end(); it++)
    {
        uint16_t current_cost, cost_thru_sender;
        int16_t node_id = it->second->node_id;

        current_cost = routing_costs[my_id - 1][node_id - 1];
        cost_thru_sender = cost_sum(neighbors[sender_id]->get_linkcost(), routing_costs[sender_id - 1][node_id - 1]);

        if (allnodes[node_id]->route_thru != sender_id)
        {
            if (cost_thru_sender < current_cost)
                update(my_id, node_id, cost_thru_sender, sender_id);
        }
        else
        {
            update(my_id, node_id, cost_thru_sender, sender_id);
            check_alternate_routes(node_id);
        }
    }

    // restart senders timer
    remove_timer(sender_id);
    start_timer(sender_id);
}

void DVRouter::frame_bcast_packet(void)
{
    char *writer = packet_buffer;

    // copy number of fields. this is equal to total servers
    memcpy(writer, &num_servers, 2);
    writer = writer + 2;

    // copy our port
    memcpy(writer, &my_port, 2);
    writer = writer + 2;

    // copy server ip address
    memcpy(writer, &my_ip, 4);
    writer = writer + 4;

    // we should include entry saying cost to self is 0
    // copy server ip address
    memcpy(writer, &my_ip, 4);
    writer = writer + 4;

    // copy our port
    memcpy(writer, &my_port, 2);
    writer = writer + 2;

    // set next 2 bytes 0
    memset(writer, 0, 2);
    writer = writer + 2;

    // copy our id
    memcpy(writer, &my_id, 2);
    writer = writer + 2;

    // cost to self is 0
    memset(writer, 0, 2);
    writer = writer + 2;

    // copy all other server details
    for (std::map<int16_t, DVNode*>::iterator it = allnodes.begin(); it != allnodes.end(); it++)
    {
        DVNode *node = it->second;

        // copy node ip address
        memcpy(writer, &node->node_ip.s_addr, 4);
        writer = writer + 4;

        // copy node port
        memcpy(writer, &node->node_port, 2);
        writer = writer + 2;

        // set next 2 bytes 0
        memset(writer, 0, 2);
        writer = writer + 2;

        // copy node id 
        memcpy(writer, &node->node_id, 2);
        writer = writer + 2;

        // copy node cost 
        memcpy(writer, &(routing_costs[my_id - 1][node->node_id - 1]), 2);
        writer = writer + 2;
    }
}

void DVRouter::broadcast_costs(void)
{
    DVNode *node;
    struct sockaddr_in neighboraddr;

    // create packet to be sent to all
    frame_bcast_packet();

    // send it to all neighbors
    for (std::map<int16_t, DVNode*>::iterator it = neighbors.begin(); it != neighbors.end(); it++)
    {
        node = it->second;

        // don't send updates on disabled links
        if (node->link_state == LINK_DISABLED)
            continue;

        bzero(&neighboraddr, sizeof(neighboraddr));
        neighboraddr.sin_family = AF_INET;
        neighboraddr.sin_addr.s_addr = node->node_ip.s_addr;
        neighboraddr.sin_port = htons(node->node_port);

        sendto(main_fd, packet_buffer, packet_buffer_size, 0, (struct sockaddr *)&neighboraddr, sizeof(neighboraddr));
    }
}

void DVRouter::update(int16_t id1, int16_t id2, uint16_t cost, int16_t via)
{
    int16_t id = -1;

    if (id1 <= 0 || id1 > num_servers)
    {
        printf ("WARNING: invalid id %d\n", id1);
        return;
    }

    if (id2 <= 0 || id2 > num_servers)
    {
        printf ("WARNING: invalid id %d\n", id2);
        return;
    }

    if (id1 == id2 && cost != 0)
    {
        printf ("WARNING: cost to same node is always 0. changing it is not allowed\n");
        return;
    }

    routing_costs[id1 - 1][id2 - 1] = cost;
    routing_costs[id2 - 1][id1 - 1] = cost;

    if (my_id == id1)
        id = id2;
    else if (my_id == id2)
        id = id1;

    if (id != -1) // this update involves us
        allnodes[id]->route_thru = via;
}

bool DVRouter::update_linkcost(int16_t id1, int16_t id2, uint16_t cost)
{
    int16_t id = -1;

    if (id1 <= 0 || id1 > num_servers)
    {
        error_msg = "Invalid id";
        printf ("WARNING: invalid id %d\n", id2);
        return false;
    }

    if (id2 <= 0 || id2 > num_servers)
    {
        error_msg = "Invalid id";
        printf ("WARNING: invalid id %d\n", id2);
        return false;
    }

    if (id1 == id2)
    {
        error_msg = "Invalid link. There are no self links";
        printf ("WARNING: invalid link. there are no self links\n");
        return false;
    }

    if (my_id == id1)
        id = id2;
    else if (my_id == id2)
        id = id1;

    if (id == -1) // this is not a valid link
    {
        error_msg = "Invalid link. This is not my link";
        printf ("WARNING: invalid link. not our link\n");
        return false;
    }

    if (neighbors.find(id) == neighbors.end()) // neighbor doesn't exist
    {
        error_msg = "Invalid link. This is not my neighbor";
        printf ("WARNING: id %d is not our neighbor. can't update link cost\n", id);
        return false;
    }

    // change the link cost
    (neighbors[id])->link_cost = cost;

    // update the cost to all nodes affected by this change
    for (std::map<int16_t, DVNode*>::iterator it = allnodes.begin(); it != allnodes.end(); it++)
    {
        if (it->second->route_thru == id)
        {
            update(my_id, it->second->node_id, cost_sum(cost, routing_costs[id - 1][it->second->node_id - 1]), id);
            check_alternate_routes(it->second->node_id);
        }
    }

    return true;
}

bool DVRouter::update_linkstate(int16_t id1, int16_t id2, LinkState state)
{
    int16_t id = -1;

    if (id1 <= 0 || id1 > num_servers)
    {
        error_msg = "Invalid id";
        printf ("WARNING: invalid id %d\n", id2);
        return false;
    }

    if (id2 <= 0 || id2 > num_servers)
    {
        error_msg = "Invalid id";
        printf ("WARNING: invalid id %d\n", id2);
        return false;
    }

    if (id1 == id2)
    {
        error_msg = "Invalid link. There are no self links";
        printf ("WARNING: invalid link. there are no self links\n");
        return false;
    }

    if (my_id == id1)
        id = id2;
    else if (my_id == id2)
        id = id1;

    if (id == -1) // this is not a valid link
    {
        error_msg = "Invalid link. This is not my link";
        printf ("WARNING: invalid link. not our link\n");
        return false;
    }

    if (neighbors.find(id) == neighbors.end()) // neighbor doesn't exist
    {
        error_msg = "Invalid link. This is not my neighbor";
        printf ("WARNING: id %d is not our neighbor. can't update link cost\n", id);
        return false;
    }

    // change the link state
    neighbors[id]->link_state = state;

    // update the cost to all nodes affected by this change
    for (std::map<int16_t, DVNode*>::iterator it = allnodes.begin(); it != allnodes.end(); it++)
    {
        if (it->second->route_thru == id)
        {
            update(my_id, it->second->node_id, cost_sum(neighbors[id]->get_linkcost(), routing_costs[id - 1][it->second->node_id - 1]), id);
            check_alternate_routes(it->second->node_id);
        }
    }

    return true;
}

void DVRouter::check_alternate_routes(int16_t id)
{
    if (id <= 0 || id > num_servers)
        return;

    uint16_t proposed_cost, current_cost = routing_costs[my_id - 1][id - 1];

    for (std::map<int16_t, DVNode*>::iterator it = neighbors.begin(); it != neighbors.end(); it++)
    {
        proposed_cost = cost_sum(it->second->get_linkcost(), routing_costs[it->second->node_id - 1][id - 1]);

        if (proposed_cost < current_cost)
        {
            current_cost = proposed_cost;
            update(my_id, id, proposed_cost, it->second->node_id);
        }
    }
}

bool DVRouter::process_command(std::string args[])
{
    bool retval = false;

    if (args[0] == "update")
    {
        std::transform(args[3].begin(), args[3].end(), args[3].begin(), tolower);

        retval = update_linkcost(atoi(args[1].c_str()), atoi(args[2].c_str()), (args[3] == "inf") ? INFINITE_COST : atoi(args[3].c_str()));
        if (retval)
            broadcast_costs();
    }
    else if (args[0] == "step")
    {
        broadcast_costs();
        retval = true;
    }
    else if (args[0] == "packets")
    {
        // print and reset received packet count
        cse4589_print_and_log("%d\n", pckts_recvd);
        pckts_recvd = 0;
        retval = true;
    }
    else if (args[0] == "display")
    {
        // std::map sorts elements by default
        for (std::map<int16_t, DVNode*>::iterator it = allnodes.begin(); it != allnodes.end(); it++)
        {
            int16_t destination_id, next_hop;
            uint16_t cost;

            destination_id = it->first;
            cost = routing_costs[my_id - 1][destination_id - 1];

            if (cost == INFINITE_COST)
                next_hop = -1;
            else
                next_hop = it->second->route_thru;

            cse4589_print_and_log("%-15d%-15d%-15d\n", destination_id, next_hop, cost);
        }
        retval = true;
    }
    else if (args[0] == "disable")
    {
        // set the cost to infinite to disable the link
        retval = update_linkstate(my_id, atoi(args[1].c_str()), LINK_DISABLED);
    }
    else if (args[0] == "crash")
    {
        // close main fd and remove it from select
        if (main_fd != 0)
        {
            FD_CLR(main_fd, &read_fds);
            FD_CLR(STDIN_FILENO, &read_fds);

            close(main_fd);
            main_fd = 0;

            retval = true;
        }
        else
        {
            retval = false;
            error_msg = "All connections are already closed";
        }
    }
    else if (args[0] == "dump")
    {
        frame_bcast_packet();
        cse4589_dump_packet(packet_buffer, packet_buffer_size);
        retval = true;
    }
    else if (args[0] == "academic_integrity")
    {
        cse4589_print_and_log("I have read and understood the course academic integrity policy located at http://www.cse.buffalo.edu/faculty/dimitrio/courses/cse4589_f14/index.html#integrity");
        printf("\n");
        retval = true;
    }
    else
    {
        std::cout << "Unknown command " << args[0] << " entered\n";
        error_msg = "Unknown command";
        retval = false;
    }

    return retval;
}

void DVRouter::start(void)
{
    int nbytes, max_fd;
    std::string args[CMD_MAX_ARGS];
    char space[] = " ";
    struct timeval timeout;

    fd_set temp_read_fds;

    FD_SET(STDIN_FILENO, &read_fds);
    FD_SET(main_fd, &read_fds);

    max_fd = std::max(STDIN_FILENO, main_fd);

    // start our timer
    start_timer(my_id);

    while(1)
    {
        // process any expired timers
        timeout.tv_sec = process_timers(); // timeout.tv_sec has timeout for front timer

        // fill stuff for select
        timeout.tv_usec = 0;
        temp_read_fds = read_fds;

        if (select(max_fd + 1, &temp_read_fds, NULL, NULL, &timeout) < 0)
            break;

        // process any expired timers
        process_timers();

        if (main_fd != 0 && FD_ISSET(main_fd, &temp_read_fds))
        {
            struct sockaddr their_addr;
            socklen_t their_addr_len = sizeof(their_addr);

            if ((nbytes = recvfrom(main_fd, packet_buffer, packet_buffer_size, 0, &their_addr, &their_addr_len)) == -1)
            {
                printf("recvfrom failed");
                exit(1);
            }

            if (nbytes == packet_buffer_size)
                process_recvd_packet();
            else
                printf ("WARNING: packet of size %d is received. dropping it. we only process %d size packets\n", nbytes, packet_buffer_size);
        }

        if (FD_ISSET(STDIN_FILENO, &temp_read_fds))
        {
            if ((nbytes = read(STDIN_FILENO, command_buffer + write_here, 
                            COMMAND_BUFFER - write_here)) <= 0)
            {
                if (nbytes == 0)
                    printf("Exiting at user's request\n");
                else
                    printf("read failed with %d\n", nbytes);

                break;
            }

            write_here += nbytes;

            if (command_buffer[write_here - 1] == '\n')
            {
                command_buffer[write_here - 1] = '\0';
                write_here = 0;

                char *token = strtok(command_buffer, space);
                for (int i = 0; i < CMD_MAX_ARGS; i++)
                {
                    if (token == NULL)
                        break;

                    args[i].assign(token);
                    token = strtok(NULL, space);
                }

                std::transform(args[0].begin(), args[0].end(), args[0].begin(), tolower);
                if (process_command(args))
                    cse4589_print_and_log("%s:SUCCESS\n", args[0].c_str());
                else
                    cse4589_print_and_log("%s:%s\n", args[0].c_str(), error_msg.c_str());

                for (int i = 0; i < CMD_MAX_ARGS; i++)
                    args[i].clear();
            }
        }
    }
}
