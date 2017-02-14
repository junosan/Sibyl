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

#ifndef SIBYL_CLIENT_TRADER_H_
#define SIBYL_CLIENT_TRADER_H_

#include "Portfolio.h"
#include "RewardModel.h"

namespace sibyl
{

// Mediates interfaces for NetClient, Portfolio, Model
// May be expanded if multiple Models are present
class Trader
{
public:
    Portfolio   portfolio;
    RewardModel model;

    void SetStateLogPaths(CSTR &state, CSTR &log) {
        portfolio.SetStateLogPaths(state, log);
        model    .SetStateLogPaths(state, log);
    }
    
    // called by NetClient
    int   ApplyMsgIn (char *msg) { return portfolio.ApplyMsgIn(msg); }
    CSTR& BuildMsgOut()          { return model.BuildMsgOut();       };

    Trader() { model.SetPortfolio(&portfolio); }
};

}

#endif /* SIBYL_CLIENT_TRADER_H_ */