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
    if (connection->is_broken())
    {
        delete connection;
        return;
    }

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

    server_ipaddress = connection->peer_ip;
}

void FSClient::make_connection(std::string peer_address, int peer_port)
{
    std::string peer_ip;
    std::string peer_name;

    if (connections.size() >= 4) // don't accept new connections
    {
        printf ("More than 4 connections (including the one with server) is not allowed\n");
        return;
    }

    hostname_to_ip(peer_address, peer_ip);
    if (peer_ip.empty())
    {
        printf ("Invalid address entered\n");
        return;
    }

    if ((peer_ip == local_ip) || (peer_ip == "127.0.0.1"))
    {
        printf ("Self connections are not allowed\n");
        return;
    }

    for (std::list<FSConnection*>::iterator it = connections.begin(); it != connections.end(); ++it)
    {
        // only one connection per ip
        if ((*it)->peer_ip == peer_ip)
        {
            printf ("Duplicate connections are not allowed\n");
            return;
        }
    }

    bool ip_found = false;
    for (std::list<ServerIP*>::iterator it = server_ip_list.begin(); it != server_ip_list.end(); ++it)
    {
        // delete connection if the peer is not among server ip list
        if ((*it)->server_ip == peer_ip)
        {
            peer_name = (*it)->server_name;
            ip_found = true;
            break;
        }
    }

    if (ip_found == false)
    {
        printf ("Can not connect as given address is not among Server-IP-List\n");
        return;
    }

    FSConnection *connection = new FSConnection(false, peer_ip.c_str(), peer_port, (FSNode *)this);
    if (connection->is_broken())
    {
        printf ("Unable to establish connection to %s:%d\n", peer_ip.c_str(), peer_port);
        delete connection;
        return;
    }

    connection->peer_name = peer_name;

    printf ("Successfully established connection to %s:%d\n", peer_ip.c_str(), peer_port);
    connections.push_back(connection);
}

void FSClient::process_newconnection(FSConnection *connection)
{
    if (connections.size() >= 4) // don't accept new connections
    {
        delete connection;
        return;
    }

    if (connection->peer_ip == local_ip)
    {
        printf ("Rejecting self connection\n");
        delete connection;
        return;
    }

    for (std::list<FSConnection*>::iterator it = connections.begin(); it != connections.end(); ++it)
    {
        // only one connection per ip
        if (connection->peer_ip == (*it)->peer_ip)
        {
            printf ("Rejecting duplicate connection\n");
            delete connection;
            return;
        }
    }

    bool ip_found = false;
    for (std::list<ServerIP*>::iterator it = server_ip_list.begin(); it != server_ip_list.end(); ++it)
    {
        // delete connection if the peer is not among server ip list
        if (connection->peer_ip == (*it)->server_ip)
        {
            connection->peer_name = (*it)->server_name;
            connection->peer_port = (*it)->port;
            ip_found = true;
            break;
        }
    }

    if (ip_found == false)
    {
        printf ("Rejecting connection as it is not among Server-IP-List\n");
        delete connection;
        return;
    }

    printf ("New connection to %s established\n", connection->peer_ip.c_str());

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
    bool ip_found;
    for (std::list<FSConnection*>::iterator it = connections.begin(); it != connections.end();)
    {
        if ((*it)->with_server == true)
        {
            ++it;
            continue;
        }

        ip_found = false;
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
            printf ("Terminating connection to %s as it is not present in new Server-IP-List\n", (*it)->peer_ip.c_str());
            delete (*it);
            it = connections.erase(it);
        }
        else
        {
            ++it;
        }
    }

    // print new server ip list
    printf ("\nServer-IP-List update received:\n");
    print_server_ip_list();
}

void FSClient::terminate_connection(int connection_id)
{
    if ((connection_id <= 0) || (connection_id > connections.size()))
    {
        printf("Not a valid connection id\n");
        return;
    }

    std::list<FSConnection*>::iterator it = connections.begin();
    std::advance(it, connection_id - 1);

    if (*it != NULL)
    {
        printf ("Terminating connection id %d with %s\n", connection_id, (*it)->peer_ip.c_str());
        delete (*it);
        connections.erase(it);
    }
    else
    {
        printf ("No valid connection found for given id %d\n", connection_id);
    }
}

void FSClient::terminate_allconnections(void)
{
    FSConnection *connection = NULL;

    while (!connections.empty())
    {
        connection = connections.front(); 
        delete connection;
        connections.pop_front();
    }

    request_exit = true;
}

void FSClient::process_upload(int connection_id, std::string filename)
{
    FSConnection *connection;
    char buffer[METADATA_SIZE];

    if ((connection_id <= 0) || (connection_id > connections.size()))
    {
        printf ("Given connection id is invalid\n");
        return;
    }
    else if (connection_id == 1)
    {
        printf ("Uploading file to server is not allowed\n");
        return;
    }

    if (access(filename.c_str(), R_OK) != 0)
    {
        printf ("File does not exist or it is not readable\n");
        return;
    }

    std::list<FSConnection*>::iterator it = connections.begin();
    std::advance(it, connection_id - 1);
    connection = *it;

    // clean the buffer
    memset(buffer, 0, METADATA_SIZE);
    strncpy(buffer, filename.c_str(), METADATA_SIZE);

    connection->send_message(MSG_TYPE_SEND_FILE, 0, buffer);
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
    else if (args[0] == "connect")
    {
        make_connection(args[1], atoi(args[2].c_str()));
    }
    else if (args[0] == "list")
    {
        print_all_connections();
    }
    else if (args[0] == "terminate")
    {
        terminate_connection(atoi(args[1].c_str()));
    }
    else if (args[0] == "exit")
    {
        terminate_allconnections();
    }
    else if (args[0] == "upload")
    {
        process_upload(atoi(args[1].c_str()), args[2]);
    }
    else
    {
        std::cout << "Unknown command " << args[0] << " entered\n";
    }
}
