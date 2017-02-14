/*
   Copyright 2017 Hosang Yoon

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

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