/**
 * @bavva_assignment1
 * @author  Bharadwaj Avva <bavva@buffalo.edu>
 * @version 1.0
 *
 * @section LICENSE
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details at
 * http://www.gnu.org/copyleft/gpl.html
 *
 * @section DESCRIPTION
 *
 * This contains the main function. Add further description here....
 */
#include <iostream>
#include <cstdlib>
#include <cstdio>

#include "../include/global.h"
#include "../include/FSServer.h"
#include "../include/FSClient.h"

using namespace std;

const int correctParamCount = 2;

void printUsageAndExit(void)
{
    printf("Usage: assignment1 <s|c> <port>\n");
    exit(-1);
}

int main(int argc, char **argv)
{
    bool appIsServer = false;
    int appPort = 0;

    if (argc != correctParamCount + 1)
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

    if (appIsServer)
    {
        FSServer *server = new FSServer(appPort);
        server->start();
        delete server;
    }
    else
    {
        FSClient *client = new FSClient(appPort);
        client->start();
        delete client;
    }

    return 0;
}
