#ifndef SIBYL_RESHAPER_DELTA_WHITEN_H_
#define SIBYL_RESHAPER_DELTA_WHITEN_H_

#include <array>

#include "Reshaper.h"

namespace sibyl
{

class ItemMem_delta_whiten : public ItemMem
{
public:
    std::array<PQ, szTb> lastTb;
    ItemMem_delta_whiten() : ItemMem(), lastTb{} {}
};

class Reshaper_delta_whiten : public Reshaper<ItemMem_delta_whiten>
{
public:
    Reshaper_delta_whiten(unsigned long maxGTck_,
                          TradeDataSet *pTradeDataSet_,
                          std::vector<std::string> *pFileList_,
                          const unsigned long (*ReadRawFile_)(std::vector<FLOAT>&, CSTR&, TradeDataSet*));

    /*   raw   -> fractal */
    /*  sibyl  -> fractal */
    void State2VecIn(FLOAT *vec, const ItemState &state);
};

}

#endif /* SIBYL_RESHAPER_DELTA_WHITEN_H_ */