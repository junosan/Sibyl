/* ========================================================================== */
/*  Copyright (C) 2016 Hosang Yoon (hosangy@gmail.com) - All Rights Reserved  */
/*  Unauthorized copying of this file, via any medium is strictly prohibited  */
/*                        Proprietary and confidential                        */
/* ========================================================================== */

#ifndef SIBYL_Reshaper_0tT_H_
#define SIBYL_Reshaper_0tT_H_
// Same as Reshaper_0 except for removing t in inputDim

#include <array>
#include <vector>

#include "Reshaper.h"

namespace sibyl
{

class ItemMem_0t : public ItemMem
{
public:
    std::array<PQ, idx::szTb> lastTb;
    std::vector<double> idleG;
    std::size_t cursor; // points at idx for current time in idleG
    ItemMem_0t() : ItemMem(), lastTb{}, cursor(0) {}
};

class Reshaper_0t : public Reshaper<ItemMem_0t>
{
public:
    Reshaper_0t(unsigned long maxGTck_, // this will be ignored and overwritten
                TradeDataSet *pTradeDataSet_,
                std::vector<std::string> *pFileList_,
                const unsigned long (*ReadRawFile_)(std::vector<FLOAT>&, CSTR&, TradeDataSet*));

    void ReadConfig(CSTR &filename) override;

    /*   raw   -> fractal */
    /*  sibyl  -> fractal */
    void State2VecIn(FLOAT *vec, const ItemState &state) override;
    
    /*   ref   -> fractal */
    void Reward2VecOut(FLOAT *vec, const Reward &reward, CSTR &code) override;
    
    /* fractal ->  sibyl  */
    void VecOut2Reward(Reward &reward, const FLOAT *vec, CSTR &code) override;
    
private:
    double b_th, s_th;
};

}

#endif /* SIBYL_Reshaper_0tT_H_ */