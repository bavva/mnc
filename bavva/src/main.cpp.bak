#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <algorithm>

/* global options */
bool appIsServer = false;
long appPort = 0;

/* forward declarations */
void printHelpAndExit(void);
void printHelp(void);
void printCreatorInfo(void);

/* constant variables */
const int expectedParamCount = 2;

/* functionalities */
void printUsageAndExit(void)
{
    printf("Usage: assignment1 <s|c> <port>\n");
    exit(0);
}

void printHelp(void)
{
    printf("Following commands are available to use\n"
           "1. CREATOR: Displays full name, UBIT name and UB email address of creator\n"
           "2. HELP: Displays this help information\n"
           "3. MYIP: Displays the IP address of this process\n"
           "4. MYPORT: Displays the port on which this process is listening for incoming connections\n"
    );
}

void printCreatorInfo(void)
{
    printf("Bharadwaj Avva\nbavva\nbavva@buffalo.edu\n");
    printf("I have read and understood the course academic integrity policy located at http://www.cse.buffalo.edu/faculty/dimitrio/courses/cse4589_f14/index.html#integrity\n");
}

int main(int argc, char **argv)
{
    if (argc != expectedParamCount + 1)
        printUsageAndExit();

    // first parameter validation. can be either s or c
    std::string appMode(argv[1]);
    if (appMode == "s" || appMode == "S")
        appIsServer = true;
    else if (appMode == "c" || appMode == "C")
        appIsServer = false;
    else
        printUsageAndExit();

    // second parameter validation. should be a port number
    appPort = atol(argv[2]);
    if (appPort <= 0 || appPort > 65535)
        printUsageAndExit();

    // start infinite loop to process commands
    std::string command;
    while(std::cin >> command)
    {
        // convert to lower case to compare
        std::transform(command.begin(), command.end(), command.begin(), tolower);

        if (command == "creator")
            printCreatorInfo();
        else if (command == "help")
            printHelp();
    }

    return 0;
}
