#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <unistd.h>
#include <cstring>
#include <algorithm>
#include <sys/time.h>

#include "../include/FSServer.h"

// class implementation
FSServer::FSServer(int port):FSNode(port, true),stat_wait_count(0){}
FSServer::~FSServer()
{
    StatisticsEntry *entry;

    // delete all stuff in stats
    while (!stats.empty())
    {
        entry = stats.front();
        delete entry;
        stats.pop_front();
    }
}

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

    // uppdate hostname and ipaddress in connections list
    for (std::list<FSConnection*>::iterator it = connections.begin(); it != connections.end(); ++it)
    {
        if ((*it)->peer_ip == ipaddress)
        {
            (*it)->peer_name = hostname;
            (*it)->peer_port = atoi(portstring);
            break;
        }
    }

    bcast_serverip_list_flag = true;
}

void FSServer::fetch_stats(void)
{
    struct timeval current_time;
    StatisticsEntry *entry;

    if (stat_wait_count > 0)
    {
        gettimeofday(&current_time, NULL);
        if ((current_time.tv_sec - last_stats_time.tv_sec) <= 5)
        {
            printf ("Previous stats command is still being processed\n");
            return;
        }
        else
        {
            print_stats();
        }
    }

    gettimeofday(&last_stats_time, NULL);

    // delete all stuff in stats
    while (!stats.empty())
    {
        entry = stats.front();
        delete entry;
        stats.pop_front();
    }

    // create new stats structure
    for (std::list<ServerIP*>::iterator itx = server_ip_list.begin(); itx != server_ip_list.end(); itx++)
    {
        for (std::list<ServerIP*>::iterator ity = server_ip_list.begin(); ity != server_ip_list.end(); ity++)
        {
            if ((*itx)->server_name != (*ity)->server_name)
            {
                stats.push_back(new StatisticsEntry((*itx)->server_name, (*ity)->server_name));
            }
        }
    }

    // this many stat queries have to be answered before display
    stat_wait_count = stats.size();
    cur_entry = stats.begin();

    // send queries loop back 
    if (cur_entry != stats.end())
        send_fetch_stat_request((*cur_entry)->host1, (*cur_entry)->host2);
}

void FSServer::process_stats_response(FSHeader *header)
{
    char *token;
    char host1[HOSTNAME_LEN];
    char host2[HOSTNAME_LEN];

    int total_downloads;
    int total_uploads;
    unsigned long total_upload_data;
    unsigned long total_download_data;
    unsigned long total_upload_time;
    unsigned long total_download_time;

    memset(host1, 0, HOSTNAME_LEN);
    memset(host2, 0, HOSTNAME_LEN);

    sscanf(header->metadata, "%s,%s,%d,%d,%ld,%ld,%ld,%ld", host1, host2, &total_downloads, &total_uploads, &total_upload_data, &total_download_data, &total_upload_time, &total_download_time);

    token = strtok(header->metadata, ",");
    if (token != NULL)
        strcpy(host1, token);
    token = strtok(NULL, ",");
    if (token != NULL)
        strcpy(host2, token);
    token = strtok(NULL, ",");
    if (token != NULL)
        total_downloads = atoi(token);
    token = strtok(NULL, ",");
    if (token != NULL)
        total_uploads = atoi(token);
    token = strtok(NULL, ",");
    if (token != NULL)
        total_upload_data = atol(token);
    token = strtok(NULL, ",");
    if (token != NULL)
        total_download_data = atol(token);
    token = strtok(NULL, ",");
    if (token != NULL)
        total_upload_time = atol(token);
    token = strtok(NULL, ",");
    if (token != NULL)
        total_download_time = atol(token);

    for (std::list<StatisticsEntry*>::iterator it = stats.begin(); it != stats.end(); it++)
    {
        if ((*it)->host1 == host1 && (*it)->host2 == host2)
        {
            (*it)->total_downloads = total_downloads;
            (*it)->total_uploads = total_uploads;
            (*it)->total_upload_data = total_upload_data;
            (*it)->total_download_data = total_download_data;
            (*it)->total_upload_time = total_upload_time;
            (*it)->total_download_time = total_download_time;
            break;
        }
    }

    stat_wait_count--;
    cur_entry++;

    if (stat_wait_count <= 0)
    {
        print_stats();
        stat_wait_count = 0;
    }
    else
    {
        stats_ready = true;
    }
}

void FSServer::process_next_stat_request(void)
{
    if (stat_wait_count > 0)
    {
        send_fetch_stat_request((*cur_entry)->host1, (*cur_entry)->host2);
    }
}

void FSServer::send_fetch_stat_request(std::string host1, std::string host2)
{
    int nchars;
    char buffer[METADATA_SIZE];
    char *writer = NULL;
    FSConnection *connection = NULL;

    stats_ready = false;

    for (std::list<FSConnection*>::iterator it = connections.begin(); it != connections.end(); ++it)
    {
        if ((*it)->peer_name == host1)
        {
            connection = *it;
            break;
        }
    }

    // clean the buffer
    writer = buffer;
    memset(writer, 0, METADATA_SIZE);

    // copy host1
    nchars = host1.length();
    strncpy(writer, host1.c_str(), nchars);
    writer += nchars;

    // add comma
    strncpy(writer, ",", 1);
    writer += 1;

    // copy host2
    nchars = host2.length();
    strncpy(writer, host2.c_str(), nchars);
    writer += nchars;

    connection->send_message(MSG_TYPE_REQUEST_STATS, 0, buffer);
}

void FSServer::print_stats(void)
{
    StatisticsEntry *entry;

    unsigned long download_speed, upload_speed;
    printf ("%-35s%-35s%-20s%-30s%-20s%-30s\n", "Hostname 1", "Hostname 2", "Total Uploads", "Average upload Speed (bps)", "Total Downloads", "Average download speed (bps)");
    for (std::list<StatisticsEntry*>::iterator it = stats.begin(); it != stats.end(); ++it)
    {
        entry = *it;
        if (entry->total_uploads == 0 && entry->total_downloads == 0)
            continue;

        if (entry->total_download_time > 0)
            download_speed = (entry->total_download_data * 1000000 / entry->total_download_time);
        else
            download_speed = 0;

        if (entry->total_upload_time > 0)
            upload_speed = (entry->total_upload_data * 1000000 / entry->total_upload_time);
        else
            upload_speed = 0;

        printf ("%-35s%-35s%-20d%-30d%-20d%-30d\n", entry->host1.c_str(), entry->host2.c_str(), entry->total_uploads, upload_speed, entry->total_downloads, download_speed);
    }
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
    else if (args[0] == "list")
    {
        print_server_ip_list();
    }
    else if (args[0] == "terminate")
    {
        printf("TERMINATE command is not applicable for server\n");
    }
    else if (args[0] == "exit")
    {
        printf("EXIT command is not applicable for server\n");
    }
    else if (args[0] == "upload")
    {
        printf("UPLOAD command is not applicable for server\n");
    }
    else if (args[0] == "download")
    {
        printf("DOWNLOAD command is not applicable for server\n");
    }
    else if (args[0] == "statistics")
    {
        fetch_stats();
    }
    else
    {
        std::cout << "Unknown command " << args[0] << " entered\n";
    }
}