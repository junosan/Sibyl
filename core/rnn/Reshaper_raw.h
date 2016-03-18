#ifndef SIBYL_RESHAPER_RAW_H_
#define SIBYL_RESHAPER_RAW_H_

#include "Reshaper.h"

namespace sibyl
{

class Reshaper_raw : public Reshaper<ItemMem>
{
public:
    Reshaper_raw(unsigned long maxGTck_);

    /*   raw   -> fractal */
    /*  sibyl  -> fractal */
    void State2Vec(FLOAT *vec, const ItemState &state);
};

}

#endif /* SIBYL_RESHAPER_RAW_H_ */