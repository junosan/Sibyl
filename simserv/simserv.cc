#include <iomanip>

#include <sibyl/server/Simulation.h>
#include <sibyl/server/NetServer.h>

int main(int argc, char *argv[])
{
    if ( (argc != 4 && argc != 5)                   ||
         (argc == 5 && std::string(argv[4]) != "-v") )
    {
        std::cerr << "USAGE: simserv <config file> <data path> <port> [-v]\n   -v\tVerbose output" << std::endl;
        exit(1);
    }
    
    using namespace sibyl;
    
    Simulation simulation;
    simulation.Initialize(TimeBounds(       -3600 /*ref */,
                                     kTimeTickSec /*init*/,
                                            21000 /*stop*/,
                                            22200 /*end */));
    if (0 != simulation.LoadData(argv[1], argv[2]))
        exit(1);
    
    NetServer<OrderSim, ItemSim> netserver(&simulation);
    netserver.SetVerbose(argc == 5 && std::string(argv[4]) == "-v");
    netserver.Run(argv[3]);
    
    std::cout << std::setprecision(6) << std::fixed
              << simulation.orderbook.GetProfitRate() << std::endl;
    
    return 0;
}