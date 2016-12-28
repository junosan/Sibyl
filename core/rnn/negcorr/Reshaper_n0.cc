/* ========================================================================== */
/*  Copyright (C) 2016 Hosang Yoon (hosangy@gmail.com) - All Rights Reserved  */
/*  Unauthorized copying of this file, via any medium is strictly prohibited  */
/*                        Proprietary and confidential                        */
/* ========================================================================== */

#include "Reshaper_n0.h"

#include <sibyl/Security.h>
#include <sibyl/time_common.h>
#include <sibyl/util/Config.h>

namespace sibyl
{

Reshaper_n0::Reshaper_n0(unsigned long maxGTck_,
                        TradeDataSet *pTradeDataSet_,
                        std::vector<std::string> *pFileList_,
                        const unsigned long (TradeDataSet::* ReadRawFile_)(std::vector<FLOAT>&, CSTR&))
                        : Reshaper(maxGTck_, pTradeDataSet_, pFileList_, ReadRawFile_),
                          b_th(1.0), s_th(1.0)
{
    maxGTck   = 0;  // overwrite Reshaper's constructor value 
    inputDim  = 44;
    targetDim = 2;  // overwrite Reshaper's constructor value
}

void Reshaper_n0::ReadConfig(CSTR &filename)
{
    Config cfg(filename);
    
    auto &ss_b_th = cfg.Get("B_TH");
    ss_b_th >> b_th;
    verify(ss_b_th.fail() == false);
    
    auto &ss_s_th = cfg.Get("S_TH");
    ss_s_th >> s_th;
    verify(ss_s_th.fail() == false);
}

void Reshaper_n0::State2VecIn(FLOAT *vec, const ItemState &state)
{
    // const long interval = kTimeRates::secPerTick; // seconds
    // const long T = (const long)(std::ceil((6 * 3600 - 10 * 60)/interval) - 1);
    
    auto iItems = items.find(state.code);
    if (iItems == std::end(items))
    {
        auto it_bool = items.insert(std::make_pair(state.code, ItemMem()));
        verify(it_bool.second == true);
        iItems = it_bool.first;
        iItems->second.initPr = state.tbr[idx::ps1].p;
    }
    auto &i = iItems->second; // reference to current ItemMem

    unsigned long idxInput = 0;
    
    // // t
    // vec[idxInput++] = (FLOAT) state.time / (interval * T);
    
    // pr
    vec[idxInput++] = ReshapePrice(state.pr) - ReshapePrice(i.initPr);
    
    // qr
    vec[idxInput++] = ReshapeQuant((INT) state.qr);
    
    // ps1
    vec[idxInput++] = ReshapePrice(state.tbr[idx::ps1].p) - ReshapePrice(i.initPr);
    
    // ps1 - pb1
    vec[idxInput++] = ReshapePrice(state.tbr[idx::ps1].p) - ReshapePrice(state.tbr[idx::pb1].p);
    
    // // tbpr(1:20)
    // for (std::size_t idx = 0; idx < (std::size_t)idx::szTb; idx++)
    //     vec[idxInput++] = ReshapePrice(state.tbr[idx].p) - ReshapePrice(i.initPr);
    
    // tbqr(1:20)
    for (std::size_t idx = 0; idx < (std::size_t)idx::szTb; idx++)
        vec[idxInput++] = ReshapeQuant(state.tbr[idx].q);
    
    // delta_tbqr(1:20)
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
        vec[idxInput++] = ReshapeQuant(delta);
    }
    i.lastTb = state.tbr;
    
    verify(inputDim == idxInput);
    
    WhitenVector(vec); // this alters vector only if matrices are initialized
}

void Reshaper_n0::Reward2VecOut(FLOAT *vec, const Reward &reward, CSTR &code)
{
    if (vec == nullptr) return;

    // Supply target with similar scales as before for similar magnitudes of gradients

    unsigned long idxTarget = 0;
    
    vec[idxTarget++] = ReshapeG_R2V(reward.G0.s);
    vec[idxTarget++] = ReshapeG_R2V(reward.G0.b);
    
    verify(targetDim == idxTarget);
}

void Reshaper_n0::VecOut2Reward(Reward &reward, const FLOAT *vec, CSTR &code)
{
    unsigned long idxTarget = 0;

    reward.G0.s = vec[idxTarget++] - s_th;
    reward.G0.b = vec[idxTarget++] - b_th;
    
    for (std::size_t j = 0; j < (std::size_t)idx::tckN; j++) reward.G[j].s  = (FLOAT)   0.0;
    for (std::size_t j = 0; j < (std::size_t)idx::tckN; j++) reward.G[j].b  = (FLOAT)   0.0;
    for (std::size_t j = 0; j < (std::size_t)idx::tckN; j++) reward.G[j].cs = (FLOAT) 100.0;
    for (std::size_t j = 0; j < (std::size_t)idx::tckN; j++) reward.G[j].cb = (FLOAT) 100.0;
    
    verify(targetDim == idxTarget);
}

}
