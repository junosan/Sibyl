#include <iomanip>

#include <sibyl/server/Simulation/Simulation_dep.h>
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
    if (0 != simulation.LoadData(argv[1], argv[2]))
        exit(1);
    
    SimulationServer server(&simulation);
    server.SetVerbose(argc == 5 && std::string(argv[4]) == "-v");
    server.Launch(argv[3], true, false);
    
    std::cout << std::setprecision(6) << std::fixed
              << simulation.orderbook.GetProfitRate() << std::endl;
    
    return 0;
}