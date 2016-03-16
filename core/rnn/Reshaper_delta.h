#ifndef SIBYL_RESHAPER_DELTA_H_
#define SIBYL_RESHAPER_DELTA_H_

#include <array>

#include "Reshaper.h"

namespace sibyl
{

class ItemMem_delta : public ItemMem
{
public:
    std::array<PQ, szTb> lastTb;
    ItemMem_delta() : ItemMem(), lastTb{} {}
};

class Reshaper_delta : public Reshaper<ItemMem_delta>
{
public:
    Reshaper_delta(unsigned long maxGTck_);

    /*   raw   -> fractal */
    /*  sibyl  -> fractal */
    void State2Vec (FLOAT *vec, const ItemState &state);
    
    /*   ref   -> fractal */
    void Reward2Vec(FLOAT *vec, const Reward &reward, CSTR &code);
    
private:
    FLOAT ReshapeG_R2V_pos(FLOAT g) { return (FLOAT) ((g > 0.0f ? g : -0.001f) * kR2V); } // use only positive ref G values
};

}

#endif /* SIBYL_RESHAPER_DELTA_H_ */