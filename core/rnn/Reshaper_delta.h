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
    Reshaper_delta(unsigned long maxGTck_,
                   TradeDataSet *pTradeDataSet_,
                   std::vector<std::string> *pFileList_,
                   const unsigned long (*ReadRawFile_)(std::vector<FLOAT>&, const std::string&, TradeDataSet*));

    /*   raw   -> fractal */
    /*  sibyl  -> fractal */
    void State2VecIn(FLOAT *vec, const ItemState &state);
};

}

#endif /* SIBYL_RESHAPER_DELTA_H_ */