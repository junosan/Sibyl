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

#include <string>
#include <iostream>
#include <fstream>
#include <memory>
#include <cassert>

#include <fractal/fractal.h>

#include <sibyl/client/Trader.h>
#include <sibyl/client/NetClient.h>

#include <rnn/regress/Reshaper_v0.h>
#include <rnn/regress/VanillaNet.h>
using Net = fractal::VanillaNet<sibyl::Reshaper_v0>;

int main(int argc, char *argv[])
{
    if ( (argc != 6 && argc != 7)                   ||
         (argc == 7 && std::string(argv[6]) != "-v") )
    {
        std::cerr << "USAGE: rnnclnt <model cfg> <reshaper cfg> <workspace list> <ip address> <port> [ -v ]" << std::endl;
        exit(1);
    }

    std::string path(argv[0]);
    path.resize(path.find_last_of('/'));


    /* ============================================= */
    /*                  Setup sibyl                  */
    /* ============================================= */
    
    verify(system(std::string("mkdir -p " + path + "/log").c_str()) == 0);
    verify(system(std::string("mkdir -p " + path + "/state").c_str()) == 0);

    using namespace sibyl;
    
    Trader trader;
    trader.model.ReadConfig(argv[1]);    
    trader.SetStateLogPaths(path + "/state", path + "/log");

    NetClient netClient(&trader);
    netClient.SetVerbose((argc == 7) && (std::string(argv[6]) == "-v"));
    
    
    /* =============================================== */
    /*                  Setup fractal                  */
    /* =============================================== */
    
    using namespace fractal;
    
    Engine engine;
    
    std::vector<std::unique_ptr<Net>> vecNet;
    std::ifstream pathList(argv[3]);
    if (pathList.is_open() == false)
    {
        std::cerr << "<workspace list> inaccessible at " << argv[3] << std::endl;
        exit(1);
    }
    for (std::string workspace; std::getline(pathList, workspace);)
    {
        if (workspace.empty() == true) continue;
        if (workspace[0] != '/') workspace = path + "/" + workspace;
        vecNet.push_back(std::unique_ptr<Net>(new Net()));
        vecNet.back()->Reshaper().ReadConfig(argv[2]);
        vecNet.back()->Configure(engine, Net::RunType::network, "", workspace);
    }
    std::size_t nNet = vecNet.size();
    verify(nNet > 0);
    
    unsigned long inputDim  = vecNet[0]->Reshaper().GetInputDim ();
    unsigned long outputDim = vecNet[0]->Reshaper().GetTargetDim();
    
    unsigned long nUnroll = 2;
    unsigned long nStream = 0; // will be set below
    
    bool is_init = true;

    
    /* ===================================== */
    /*                  Run                  */
    /* ===================================== */

    /* Connect to server */
    if (0 != netClient.Connect(argv[4], argv[5])) exit(1);

    /* Network main loop */
    while (true)
    {
        /* Receive data and fill Portfolio entries */
        if (0 != netClient.RecvNextTick()) break;
        
        /* Initialize nUnroll & nStream */
        if(is_init == true)
        {
            nStream = trader.portfolio.items.size();
            for (auto &pNet : vecNet)
                pNet->InitUnrollStream(nUnroll, nStream);
            is_init = false;
        }
        
        // Rnn is run only between 09:00:int and 14:50:00
        if ( (trader.portfolio.time >= kTimeBounds::init) &&
             (trader.portfolio.time <  kTimeBounds::stop) ) 
        {
            /* Retrieve state vector for current frame */
            const auto &vecState = trader.portfolio.GetStateVec();

            /* Generate the input matrix */
            for (auto &pNet : vecNet)
            {
                FLOAT *vecIn = pNet->GetInputVec();
                for (std::size_t codeIdx = 0; codeIdx < nStream; codeIdx++)
                    pNet->Reshaper().State2VecIn(vecIn + codeIdx * inputDim, vecState[codeIdx]);
            }

            /* Run RNN */
            for (auto &pNet : vecNet)
                pNet->RunOneFrame();

            /* Allocate 0-filled rewards vector */
            auto &vecReward = trader.model.GetRewardVec(); 

            /* Get gain values from the output matrix */
            for (auto &pNet : vecNet)
            {
                FLOAT *vecOut = pNet->GetOutputVec();
                for (std::size_t codeIdx = 0; codeIdx < nStream; codeIdx++)
                {
                    Reward temp;
                    pNet->Reshaper().VecOut2Reward(temp, vecOut + codeIdx * outputDim, vecState[codeIdx].code);
                    vecReward[codeIdx] += temp;
                }
            }
            for (auto &reward : vecReward) reward *= (FLOAT) 1 / nNet;
            
            /* Send rewards vector back to model */
            trader.model.SetRewardVec(vecReward); 
        }
        
        /* Calculate based on vecState/vecReward and send requests */
        netClient.SendResponse();
    }

    return 0;
}

