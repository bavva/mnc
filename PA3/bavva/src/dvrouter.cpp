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

// class implementation
DVRouter::DVRouter(std::string topology, time_t router_timeout)
{
    // internal things
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

    // free all nodes
    for (std::map<int, DVNode*>::iterator it = allnodes.begin(); it != allnodes.end(); it++)
        delete it->second;

    // free all timers
    while (!timer_list.empty())
        delete timer_list.front();

    // free packet buffer
    delete packet_buffer;
}

void DVRouter::initialize(std::string topology)
{
    std::string line;
    std::ifstream input(topology.c_str());

    // to create DVNodes(servers)
    std::string ipaddressstr;
    unsigned short port;
    int id;

    // for ipaddress stff
    struct in_addr ipaddr;

    // for neighbors cost
    int id1, id2;
    unsigned short cost;

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
    routing_costs = new unsigned short*[num_servers]; // allocate memory
    for (int i = 0; i < num_servers; i++)
    {
        routing_costs[i] = new unsigned short[num_servers];
    }
    for (int i = 0; i < num_servers; i++) // initialize
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

        // update our port
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

    // read the costs of neighbors and update
    while (std::getline(input, line))
    {
        std::istringstream iss(line);

        iss >> id1;
        iss >> id2;
        iss >> cost;

        id = (id1 == my_id) ? id2 : id1;
        allnodes[id]->node_cost = cost;
        allnodes[id]->is_neighbor = true;
        allnodes[id]->route_thru = id;
        neighbors[id] = allnodes[id];

        // update cost in routing table too
        routing_costs[my_id - 1][id - 1] = cost;
        routing_costs[id - 1][my_id - 1] = cost;
    }

    // allocate buffer to frame packet
    packet_buffer_size = 8 + num_servers * 12;
    packet_buffer = new char[packet_buffer_size];
}

void DVRouter::start_timer(int id)
{
    DVTimer *new_timer = NULL;
    std::list<DVTimer*>::iterator it;

    // don't start duplicate timer
    if (timer_map.find(id) != timer_map.end())
        return;

    // create timer node
    new_timer = new DVTimer(id, time(NULL) + router_timeout);

    // insert in map and list
    timer_list.push_back(new_timer);
    it = timer_list.end();
    timer_map[id] = --it;
}

void DVRouter::remove_timer(int id)
{
    DVTimer *timer = NULL;
    std::list<DVTimer*>::iterator it;

    // return if timer doen't exist
    if (timer_map.find(id) == timer_map.end())
        return;

    // get timer and iterator of timer in the list
    it = timer_map[id];
    timer = *it;

    // delete timer in list and map
    timer_list.erase(it);
    timer_map.erase(id);

    // free timer
    delete timer;
}

void DVRouter::on_fire(int id)
{
}

time_t DVRouter::process_timers(void)
{
    time_t current_time = time(NULL);
    DVTimer *timer = NULL;
    int id;

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
    unsigned count;
    unsigned short peer_port, remote_cost;
    struct in_addr peer_ip;
    char *reader = command_buffer;
    int peer_id = -1, remote_id;

    // copy number of fields. this is equal to total servers
    memcpy(&count, reader, 2);
    reader = reader + 2;

    // copy peer port
    memcpy(&peer_port, reader, 2);
    reader = reader + 2;

    // copy peer ip
    memcpy(&peer_ip, reader, 4);
    reader = reader + 4;

    // based on peer ip, get peer id
    for (std::map<int, DVNode*>::iterator it = allnodes.begin(); it != allnodes.end(); it++)
    {
        if (peer_ip.s_addr == it->second->node_ip.s_addr)
        {
            peer_id = it->second->node_id;
            break;
        }
    }

    if (peer_id == -1)
        return;

    // for each server in packet
    for (unsigned i = 0; i < count; i++)
    {
        reader = reader + 8;

        // copy remote id
        memcpy(&remote_id, reader, 2);
        reader = reader + 2;

        // copy remote cost
        memcpy(&remote_cost, reader, 2);
        reader = reader + 2;
    }
}

void DVRouter::frame_bcast_packet(void)
{
    char *writer = command_buffer;

    // copy number of fields. this is equal to total servers
    memcpy(writer, &num_servers, 2);
    writer = writer + 2;

    // copy our port
    memcpy(writer, &my_port, 2);
    writer = writer + 2;

    // copy server ip address
    memcpy(writer, &my_ip, 4);
    writer = writer + 4;

    // copy all other server details
    for (std::map<int, DVNode*>::iterator it = allnodes.begin(); it != allnodes.end(); it++)
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
        memcpy(writer, &node->node_cost, 2);
        writer = writer + 2;
    }
}

void DVRouter::broadcast_costs(void)
{
    DVNode *node;
    struct sockaddr_in neighboraddr;

    frame_bcast_packet();

    for (std::map<int, DVNode*>::iterator it = neighbors.begin(); it != neighbors.end(); it++)
    {
        node = it->second;

        bzero(&neighboraddr,sizeof(neighboraddr));
        neighboraddr.sin_family = AF_INET;
        neighboraddr.sin_addr.s_addr = node->node_ip.s_addr;
        neighboraddr.sin_port = htons(node->node_port);

        sendto(main_fd, packet_buffer, packet_buffer_size, 0, (struct sockaddr *)&neighboraddr, sizeof(neighboraddr));
    }
}

void DVRouter::update(int id1, int id2, unsigned short cost, int via)
{
    int id = -1;

    if (id1 <= 0 || id1 > num_servers)
        return;

    if (id2 <= 0 || id2 > num_servers)
        return;

    if (id1 == id2)
        return;

    routing_costs[id1 - 1][id2 - 1] = cost;
    routing_costs[id2 - 1][id1 - 1] = cost;

    if (my_id == id1)
        id = id2;
    else if (my_id == id2)
        id = id1;

    if (id != -1) // this update involves us
        neighbors[id]->route_thru = via;
}

void DVRouter::update_linkcost(int id1, int id2, unsigned short cost)
{
    int id = -1;

    if (id1 <= 0 || id1 > num_servers)
        return;

    if (id2 <= 0 || id2 > num_servers)
        return;

    if (id1 == id2)
        return;

    if (my_id == id1)
        id = id2;
    else if (my_id == id2)
        id = id1;

    if (id == -1) // this is not a valid link
        return;

    if (neighbors.find(id) == neighbors.end()) // neighbor doesn't exist
        return;

    (neighbors[id])->node_cost = cost;
}

void DVRouter::process_command(std::string args[])
{
    if (args[0] == "update")
    {
        std::transform(args[3].begin(), args[3].end(), args[3].begin(), tolower);

        update_linkcost(atoi(args[1].c_str()), atoi(args[2].c_str()), (args[3] == "inf") ? INFINITE_COST : atoi(args[3].c_str()));
        broadcast_costs();
    }
    else if (args[0] == "step")
    {
        broadcast_costs();
    }
    else if (args[0] == "packets")
    {
    }
    else if (args[0] == "display")
    {
    }
    else if (args[0] == "disable")
    {
    }
    else if (args[0] == "crash")
    {
    }
    else if (args[0] == "dump")
    {
        frame_bcast_packet();
        cse4589_dump_packet(packet_buffer, packet_buffer_size);
    }
    else if (args[0] == "academic_integrity")
    {
        cse4589_print_and_log("I have read and understood the course academic integrity policy located at http://www.cse.buffalo.edu/faculty/dimitrio/courses/cse4589_f14/index.html#integrity");
        std::cout << std::endl;
    }
    else
    {
        std::cout << "Unknown command " << args[0] << " entered\n";
    }
}

void DVRouter::start(void)
{
    int nbytes, max_fd;
    std::string args[CMD_MAX_ARGS];
    char space[] = " ";
    struct timeval timeout;

    fd_set read_fds;
    fd_set temp_read_fds;

    FD_SET(STDIN_FILENO, &read_fds);
    FD_SET(main_fd, &read_fds);

    max_fd = std::max(STDIN_FILENO, main_fd);

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

        if (FD_ISSET(main_fd, &temp_read_fds))
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
                process_command(args);

                for (int i = 0; i < CMD_MAX_ARGS; i++)
                    args[i].clear();
            }
        }
    }
}
