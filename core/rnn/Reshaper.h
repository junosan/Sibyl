#ifndef RESHAPER_H_
#define RESHAPER_H_

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

class Reshaper
{
public:
    Reshaper(unsigned long maxGTck_);
    unsigned long GetMaxGTck  () { return maxGTck;   }
    unsigned long GetInputDim () { return inputDim;  }
    unsigned long GetTargetDim() { return targetDim; }

    /*   raw   -> fractal */
    /*  sibyl  -> fractal */
    void State2Vec (FLOAT *vec, const ItemState &state);
    
    /*   ref   -> fractal */
    void Reward2Vec(FLOAT *vec, const Reward &reward, CSTR &code);
    
    /* fractal ->  sibyl  */
    void Vec2Reward(Reward &reward, const FLOAT *vec, CSTR &code);
    
protected:
    FLOAT ReshapePrice(FLOAT p) { return (FLOAT) (std::log((FLOAT) p)   * 100.0); }
    FLOAT ReshapeQuant(INT   q) { return (FLOAT) (std::log((FLOAT) 1 + q) / 4.0); }
    
    constexpr static const double kR2V = 100.0;
    FLOAT ReshapeG_R2V(FLOAT g) { return (FLOAT)(g * kR2V); }
    FLOAT ReshapeG_V2R(FLOAT g) { return (FLOAT)(g / kR2V); }
    
    unsigned long maxGTck, inputDim, targetDim;
    std::map<STR, ItemMem> items;
};

}

#endif /* RESHAPER_H_ */