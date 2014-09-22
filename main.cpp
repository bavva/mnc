#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <algorithm>

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
    std::string command;

    if (argc != expectedParamCount + 1)
        printUsageAndExit();

    // start infinite loop to process commands
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
