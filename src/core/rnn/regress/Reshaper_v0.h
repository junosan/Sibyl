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

#ifndef SIBYL_RESHAPER_V0_H_
#define SIBYL_RESHAPER_V0_H_

#include "../Reshaper.h"

#include <array>
#include <vector>
#include <map>

namespace sibyl
{

class Reshaper_v0 : public Reshaper
{
public:
    Reshaper_v0(unsigned long maxGTck_, // this will be ignored and overwritten
                TradeDataSet *pTradeDataSet_,
                std::vector<std::string> *pFileList_,
                const unsigned long (TradeDataSet::* ReadRawFile_)(std::vector<FLOAT>&, CSTR&));

    void ReadConfig(CSTR &filename) override;

    /*   raw   -> fractal */
    /*  sibyl  -> fractal */
    void State2VecIn(FLOAT *vec, const ItemState &state) override;
    
    /*   ref   -> fractal */
    void Reward2VecOut(FLOAT *vec, const Reward &reward, CSTR &code) override;
    
    /* fractal ->  sibyl  */
    void VecOut2Reward(Reward &reward, const FLOAT *vec, CSTR &code) override;
    
private:
    double b_th, s_th;

    struct ItemMem {
        FLOAT initPr;
        std::array<PQ, idx::szTb> lastTb;
        std::vector<double> idleG;
        std::size_t cursor; // points at idx for current time in idleG
        ItemMem() : initPr(0.0f), lastTb{}, cursor(0) {}
    };
    std::map<STR, ItemMem> items;
};

}

#endif /* SIBYL_RESHAPER_V0_H_ */