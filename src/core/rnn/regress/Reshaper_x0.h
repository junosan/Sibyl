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

#ifndef SIBYL_RESHAPER_X0_H_
#define SIBYL_RESHAPER_X0_H_

#include "../Reshaper.h"

#include <array>
#include <vector>
#include <map>

namespace sibyl
{

class Reshaper_x0 : public Reshaper
{
public:
    Reshaper_x0(unsigned long maxGTck_, // this will be ignored and overwritten
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

    constexpr static FLOAT c = 6.; // time constant multiplier
    constexpr static std::size_t nTimeConsts = 5;
    struct EMA {
        FLOAT prev;
        FLOAT alpha;

        EMA(FLOAT timeConst) : prev(0.), alpha(1. / timeConst) {}
        FLOAT Acc(FLOAT i) {
            prev = (1. - alpha) * prev + alpha * i;
            return prev;
        }
    };

    struct ItemMem {
        FLOAT initPr, lastPr;
        std::array<PQ, idx::szTb> lastTb;
        std::vector<double> idleG;
        std::size_t cursor; // points at idx for current time in idleG
        EMA aQr;
        std::array<EMA, nTimeConsts> aPr, aTR, aU, aD, aDX;
        std::array<EMA, idx::szTb> aTb;

        void Init(FLOAT p = 0.f, FLOAT tr = 0.f) {
            initPr = lastPr = p;
            lastTb = {};
            idleG.clear();
            cursor = 0;
            aQr.prev = 0.f;
            for (auto &a : aPr) a.prev = p;
            for (auto &a : aTR) a.prev = tr;
            for (auto &a : aU ) a.prev = 0.f;
            for (auto &a : aD ) a.prev = 0.f;
            for (auto &a : aDX) a.prev = 0.f;
            for (auto &a : aTb) a.prev = 0.f;
        }
        ItemMem() : aQr{c},
                    aPr{1 * c, 2 * c, 4 * c, 8 * c, 16 * c},
                    aTR{1 * c, 2 * c, 4 * c, 8 * c, 16 * c},
                    aU {1 * c, 2 * c, 4 * c, 8 * c, 16 * c},
                    aD {1 * c, 2 * c, 4 * c, 8 * c, 16 * c},
                    aDX{1 * c, 2 * c, 4 * c, 8 * c, 16 * c},
                    aTb{c, c, c, c, c, c, c, c, c, c,
                        c, c, c, c, c, c, c, c, c, c} { Init(); }
    };
    std::map<STR, ItemMem> items;
};

}

#endif /* SIBYL_RESHAPER_X0_H_ */