#include <iostream>
#include <cstdio>
#include <cerrno>

#include "sibyl/client/Trader.h"
#include "sibyl/client/NetClient.h"

int main (int argc, char *argv[]) {
    if (((argc != 4) && (argc != 5)) || ((argc == 5) && (std::string(argv[4]) != "-v")))
    {
        std::cerr << "USAGE: refclnt <ipaddress> <port> <datagpath> [-v]\n   -v\tVerbose output" << std::endl;
        return 0;
    }

    std::string path(argv[0]);
    path.resize(path.find_last_of('/'));

    using namespace sibyl;
    
    Trader trader;
    trader.Initialize(TimeBounds(       -3600 /* ref       */,
                                 kTimeTickSec /* init      */,
                                        21000 /* stop      */,
                                        22200 /* end       */));    
    trader.model.SetParams(              60.0 /* timeConst */,
                                         30.0 /* rhoWeight */,
                                        0.001 /* rhoInit   */);
    trader.model.SetRefPath(argv[3]);
    trader.SetStateLogPaths(path + "/state", "");

    NetClient netclient(&trader);
    netclient.SetVerbose((argc == 5) && (std::string(argv[4]) == "-v"));

    if (0 != netclient.Connect(argv[1], argv[2]))
        exit(1);
        
    while (true)
    {
        if (0 != netclient.RecvNextTick()) break;
        trader.model.GetRefData();
        netclient.SendResponse();
    }
    
    return 0;
}
