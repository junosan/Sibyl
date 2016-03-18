#ifndef SIBYL_RESHAPER_DELTA_POS_H_
#define SIBYL_RESHAPER_DELTA_POS_H_

#include <array>

#include "Reshaper.h"

namespace sibyl
{

class ItemMem_delta_pos : public ItemMem
{
public:
    std::array<PQ, szTb> lastTb;
    ItemMem_delta_pos() : ItemMem(), lastTb{} {}
};

class Reshaper_delta_pos : public Reshaper<ItemMem_delta_pos>
{
public:
    Reshaper_delta_pos(unsigned long maxGTck_);

    /*   raw   -> fractal */
    /*  sibyl  -> fractal */
    void State2Vec (FLOAT *vec, const ItemState &state);
    
    /*   ref   -> fractal */
    void Reward2Vec(FLOAT *vec, const Reward &reward, CSTR &code) override;
    
private:
    FLOAT ReshapeG_R2V_pos(FLOAT g) { return (FLOAT) ((g > 0.0f ? g : -0.001f) * kR2V); } // use only positive ref G values
};

}

#endif /* SIBYL_RESHAPER_DELTA_POS_H_ */