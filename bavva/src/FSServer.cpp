#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <unistd.h>
#include <cstring>
#include <algorithm>

#include "../include/FSServer.h"

// class implementation
FSServer::FSServer(int port):FSNode(port, true){}
FSServer::~FSServer(){}

void FSServer::process_newconnection(FSConnection *connection)
{
    connections.push_back(connection);
}

void FSServer::process_register_request(FSHeader *header)
{
    char *hostname, *ipaddress, *portstring;

    if (header == NULL)
        return;

    hostname = strtok(header->metadata, ",");
    ipaddress = strtok(NULL, ",");
    portstring = strtok(NULL, ",");

    add_serverip(new ServerIP(ipaddress, hostname, atoi(portstring)));
    bcast_serverip_list_flag = true;
}

void FSServer::process_command(std::string args[])
{
    if (args[0] == "creator")
    {
        printf("Bharadwaj Avva\nbavva\nbavva@buffalo.edu\n");
        printf("I have read and understood the course academic integrity policy located at http://www.cse.buffalo.edu/faculty/dimitrio/courses/cse4589_f14/index.html#integrity\n");
    }
    else if (args[0] == "help")
    {
        printf("Following commands are available to use\n"
                "1. CREATOR: Displays full name, UBIT name and UB email address of creator\n"
                "2. HELP: Displays this help information\n"
                "3. MYIP: Displays the IP address of this process\n"
                "4. MYPORT: Displays the port on which this process is listening for incoming connections\n");
    }
    else if (args[0] == "myip")
    {
        printf("IP address:%s\n", local_ip.c_str());
    }
    else if (args[0] == "myport")
    {
        printf("Port number:%d\n", port);
    }
    else if (args[0] == "register")
    {
        printf("REGISTER command is not applicable for server\n");
    }
    else if (args[0] == "connect")
    {
        printf("CONNECT command is not applicable for server\n");
    }
    else
    {
        std::cout << "Unknown command " << args[0] << " entered\n";
    }
}
