
#include "Reshaper.h"

namespace sibyl
{

Reshaper::Reshaper(unsigned long maxGTck_)
{
    assert(maxGTck_ >= 0);
    maxGTck = maxGTck_;
    inputDim  = 43;
    targetDim = 2 + 4 * maxGTck;
}

void Reshaper::State2Vec(FLOAT *vec, const ItemState &state)
{
    const long interval = 10; // seconds
    const long T = (const long)(std::ceil((6 * 3600 - 10 * 60)/interval) - 1);
    
    auto iItems = items.find(state.code);
    if (iItems == std::end(items))
    {
        auto it_bool = items.insert(std::make_pair(state.code, ItemMem()));
        assert(it_bool.second == true);
        iItems = it_bool.first;
        iItems->second.initPr = state.pr;
    }
    auto &i = iItems->second; // reference to current ItemMem
    
    unsigned long idxInput = 0;
    
    // t
    vec[idxInput++] = (FLOAT) state.time / (interval * T);
    
    // pr
    vec[idxInput++] = ReshapePrice(state.pr) - ReshapePrice(i.initPr);
    
    // qr
    vec[idxInput++] = ReshapeQuant((INT) state.qr);
    
    // tbpr(1:20)
    for (std::size_t idx = 0; idx < (std::size_t)szTb; idx++)
        vec[idxInput++] = ReshapePrice(state.tbr[idx].p) - ReshapePrice(i.initPr);
    
    // tbqr(1:20)
    for (std::size_t idx = 0; idx < (std::size_t)szTb; idx++)
        vec[idxInput++] = ReshapeQuant(state.tbr[idx].q);
    
    assert(inputDim == idxInput);
}

void Reshaper::Reward2Vec(FLOAT *vec, const Reward &reward, CSTR &code)
{
    unsigned long idxTarget = 0;
    
    vec[idxTarget++] = ReshapeG_R2V(reward.G0.s);
    vec[idxTarget++] = ReshapeG_R2V(reward.G0.b);
    
    for (std::size_t j = 0; j < maxGTck; j++) vec[idxTarget++] = ReshapeG_R2V(reward.G[j].s );
    for (std::size_t j = 0; j < maxGTck; j++) vec[idxTarget++] = ReshapeG_R2V(reward.G[j].b );
    for (std::size_t j = 0; j < maxGTck; j++) vec[idxTarget++] = ReshapeG_R2V(reward.G[j].cs);
    for (std::size_t j = 0; j < maxGTck; j++) vec[idxTarget++] = ReshapeG_R2V(reward.G[j].cb);
    
    assert(targetDim == idxTarget);
}

void Reshaper::Vec2Reward(Reward &reward, const FLOAT *vec, CSTR &code)
{
    unsigned long idxTarget = 0;
    
    reward.G0.s = ReshapeG_V2R(vec[idxTarget++]);
    reward.G0.b = ReshapeG_V2R(vec[idxTarget++]);
    
    for (std::size_t j = 0; j < (std::size_t)szTck; j++) reward.G[j].s  = (j < maxGTck ? ReshapeG_V2R(vec[idxTarget++]) : (FLOAT)   0.0);
    for (std::size_t j = 0; j < (std::size_t)szTck; j++) reward.G[j].b  = (j < maxGTck ? ReshapeG_V2R(vec[idxTarget++]) : (FLOAT)   0.0);
    for (std::size_t j = 0; j < (std::size_t)szTck; j++) reward.G[j].cs = (j < maxGTck ? ReshapeG_V2R(vec[idxTarget++]) : (FLOAT) 100.0);
    for (std::size_t j = 0; j < (std::size_t)szTck; j++) reward.G[j].cb = (j < maxGTck ? ReshapeG_V2R(vec[idxTarget++]) : (FLOAT) 100.0);
    
    assert(targetDim == idxTarget);
}

}
