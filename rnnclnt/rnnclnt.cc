/* ========================================================================== */
/*  Copyright (C) 2016 Hosang Yoon (hosangy@gmail.com) - All Rights Reserved  */
/*  Unauthorized copying of this file, via any medium is strictly prohibited  */
/*                        Proprietary and confidential                        */
/* ========================================================================== */

#include <string>
#include <iostream>
#include <fstream>
#include <memory>
#include <cassert>

#include <fractal/fractal.h>

#include <rnn/TradeRnn.h>

#include <sibyl/client/Trader.h>
#include <sibyl/client/NetClient.h>

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
    
    std::vector<std::unique_ptr<TradeRnn>> vecRnn;
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
        vecRnn.push_back(std::unique_ptr<TradeRnn>(new TradeRnn()));
        vecRnn.back()->Reshaper().ReadConfig(argv[2]);
        vecRnn.back()->Configure(engine, TradeRnn::RunType::network, "", workspace);
    }
    std::size_t nRnn = vecRnn.size();
    verify(nRnn > 0);
    
    unsigned long inputDim  = vecRnn[0]->Reshaper().GetInputDim ();
    unsigned long outputDim = vecRnn[0]->Reshaper().GetTargetDim();
    
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
            for (auto &pRnn : vecRnn)
                pRnn->InitUnrollStream(nUnroll, nStream);
            is_init = false;
        }
        
        // Rnn is run only between 09:00:int and 14:50:00
        if ( (trader.portfolio.time >= kTimeBounds::init) &&
             (trader.portfolio.time <  kTimeBounds::stop) ) 
        {
            /* Retrieve state vector for current frame */
            const auto &vecState = trader.portfolio.GetStateVec();

            /* Generate the input matrix */
            for (auto &pRnn : vecRnn)
            {
                FLOAT *vecIn = pRnn->GetInputVec();
                for (std::size_t codeIdx = 0; codeIdx < nStream; codeIdx++)
                    pRnn->Reshaper().State2VecIn(vecIn + codeIdx * inputDim, vecState[codeIdx]);
            }

            /* Run RNN */
            for (auto &pRnn : vecRnn)
                pRnn->RunOneFrame();

            /* Allocate 0-filled rewards vector */
            auto &vecReward = trader.model.GetRewardVec(); 

            /* Get gain values from the output matrix */
            for (auto &pRnn : vecRnn)
            {
                FLOAT *vecOut = pRnn->GetOutputVec();
                for (std::size_t codeIdx = 0; codeIdx < nStream; codeIdx++)
                {
                    Reward temp;
                    pRnn->Reshaper().VecOut2Reward(temp, vecOut + codeIdx * outputDim, vecState[codeIdx].code);
                    vecReward[codeIdx] += temp;
                }
            }
            for (auto &reward : vecReward) reward *= (FLOAT) 1 / nRnn;
            
            /* Send rewards vector back to model */
            trader.model.SetRewardVec(vecReward); 
        }
        
        /* Calculate based on vecState/vecReward and send requests */
        netClient.SendResponse();
    }

    return 0;
}

