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
    this->router_timeout = router_timeout;
    update_localip(); // has to be done before initialize

    // read topology file and update variables
    initialize(topology);
    
    // all this need to be done before start
    do_bind();
}

DVRouter::~DVRouter()
{
}

void DVRouter::initialize(std::string topology)
{
    std::string line;
    std::ifstream input(topology.c_str());

    // to create DVNodes(servers)
    std::string ipaddressstr;
    unsigned short port;
    unsigned id;

    // for ipaddress stff
    struct in_addr ipaddr;

    // for neighbors cost
    unsigned id1, id2;
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
            DVNode *node = new DVNode(ipaddr, port, INFINITE_COST, id, false);
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
        neighbors[id] = allnodes[id];

        // update cost in routing table too
        routing_costs[my_id - 1][id - 1] = cost;
        routing_costs[id - 1][my_id - 1] = cost;
    }
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

void DVRouter::update(unsigned id1, unsigned id2, unsigned short cost)
{
    unsigned id = 0;

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

    if (id != 0) // cost involvind us
        allnodes[id]->node_cost = cost;
}

void DVRouter::process_command(std::string args[])
{
    if (args[0] == "update")
    {
    }
    else if (args[0] == "step")
    {
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
    }
    else if (args[0] == "academic_integrity")
    {
        cse4589_print_and_log("I have read and understood the course academic integrity policy located at http://www.cse.buffalo.edu/faculty/dimitrio/courses/cse4589_f14/index.html#integrity");
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
        timeout.tv_sec = router_timeout;
        timeout.tv_usec = 0;

        temp_read_fds = read_fds;

        if (select(max_fd + 1, &temp_read_fds, NULL, NULL, &timeout) < 0)
            break;

        if (FD_ISSET(main_fd, &temp_read_fds))
        {
            // recv packet and process
        }

        if (FD_ISSET(STDIN_FILENO, &temp_read_fds))
        {
            if ((nbytes = read(STDIN_FILENO, command_buffer + write_here, 
                            COMMAND_BUFFER - write_here)) <= 0)
            {
                if (nbytes == 0)
                    printf("Exiting at user's request\n");
                else
                    printf("recv failed with %d\n", nbytes);

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
