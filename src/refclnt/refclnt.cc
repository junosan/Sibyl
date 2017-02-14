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

#include <iostream>
#include <string>

#include <sibyl/client/Trader.h>
#include <sibyl/client/NetClient.h>

int main (int argc, char *argv[]) {
    if ( (argc != 5 && argc != 6)                   ||
         (argc == 6 && std::string(argv[5]) != "-v") )
    {
        std::cerr << "USAGE: refclnt <config file> <ref path> <ip address> <port> [-v]\n   -v\tVerbose output" << std::endl;
        exit(1);
    }

    std::string path(argv[0]);
    path.resize(path.find_last_of('/'));

    using namespace sibyl;
    
    Trader trader;
    trader.model.ReadConfig(argv[1]);
    trader.model.SetRefPath(argv[2]);
    trader.SetStateLogPaths(path + "/state", "");

    NetClient netclient(&trader);
    netclient.SetVerbose(argc == 6 && std::string(argv[5]) == "-v");

    if (0 != netclient.Connect(argv[3], argv[4]))
        exit(1);
        
    while (true)
    {
        if (0 != netclient.RecvNextTick()) break;
        trader.model.GetRefData();
        netclient.SendResponse();
    }
    
    return 0;
}
