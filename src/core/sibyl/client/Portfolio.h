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

#ifndef SIBYL_CLIENT_PORTFOLIO_H_
#define SIBYL_CLIENT_PORTFOLIO_H_

#include <vector>
#include <fstream>

#include "../Security.h"
#include "../Catalog.h"
#include "ItemState.h"

namespace sibyl
{

class OrderPf : public PQ
{
public:
    OrdType type;
    OrderPf() : type(OrdType::null) {}
};

class ItemPf : public Security<OrderPf> // abstract class; derive as stock, ELW, ETF, option, etc.
{
public:
    virtual ~ItemPf() {}
};

class Portfolio : public Catalog<ItemPf>
{
public:
    // to be used by rnn client
    const std::vector<ItemState>& GetStateVec();
    
    // to be called by Trader
    void SetStateLogPaths(CSTR &state, CSTR &log);
    int  ApplyMsgIn      (char *msg); // this destroys msg during parsing; returns non-0 to signal termination
private:
    std::vector<ItemState> vecState;
    
    STR pathState;
    STR pathLog;
    
    void WriteState(); // writes only if state path was set
    
    std::ofstream logMsgIn;  // full state msg
    std::ofstream logVecOut; // t, pr, qr, tbr
    
    char bufLine[1 << 10];
};

}

#endif  /* SIBYL_CLIENT_PORTFOLIO_H_ */