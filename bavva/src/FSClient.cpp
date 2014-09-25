#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <unistd.h>
#include <cstring>
#include <algorithm>

#include "../include/FSClient.h"

// class implementation
FSClient::FSClient(int port):FSNode(port){}
FSClient::~FSClient(){}

void FSClient::process_command(std::string& command)
{
    if (command == "creator")
    {
        printf("Bharadwaj Avva\nbavva\nbavva@buffalo.edu\n");
        printf("I have read and understood the course academic integrity policy located at http://www.cse.buffalo.edu/faculty/dimitrio/courses/cse4589_f14/index.html#integrity\n");
    }
    else if (command == "help")
    {
        printf("Following commands are available to use\n"
                "1. CREATOR: Displays full name, UBIT name and UB email address of creator\n"
                "2. HELP: Displays this help information\n"
                "3. MYIP: Displays the IP address of this process\n"
                "4. MYPORT: Displays the port on which this process is listening for incoming connections\n");
    }
    else if (command == "myip")
    {
        printf("IP address:%s\n", local_ip.c_str());
    }
    else if (command == "myport")
    {
        printf("Port number:%d\n", port);
    }
    else
    {
        std::cout << "Unknown command " << command << " entered\n";
    }
}
