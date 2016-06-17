/* ========================================================================== */
/*  Copyright (C) 2016 Hosang Yoon (hosangy@gmail.com) - All Rights Reserved  */
/*  Unauthorized copying of this file, via any medium is strictly prohibited  */
/*                        Proprietary and confidential                        */
/* ========================================================================== */

#include <iomanip>

#include <sibyl/server/Simulation/Simulation_dep.h>
#include <sibyl/server/NetServer.h>
#include <sibyl/util/OstreamRedirector.h>

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        std::cerr << "USAGE: simserv <config file> <data path> <port>" << std::endl;
        exit(1);
    }
    
    std::string path(argv[0]);
    path.resize(path.find_last_of('/'));
    
    using namespace sibyl;
    
    Simulation simulation;
    if (0 != simulation.LoadData(argv[1], argv[2]))
        exit(1);
    
    SimulationServer server(&simulation);
    server.SetVerbose(true);
    {
        OstreamRedirector redir(std::cout, path + "/log/sim.log");
        server.Launch(argv[3], true, false);
    }
    
    std::cout << std::setprecision(6) << std::fixed
              << simulation.orderbook.GetProfitRate() << std::endl;
    
    return 0;
}