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

#include "Reshaper_x0.h"

#include <sibyl/Security.h>
#include <sibyl/time_common.h>
#include <sibyl/util/Config.h>

namespace sibyl
{

Reshaper_x0::Reshaper_x0(unsigned long maxGTck_,
                        TradeDataSet *pTradeDataSet_,
                        std::vector<std::string> *pFileList_,
                        const unsigned long (TradeDataSet::* ReadRawFile_)(std::vector<FLOAT>&, CSTR&))
                        : Reshaper(maxGTck_, pTradeDataSet_, pFileList_, ReadRawFile_),
                          b_th(1.0), s_th(1.0)
{
    maxGTck   = 0; // overwrite Reshaper's constructor value 
    inputDim  = 45;
    targetDim = 1; // overwrite Reshaper's constructor value
    fullWhitening = false; // overwrite Reshaper's constructor value
}

void Reshaper_x0::ReadConfig(CSTR &filename)
{
    Config cfg(filename);
    
    auto &ss_b_th = cfg.Get("B_TH");
    ss_b_th >> b_th;
    verify(ss_b_th.fail() == false);
    
    auto &ss_s_th = cfg.Get("S_TH");
    ss_s_th >> s_th;
    verify(ss_s_th.fail() == false);
}

void Reshaper_x0::State2VecIn(FLOAT *vec, const ItemState &state)
{
    constexpr long interval = kTimeRates::secPerTick;
    constexpr long T = (long) (std::ceil((double) sibyl::kTimeBounds::stop / interval) - 1);
    
    auto iItems = items.find(state.code);
    if (iItems == std::end(items))
    {
        auto it_bool = items.insert(std::make_pair(state.code, ItemMem()));
        verify(it_bool.second == true);
        iItems = it_bool.first;
    }
    auto &i = iItems->second; // reference to current ItemMem
    
    KOSPI<Security<PQ>> sec;
    INT   ps1   = state.tbr[idx::ps1].p;
    FLOAT minTR = (ps1 > 0 ? (ps1 - sec.TckLo(ps1)) / 10.f : .1f); // epsilon

    if (1 == state.time / kTimeRates::secPerTick)
        i.Init(ps1, minTR); // also clears i.idleG

    // store idleG
    double s0f  = sec.TckLo(state.tbr[idx::ps1].p) * (1.0 - sec.dSF());
    double b0f  =           state.tbr[idx::ps1].p  * (1.0 + sec.dBF());
    double idleG = (s0f - b0f) / (s0f + b0f); // note: negative value
    verify(idleG < 0.0);
    i.idleG.push_back(idleG);
    i.cursor = i.idleG.size() - 1; // advance time tick for VecOut2Reward
    // verify((int) i.idleG.size() == state.time / kTimeRates::secPerTick); // for debugging training

    unsigned long idxInput = 0;
    
    // t
    vec[idxInput++] = (FLOAT) state.time / (interval * T);
    
    // pr
    vec[idxInput++] = ReshapePrice(state.pr) - ReshapePrice(i.initPr);
    
    // EMA(qr)
    vec[idxInput++] = ReshapeQuant(i.aQr.Acc(state.qr));
    
    // ps1
    vec[idxInput++] = ReshapePrice(state.tbr[idx::ps1].p) - ReshapePrice(i.initPr);
    
    // ps1 - pb1
    vec[idxInput++] = ReshapePrice(state.tbr[idx::ps1].p) - ReshapePrice(state.tbr[idx::pb1].p);
    
    // EMA(pr) (akin to https://en.wikipedia.org/wiki/MACD)
    for (auto &a : i.aPr)
        vec[idxInput++] = ReshapePrice(a.Acc(state.pr)) - ReshapePrice(i.initPr);

    // EMA(U, D, DX) (https://en.wikipedia.org/wiki/Average_directional_movement_index)
    for (auto idx = 0u; idx < nTimeConsts; ++idx)
    {
        auto u = (state.pr > i.lastPr ? state.pr - i.lastPr : 0.f);
        auto d = (i.lastPr > state.pr ? i.lastPr - state.pr : 0.f);
        auto aTR = i.aTR[idx].Acc(std::max(std::abs(state.pr - i.lastPr), minTR));

        auto aU  = i.aU [idx].Acc(u / aTR);
        auto aD  = i.aD [idx].Acc(d / aTR);
        auto aDX = i.aDX[idx].Acc(aU + aD > 0.f ? std::abs(aU - aD) / (aU + aD) : 0.f);

        vec[idxInput++] = (FLOAT) aU;
        vec[idxInput++] = (FLOAT) aD;
        vec[idxInput++] = (FLOAT) aDX;
    }
    i.lastPr = state.pr;

    // // tbpr(1:20)
    // for (std::size_t idx = 0; idx < (std::size_t)idx::szTb; idx++)
    //     vec[idxInput++] = ReshapePrice(state.tbr[idx].p) - ReshapePrice(i.initPr);
    
    // // tbqr(1:20)
    // for (std::size_t idx = 0; idx < (std::size_t)idx::szTb; idx++)
    //     vec[idxInput++] = ReshapeQuant(state.tbr[idx].q);
    
    // EMA(delta_tbqr(1:20))
    for (std::size_t idx = 0; idx < (std::size_t)idx::szTb; idx++)
    {
        INT delta = state.tbr[idx].q;
        for (std::size_t idxL = 0; idxL < (std::size_t)idx::szTb; idxL++)
        {
            if (state.tbr[idx].p == i.lastTb[idxL].p)
            {
                if ( (idx <= idx::ps1 && idxL <= idx::ps1) || 
                     (idx >= idx::pb1 && idxL >= idx::pb1) )
                    delta = state.tbr[idx].q - i.lastTb[idxL].q;
                else
                    delta = state.tbr[idx].q + i.lastTb[idxL].q;
                break;
            }
        }
        vec[idxInput++] = ReshapeQuant(i.aTb[idx].Acc(delta));
    }
    i.lastTb = state.tbr;
    
    verify(inputDim == idxInput);
    
    WhitenVector(vec); // this alters vector only if matrices are initialized
}

void Reshaper_x0::Reward2VecOut(FLOAT *vec, const Reward &reward, CSTR &code)
{
    auto iItems = items.find(code);
    verify(iItems != std::end(items));
    auto &i = iItems->second; // reference to current ItemMem
    
    if (vec == nullptr) // rewind idleG's cursor
    {
        i.cursor = 0;
        return;
    }
    
    verify(i.cursor < i.idleG.size());
    double idleG = i.idleG[i.cursor++]; // time tick advanced by repeated calls to this function
    
    // G' = (G - idle) / (2 * -idle) = 0.5 - G / (2 * idle)
    double G0s_scaled = 0.5 - reward.G0.s / (2.0 * idleG);
    double G0b_scaled = 0.5 - reward.G0.b / (2.0 * idleG);
    
    unsigned long idxTarget = 0;
    
    vec[idxTarget++] = (FLOAT) (G0b_scaled * (G0b_scaled > 0.0) - G0s_scaled * (G0s_scaled > 0.0));
    
    verify(targetDim == idxTarget);
}

void Reshaper_x0::VecOut2Reward(Reward &reward, const FLOAT *vec, CSTR &code)
{
    const auto iItems = items.find(code);
    verify(iItems != std::end(items));
    const auto &i = iItems->second; // reference to current ItemMem
    
    verify(i.cursor < i.idleG.size());
    double idleG = i.idleG[i.cursor]; // time tick advanced by State2VecIn
    
    unsigned long idxTarget = 0;
    
    double G_scaled = (double) vec[idxTarget++];
    
    // G = (G' - 0.5) * (2 * -idle) = (1 - 2 * G') * idle
    reward.G0.s = (s_th + 2.0 * G_scaled) * idleG;
    reward.G0.b = (b_th - 2.0 * G_scaled) * idleG;
    
    for (std::size_t j = 0; j < (std::size_t)idx::tckN; j++) reward.G[j].s  = (FLOAT)   0.0;
    for (std::size_t j = 0; j < (std::size_t)idx::tckN; j++) reward.G[j].b  = (FLOAT)   0.0;
    for (std::size_t j = 0; j < (std::size_t)idx::tckN; j++) reward.G[j].cs = (FLOAT) 100.0;
    for (std::size_t j = 0; j < (std::size_t)idx::tckN; j++) reward.G[j].cb = (FLOAT) 100.0;
    
    verify(targetDim == idxTarget);
}

}
