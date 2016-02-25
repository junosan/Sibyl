#include <iostream>
#include <cstdio>
#include <cerrno>

#include "sibyl/Portfolio.h"
#include "sibyl/NetClient.h"

int main (int argc, char** argv) {
    if (((argc != 4) && (argc != 5)) || ((argc == 5) && (std::string(argv[4]) != "-v")))
    {
        printf("USAGE: refclnt <ipaddress> <port> <datagpath> [-v]\n   -v\tVerbose output\n");
        return 0;
    }

    using namespace sibyl;
    Portfolio p;
    NetClient nc;
    
    nc.SetPortfolio(&p);
    nc.SetVerbose((argc == 5) && (std::string(argv[4]) == "-v"));
    
    const int timeRef  = -3600;        // (08:00:00) last ref price reference
    const int timeInit = kTimeTickSec; // (09:00:10) start input to rnn
    const int timeStop = 21000;        // (14:50:00) stop input to rnn
    const int timeEnd  = 22200;        // (15:10:00) terminate
    p.SetTimeBoundaries(timeRef, timeInit, timeStop, timeEnd);
    
    const double timeConst = 60.0;
    const double rhoWeight = 30.0;
    const double rhoInit   = 0.001;
    p.SetParams(timeConst, rhoWeight, rhoInit);

    std::string pathState(argv[0]), pathLog(argv[0]);
    auto pos  = pathState.find_last_of('/');
    pathState = pathState.substr(0, pos) + std::string("/state/");
    pathLog   = pathLog  .substr(0, pos) + std::string("/log/");
    p.SetStateLogPaths(pathState, "");

    // For .ref files
    std::map<std::string, std::unique_ptr<FILE, int(*)(FILE*)>> mfRef;
    
    std::string pathData(argv[3]);
    if ('/' != pathData[pathData.size() - 1]) pathData.append("/");
    
    int ret = nc.Connect(std::string(argv[1]), std::string(argv[2]));
    if (ret != 0) return ret;
    
    bool isFirstMsg = true;
    while (true)
    {
        ret = nc.RecvNextTick();
        if (ret != 0) break;
        
        // Load .ref files on first message
        if (isFirstMsg)
        {
            for (const auto &code_pItem : p.map)
            {
                std::string filename = pathData + code_pItem.first + std::string(".ref");
                FILE *pf = fopen(filename.c_str(), "rb");
                if (pf == nullptr)
                {
                    fprintf(stderr, "%s not found\n", filename.c_str());
                    return -1;
                }
                auto it_bool = mfRef.insert(std::make_pair(code_pItem.first, std::unique_ptr<FILE, int(*)(FILE*)>(pf, fclose)));
                assert(it_bool.second == true);
            }
            isFirstMsg = false;
        }
        
        // Read .ref files
        if ((p.time >= timeInit) && (p.time < timeStop))
        {
            const std::ptrdiff_t dimRef = 42;
            float bufRef[dimRef];

            for (const auto &code_pItem : p.map)
            {
                auto &i = *code_pItem.second;
                auto imf = mfRef.find(code_pItem.first);
                assert(imf != std::end(mfRef));
                std::size_t szRead = fread(bufRef, sizeof(float), dimRef, &*(imf->second));
                if (szRead == dimRef)
                {
                    i.G0.s = bufRef[0];
                    i.G0.b = bufRef[1];
                    std::ptrdiff_t tck = 0;
                    for (auto &gn : i.G)
                    {
                        gn.s  = bufRef[ 2 + tck  ];
                        gn.b  = bufRef[12 + tck  ]; 
                        gn.cs = bufRef[22 + tck  ];
                        gn.cb = bufRef[32 + tck++]; 
                    }
                }
                else
                    break;
                
                if (i.Type() == kSecStock)
                {
                    for (int iB = 0; iB < (int)dimRef; iB++)
                    {
                        if ((std::abs(bufRef[iB]) > 1.0f) && (std::abs(bufRef[iB]) < 100.0f))
                            fprintf(stderr, "Warning: [%5d] {%s} G = %f too large (iB %d) (ps1 %d, pb1 %d)\n", p.time, code_pItem.first.c_str(), bufRef[iB], iB, i.Tck2P(0, kOrdSell), i.Tck2P(0, kOrdBuy));
                    }
                }
            }
        }
        
        nc.SendResponse();   
    }

    return 0;
}
