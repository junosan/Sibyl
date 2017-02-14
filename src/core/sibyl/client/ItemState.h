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

#ifndef SIBYL_CLIENT_ITEMSTATE_H_
#define SIBYL_CLIENT_ITEMSTATE_H_

#include <array>

#include "../sibyl_common.h"
#include "../Security.h"

namespace sibyl
{

// Stores data in a flattened structure for communication with fractal
// Used by Portfolio and Reshaper::State2VecIn
class ItemState
{
public:
    // common
    STR   code;
    int   time;
    FLOAT pr;
    INT64 qr;
    std::array<PQ, idx::szTb> tbr;
    
    // ELW only
    bool isELW;
    int iCP; // +1 for call, -1 for put
    int expiry;
    FLOAT kospi200;
    std::array<FLOAT, ELW<Security<PQ>>::szTh> thr;
    
    // ETF only
    bool isETF;
    FLOAT devNAV;
    
    ItemState() : time(0), pr(0.0f), qr(0), tbr{},
                  isELW(false), iCP(0), expiry(0), kospi200(0.0f), thr{},
                  isETF(false), devNAV(0.0f)
                  {}
};

}

#endif /* SIBYL_CLIENT_ITEMSTATE_H_ */