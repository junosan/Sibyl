#ifndef SIBYL_RESHAPER_0_H_
#define SIBYL_RESHAPER_0_H_

#include <array>
#include <vector>

#include "Reshaper.h"

namespace sibyl
{

class ItemMem_0 : public ItemMem
{
public:
    std::array<PQ, szTb> lastTb;
    std::vector<double> idleG;
    std::size_t cursor; // points at idx for current time in idleG
    ItemMem_0() : ItemMem(), lastTb{}, cursor(0) {}
};

class Reshaper_0 : public Reshaper<ItemMem_0>
{
public:
    Reshaper_0(unsigned long maxGTck_, // this will be ignored and overwritten
               TradeDataSet *pTradeDataSet_,
               std::vector<std::string> *pFileList_,
               const unsigned long (*ReadRawFile_)(std::vector<FLOAT>&, CSTR&, TradeDataSet*));

    /*   raw   -> fractal */
    /*  sibyl  -> fractal */
    void State2VecIn(FLOAT *vec, const ItemState &state);
    
    /*   ref   -> fractal */
    void Reward2VecOut(FLOAT *vec, const Reward &reward, CSTR &code) override;
    
    /* fractal ->  sibyl  */
    void VecOut2Reward(Reward &reward, const FLOAT *vec, CSTR &code) override;
};

}

#endif /* SIBYL_RESHAPER_0_H_ */