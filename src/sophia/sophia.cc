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

#include <zmq.hpp>

#include <sibyl/client/Trader.h>
#include <sibyl/client/NetClient.h>

#include <rnn/regress/Reshaper_v0.h>
#include <rnn/regress/RegressDataSet.h>
using Data = RegressDataSet<sibyl::Reshaper_v0>;

#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <cassert>
#include <cmath>
#include <map>
#include <sstream>

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
    
    std::string info; // ; and \n delimited 
    std::vector<Data> datas;
    std::vector<std::map<std::string, int>> id_idx_maps;

    // Initialize from each workspace
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
            info += workspace + ';';

            datas.emplace_back();
            datas.back().Reshaper().ReadConfig(argv[2]);
            verify(true == datas.back().Reshaper().ReadWhiteningMatrix
                (workspace + "/mean.matrix", workspace + "/whitening.matrix"));
            
            // Build code -> idx_during_training map
            id_idx_maps.emplace_back();
            std::ifstream indice_file(workspace + "/ids.order");
            if (indice_file.is_open() == false)
            {
                std::cerr << "<ids.order> inaccessible at " << workspace << '\n';
                exit(1);
            }

            std::string indice;
            std::getline(indice_file, indice);

            int idx(0);
            std::istringstream iss(indice);
            for (std::string id; std::getline(iss, id, ';');)
            {
                bool success;
                std::tie(std::ignore, success) = id_idx_maps.back().emplace(id, idx++);
                verify(success == true);
            }
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
    /*  Setup communication with Sophia  */
    /* ================================= */

    zmq::context_t context(1);
    zmq::socket_t socket(context, ZMQ_REQ);

    // using IPC here, but also supports TCP if communicating over a network
    socket.connect("ipc:///tmp/sophia_ipc");


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

            // Inform Sophia of workspaces, nStream (== batch_size in Sophia), and id_indices
            info += std::to_string(nStream) + '\n';

            const auto &vecState = trader.portfolio.GetStateVec();
            for (auto n = 0u; n < nNet; ++n)
            {
                for (auto b = 0u; b < nStream; ++b)
                {
                    const auto it = id_idx_maps[n].find(vecState[b].code);
                    verify(it != std::end(id_idx_maps[n])); // crash on code unseed during training
                    info += std::to_string(it->second) + ';';
                }
                if (info.back() == ';') info.pop_back();
                if (n < nNet - 1) info += '\n';
            }

            const auto bytes = info.size();
            zmq::message_t beg(bytes);
            memcpy(beg.data(), info.data(), bytes);
            socket.send(beg);

            // Need to receive once before sending another (REQ/REP)
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

            // Send vecIn to Sophia and retrieve vecOut
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

