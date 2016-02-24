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
            for (auto iP = std::begin(p.p); iP != std::end(p.p); iP++)
            {
                std::string filename = pathData + iP->first + std::string(".ref");
                FILE *pf = fopen(filename.c_str(), "rb");
                if (pf == nullptr)
                {
                    fprintf(stderr, "%s not found\n", filename.c_str());
                    return -1;
                }
                auto pair = mfRef.insert(std::make_pair(iP->first, std::unique_ptr<FILE, int(*)(FILE*)>(pf, fclose)));
                assert(pair.second == true);
            }
            isFirstMsg = false;
        }
        
        // Read .ref files
        if ((p.time >= timeInit) && (p.time < timeStop))
        {
            const std::ptrdiff_t dimRef = 42;
            float bufRef[dimRef];

            for (auto iP = std::begin(p.p); iP != std::end(p.p); iP++)
            {
                auto &i = *(iP->second);
                auto imf = mfRef.find(iP->first);
                assert(imf != std::end(mfRef));
                std::size_t szRead = fread(bufRef, sizeof(float), dimRef, &*(imf->second));
                if (szRead == dimRef)
                {
                    i.G0.s = bufRef[0];
                    i.G0.b = bufRef[1];
                    for (auto iG = std::begin(i.G); iG != std::end(i.G); iG++)
                    {
                        auto &gn = *iG;
                        std::ptrdiff_t tck = iG - std::begin(i.G);
                        gn.s  = bufRef[ 2 + tck];
                        gn.b  = bufRef[12 + tck]; 
                        gn.cs = bufRef[22 + tck];
                        gn.cb = bufRef[32 + tck]; 
                    }
                }
                else
                    break;
                
                if (i.Type() == kSecStock)
                {
                    for (int iB = 0; iB < (int)dimRef; iB++)
                    {
                        if ((std::abs(bufRef[iB]) > 1.0f) && (std::abs(bufRef[iB]) < 100.0f))
                            fprintf(stderr, "Warning: [%5d] {%s} G = %f too large (iB %d) (ps1 %d, pb1 %d)\n", p.time, iP->first.c_str(), bufRef[iB], iB, i.Tck2P(0, kOrdSell), i.Tck2P(0, kOrdBuy));
                    }
                }
            }
        }
        
        nc.SendResponse();   
    }

    return 0;
}
