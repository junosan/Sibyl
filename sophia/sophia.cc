/* ========================================================================== */
/*  Copyright (C) 2016 Hosang Yoon (hosangy@gmail.com) - All Rights Reserved  */
/*  Unauthorized copying of this file, via any medium is strictly prohibited  */
/*                        Proprietary and confidential                        */
/* ========================================================================== */

#include <zmq.hpp>

#include <sibyl/client/Trader.h>
#include <sibyl/client/NetClient.h>

#include <rnn/value/ValueDataSet.h>
using Data = ValueDataSet;

#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <cassert>
#include <cmath>

int main(int argc, char *argv[])
{
    if ( (argc != 6 && argc != 7)                   ||
         (argc == 7 && std::string(argv[6]) != "-v") )
    {
        std::cerr << "USAGE: sophia <model cfg> <reshaper cfg> <workspace list> <ip address> <port> [-v]\n";
        exit(1);
    }

    std::string path(argv[0]);
    path.resize(path.find_last_of('/'));


    /* ================================= */
    /*            Setup Sibyl            */
    /* ================================= */

    using namespace sibyl;
    
    Trader trader;
    trader.model.ReadConfig(argv[1]);
    trader.SetStateLogPaths(path + "/state", path + "/log");

    NetClient netClient(&trader);
    netClient.SetVerbose(argc == 7 && std::string(argv[6]) == "-v");
    
    std::string workspaces; // ; delimited 
    std::vector<Data> datas;

    {
        std::ifstream pathList(argv[3]);
        if (pathList.is_open() == false)
        {
            std::cerr << "<workspace list> inaccessible at " << argv[3] << '\n';
            exit(1);
        }
        for (std::string workspace; std::getline(pathList, workspace);)
        {
            if (workspace.empty() == true) continue;
            if (workspace[0] != '/') workspace = path + "/" + workspace;
            datas.emplace_back();
            datas.back().Reshaper().ReadConfig(argv[2]);
            verify(true == datas.back().Reshaper().ReadWhiteningMatrix
                (workspace + "/mean.matrix", workspace + "/whitening.matrix"));
            workspaces += workspace + ';';
        }
    }
    const auto nNet = datas.size();
    verify(nNet > 0);
    
    const unsigned long inputDim  = datas[0].Reshaper().GetInputDim ();
    const unsigned long targetDim = datas[0].Reshaper().GetTargetDim();
    
    // To be initialized at first RecvNextTick
    unsigned long nStream(0);
    unsigned long inputStride(0);
    unsigned long targetStride(0);
    std::vector<FLOAT> vecIn;
    std::vector<FLOAT> vecOut;


    /* ================================= */
    /*       Setup IPC with Sophia       */
    /* ================================= */

    zmq::context_t context(1);
    zmq::socket_t socket(context, ZMQ_REQ);
    socket.connect("ipc:///tmp/sophia_sibyl");


    /* ================================= */
    /*                Run                */
    /* ================================= */

    bool is_init = true;
    bool is_done = false;

    // Connect to server
    if (0 != netClient.Connect(argv[4], argv[5])) exit(1);

    // Network main loop
    while (true)
    {
        // Receive data and fill Portfolio entries
        if (0 != netClient.RecvNextTick()) break;

        // Initialize nStream
        if(is_init == true)
        {
            nStream  = trader.portfolio.items.size();
            vecIn .resize(nNet * nStream * inputDim);
            inputStride  = nStream * inputDim;
            targetStride = nStream * targetDim;

            // Inform Sophia of workspaces & nStream (== batch_size in Sophia)
            workspaces += std::to_string(nStream);
            const auto bytes = workspaces.size();
            zmq::message_t beg(bytes);
            memcpy(beg.data(), workspaces.data(), bytes);
            socket.send(beg);

            // Need to receive once before sending another
            zmq::message_t rcv;
            socket.recv(&rcv);

            is_init = false;
        }
        
        // Rnn is run only between 09:00:int and 14:50:00
        if ( trader.portfolio.time >= kTimeBounds::init &&
             trader.portfolio.time <  kTimeBounds::stop ) 
        {
            // Retrieve ItemState vector for current frame
            const auto &vecState = trader.portfolio.GetStateVec();

            // Convert ItemState to input vec (flattened 3-dim array)
            for (auto n = 0u; n < nNet; ++n) {
                for (auto b = 0u; b < nStream; ++b)
                    datas[n].Reshaper().State2VecIn(vecIn.data() + n * inputStride + b * inputDim, 
                                                    vecState[b]);
            }

            // // debug
            // std::cerr << "Sending : ["
            //           << vecIn[0] << ", "
            //           << vecIn[1] << ", "
            //           << vecIn[2] << ", "
            //           << vecIn[3] << "]\n";
            // Invoke Sophia::Ensemble::run_one_step
            {
                auto bytes = vecIn.size() * sizeof(FLOAT);
                zmq::message_t msg(bytes);
                memcpy(msg.data(), vecIn.data(), bytes);
                socket.send(msg);

                zmq::message_t rcv;
                socket.recv(&rcv);

                const auto numel = nNet * nStream * targetDim;
                verify(numel * sizeof(FLOAT) == rcv.size());

                FLOAT* ptr = static_cast<FLOAT*>(rcv.data());
                vecOut.assign(ptr, ptr + numel);
            }
            // // debug
            // std::cerr << "Received: ["
            //           << vecOut[0] << ", "
            //           << vecOut[1] << ", "
            //           << vecOut[2] << ", "
            //           << vecOut[3] << "]\n";
            
            // Allocate 0-filled Reward vector
            auto &vecReward = trader.model.GetRewardVec(); 

            // Convert target vec (flattened 3-dim array) to Reward
            for (auto n = 0u; n < nNet; ++n) {
                for (auto b = 0u; b < nStream; ++b) {
                    Reward temp;
                    datas[n].Reshaper().VecOut2Reward(temp,
                                                      vecOut.data() + n * targetStride + b * targetDim,
                                                      vecState[b].code);
                    vecReward[b] += temp;
                }
            } 
            for (auto &reward : vecReward) reward *= (FLOAT) (1.0 / nNet);
            
            // Send rewards vector back to model
            trader.model.SetRewardVec(vecReward); 
        }

        if (trader.portfolio.time >= kTimeBounds::stop && is_done == false)
        {
            // Tell Sophia we're done
            FLOAT nan = std::nanf("");
            zmq::message_t end(sizeof(FLOAT));
            memcpy(end.data(), &nan, sizeof(FLOAT));
            socket.send(end);
            is_done = true;
        }
        
        // Calculate based on vecState/vecReward and send requests
        netClient.SendResponse();
    }

    return 0;
}

