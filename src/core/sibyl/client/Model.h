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

#ifndef SIBYL_CLIENT_MODEL_H_
#define SIBYL_CLIENT_MODEL_H_

#include "Portfolio.h"

namespace sibyl
{

// Defines interface for use by Trader
// Base class for RewardModel
class Model
{
public:
    // Build list of requests based on pPortfolio and internal model
    virtual CSTR& BuildMsgOut() = 0;

    // Called by Trader
    virtual void SetStateLogPaths(CSTR &state, CSTR &log) = 0;

    void SetPortfolio(Portfolio *pPortfolio_) { pPortfolio = pPortfolio_; }

    Model() : pPortfolio(nullptr) {} 
protected:
    Portfolio *pPortfolio;
};

}

#endif /* SIBYL_CLIENT_MODEL_H_ */