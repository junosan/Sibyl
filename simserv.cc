#include <iomanip>

#include "sibyl/server/Simulation.h"
#include "sibyl/server/NetServer.h"

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        std::cerr << "USAGE: simserv <port> <configfile> <datapath>" << std::endl;
        exit(1);
    }
    
    std::string path(argv[0]);
    path.resize(path.find_last_of('/'));
    
    using namespace sibyl;
    Simulation simulation;
    simulation.Initialize(TimeBounds(       -3600 /*ref */,
                                     kTimeTickSec /*init*/,
                                            21000 /*stop*/,
                                            22200 /*end */));
    if (0 != simulation.LoadData(path + "/simserv.config", argv[2]))
        exit(1);
    
    NetServer<OrderSim, ItemSim> netserver(&simulation);
    netserver.Run(argv[1]);
    
    std::cout << std::setprecision(6) << std::fixed
              << simulation.orderbook.GetProfitRate() << std::endl;
    
    return 0;
}