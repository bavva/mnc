#include <stdio.h>
#include <stdlib.h>
#include <iostream>

/* forward declarations */
void printHelpAndExit(void);
void printHelp(void);
void printCreatorInfo(void);

/* constant variables */
const int expectedParamCount = 2;

/* functionalities */
void printHelpAndExit(void)
{
    printHelp();
    exit(0);
}

void printHelp(void)
{
    printf("Usage: assignment1 <s|c> <port>\n");
}

void printCreatorInfo(void)
{
    printf("Bharadwaj Avva\nbavva\nbavva@buffalo.edu\n");
    printf("I have read and understood the course academic integrity policy located at http://www.cse.buffalo.edu/faculty/dimitrio/courses/cse4589_f14/index.html#integrity\n");
}

int main(int argc, char **argv)
{
    if (argc != expectedParamCount + 1)
        printHelpAndExit();

    return 0;
}
