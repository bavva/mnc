#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <unistd.h>
#include <cstring>

#include "../include/FSClient.h"

// class implementation
FSClient::FSClient(int port):FSNode(port, false){}
FSClient::~FSClient(){}

void FSClient::register_self(std::string server_ip, int server_port)
{
    int nchars;
    char buffer[METADATA_SIZE];
    char hostname[128];
    char *writer = NULL;

    if (connections.size() > 0)
    {
        printf("Already connected to different server\n");
        return;
    }

    FSConnection *connection = new FSConnection(true, server_ip.c_str(), server_port, (FSNode *)this);
    connections.push_back(connection);

    // clean the buffer
    writer = buffer;
    memset(writer, 0, METADATA_SIZE);

    // get hostname and copy it
    memset(hostname, 0, 128);
    gethostname(hostname, 127);
    nchars = strlen(hostname);
    strncpy(writer, hostname, nchars);
    writer += nchars;

    // add comma
    strncpy(writer, ",", 1);
    writer += 1;

    // copy ip address
    nchars = local_ip.length();
    strncpy(writer, local_ip.c_str(), nchars);
    writer += nchars;

    // add comma
    strncpy(writer, ",", 1);
    writer += 1;

    // copy port
    nchars = sprintf(writer, "%d", port);
    writer += nchars;

    connection->send_message(MSG_TYPE_REGISTER_REQUEST, 0, buffer);
}

void FSClient::process_newconnection(FSConnection *connection)
{
    if (connections.size() >= 4) // don't accept new connections
    {
        delete connection;
        return;
    }

    for (std::list<FSConnection*>::iterator it = connections.begin(); it != connections.end(); ++it)
    {
        // only one connection per ip
        if (connection->peer_ip == (*it)->peer_ip)
        {
            delete connection;
            return;
        }
    }

    connections.push_back(connection);
}

void FSClient::process_register_response(FSHeader *header)
{
    char *hostname, *ipaddress, *portstring;

    if (header == NULL)
        return;

    // empty server ip list
    while(!server_ip_list.empty())
    {
        delete server_ip_list.front();
        server_ip_list.pop_front();
    }

    if (header->content_length < 1)
        return;

    // add all received to server ip list
    hostname = strtok(header->metadata, ",");
    ipaddress = strtok(NULL, ",");
    portstring = strtok(NULL, ",");
    add_serverip(new ServerIP(ipaddress, hostname, atoi(portstring)));

    for (int i = 0; i < header->content_length - 1; i++)
    {
        hostname = strtok(NULL, ",");
        ipaddress = strtok(NULL, ",");
        portstring = strtok(NULL, ",");
        add_serverip(new ServerIP(ipaddress, hostname, atoi(portstring)));
    }

    // remove any connections to host which is not there in server ip list
    std::list<FSConnection*>::iterator it = connections.begin();
    if (it == connections.end())
        return;
    else
        it++;

    bool ip_found;
    for (;it != connections.end();)
    {
        ip_found == false;
        for (std::list<ServerIP*>::iterator ite = server_ip_list.begin(); ite != server_ip_list.end(); ++ite)
        {
            if ((*ite)->server_ip == (*it)->peer_ip)
            {
                ip_found = true;
                break;
            }
        }

        if (ip_found == false)
        {
            delete (*it);
            it = connections.erase(it);
        }
        else
        {
            ++it;
        }
    }

    // print new server ip list
    print_server_ip_list();
}

void FSClient::process_command(std::string args[])
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
        register_self(args[1], atoi(args[2].c_str()));
    }
    else
    {
        std::cout << "Unknown command " << args[0] << " entered\n";
    }
}
