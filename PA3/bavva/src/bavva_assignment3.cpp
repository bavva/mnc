/**
 * @bavva_assignment3
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
#include <stdio.h>
#include <stdlib.h>

#include "../include/global.h"
#include "../include/logger.h"
#include "../include/dvrouter.h"

using namespace std;

// global parameters
const int correctParamCount = 4;

void printUsageAndExit(void)
{
    printf("Usage: assignment3 -t <path-to-topology-file> -i <routing-update-interval>\n");
    exit(-1);
}

int main(int argc, char **argv)
{
    std::string flag1, flag2, topology;
    bool appIsServer = false;
    int appPort = 0, routing_timeout = 0;

    /* Init. Logger */
    cse4589_init_log();

    /* Clear LOGFILE and DUMPFILE */
    fclose(fopen(LOGFILE, "w"));
    fclose(fopen(DUMPFILE, "wb"));

    if (argc != correctParamCount + 1)
        printUsageAndExit();

    // flags can be either -t/-T or -i/-I
    flag1.assign(argv[1]);
    flag2.assign(argv[3]);

    if ((flag1 == "-t" || flag1 == "-T") && (flag2 == "-i" || flag2 == "-I"))
    {
        topology.assign(argv[2]);
        routing_timeout = atoi(argv[4]);
    }
    else if ((flag1 == "-i" || flag1 == "-I") && (flag2 == "-t" || flag2 == "-T"))
    {
        routing_timeout = atoi(argv[2]);
        topology.assign(argv[4]);
    }
    else
        printUsageAndExit();

    DVRouter *router = new DVRouter(topology, routing_timeout);
    router->start();
    delete router;

    return 0;
}
