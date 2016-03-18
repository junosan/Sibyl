#ifndef SIBYL_RESHAPER_H_
#define SIBYL_RESHAPER_H_

#include <map>
#include <cmath>
#include <cassert>

#include "../sibyl/client/ItemState.h"
#include "../sibyl/client/Reward.h"

namespace sibyl
{

class ItemMem // storage class for each item (to implement whitening, etc.)
{
public:
    FLOAT initPr;
    
    ItemMem() : initPr(0.0f) {}
};

template <class TItemMem>
class Reshaper
{
public:
    Reshaper(unsigned long maxGTck_); // inputDim to be set in derived class's constructor
    unsigned long GetMaxGTck  () { return maxGTck;   }
    unsigned long GetInputDim () { return inputDim;  }
    unsigned long GetTargetDim() { return targetDim; }

    /*   raw   -> fractal */
    /*  sibyl  -> fractal */
    virtual void State2Vec (FLOAT *vec, const ItemState &state) = 0;
    
    /*   ref   -> fractal */
    virtual void Reward2Vec(FLOAT *vec, const Reward &reward, CSTR &code);
    
    /* fractal ->  sibyl  */
    virtual void Vec2Reward(Reward &reward, const FLOAT *vec, CSTR &code);
    
protected:
    FLOAT ReshapePrice(FLOAT p) { return (FLOAT) (std::log((FLOAT) p)   * 100.0); }
    FLOAT ReshapeQuant(INT   q) { return (FLOAT) (std::log((FLOAT) 1 + std::abs(q)) * ((q > 0) - (q < 0)) / 4.0); }
    
    constexpr static const double kR2V = 100.0;
    FLOAT ReshapeG_R2V(FLOAT g) { return (FLOAT)(g * kR2V); }
    FLOAT ReshapeG_V2R(FLOAT g) { return (FLOAT)(g / kR2V); }
    
    unsigned long maxGTck, inputDim, targetDim;
    std::map<STR, TItemMem> items;
};

template <class TItemMem>
Reshaper<TItemMem>::Reshaper(unsigned long maxGTck_)
{
    assert(maxGTck_ >= 0);
    maxGTck = maxGTck_;
    targetDim = 2 + 4 * maxGTck;
}

template <class TItemMem>
void Reshaper<TItemMem>::Reward2Vec(FLOAT *vec, const Reward &reward, CSTR &code)
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

template <class TItemMem>
void Reshaper<TItemMem>::Vec2Reward(Reward &reward, const FLOAT *vec, CSTR &code)
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

#endif /* SIBYL_RESHAPER_H_ */