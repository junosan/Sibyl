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
    Reshaper_delta_pos(unsigned long maxGTck_,
                       TradeDataSet *pTradeDataSet_,
                       std::vector<std::string> *pFileList_,
                       const unsigned long (*ReadRawFile_)(std::vector<FLOAT>&, const std::string&, TradeDataSet*));

    /*   raw   -> fractal */
    /*  sibyl  -> fractal */
    void State2VecIn(FLOAT *vec, const ItemState &state);

private:
    FLOAT ReshapeG_R2V(FLOAT g) override { return (FLOAT) ((g > 0.0f ? g : -0.001f) * kR2V); } // use only positive ref G values
};

}

#endif /* SIBYL_RESHAPER_DELTA_POS_H_ */